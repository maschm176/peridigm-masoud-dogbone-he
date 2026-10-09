/*! \file Peridigm_BondLevelArtificialViscosityDampingModel.cpp */

#include <cmath>
#include "Peridigm_BondLevelArtificialViscosityDampingModel.hpp"
#include "Peridigm_Field.hpp"

using namespace std;

PeridigmNS::BondLevelArtificialViscosityDampingModel::BondLevelArtificialViscosityDampingModel(const Teuchos::ParameterList& params)
  : DampingModel(params),
    m_viscosityCoefficient(0.0),
    m_useBondDamage(false),
    m_volumeFieldId(-1),
    m_velocityFieldId(-1),
    m_coordinatesFieldId(-1),
    m_forceDensityFieldId(-1),
    m_bondDamageFieldId(-1)
{
  m_viscosityCoefficient = params.get<double>("Viscosity Coefficient");

  PeridigmNS::FieldManager& fieldManager = PeridigmNS::FieldManager::self();

  m_volumeFieldId       = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Volume");
  m_velocityFieldId     = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Velocity");
  m_coordinatesFieldId  = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Coordinates");
  m_forceDensityFieldId = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Force_Density");

  m_fieldIds.push_back(m_volumeFieldId);
  m_fieldIds.push_back(m_velocityFieldId);
  m_fieldIds.push_back(m_coordinatesFieldId);
  m_fieldIds.push_back(m_forceDensityFieldId);

  // Only fade by bond damage if some damage model elsewhere in this run has
  // already registered the field -- a block using this damping model
  // without any damage model simply gets undiminished artificial viscosity.
  m_useBondDamage = fieldManager.hasField("Bond_Damage");
  if(m_useBondDamage){
    m_bondDamageFieldId = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Bond_Damage");
    m_fieldIds.push_back(m_bondDamageFieldId);
  }
}

PeridigmNS::BondLevelArtificialViscosityDampingModel::~BondLevelArtificialViscosityDampingModel()
{
}

void
PeridigmNS::BondLevelArtificialViscosityDampingModel::computeDamping(const double dt,
                                                                      const int numOwnedPoints,
                                                                      const int* ownedIDs,
                                                                      const int* neighborhoodList,
                                                                      PeridigmNS::DataManager& dataManager) const
{
  double *volume, *velocity, *coordinates, *force;
  dataManager.getData(m_volumeFieldId, PeridigmField::STEP_NONE)->ExtractView(&volume);
  dataManager.getData(m_velocityFieldId, PeridigmField::STEP_NP1)->ExtractView(&velocity);
  dataManager.getData(m_coordinatesFieldId, PeridigmField::STEP_NP1)->ExtractView(&coordinates);
  dataManager.getData(m_forceDensityFieldId, PeridigmField::STEP_NP1)->ExtractView(&force);

  double *bondDamage = NULL;
  if(m_useBondDamage)
    dataManager.getData(m_bondDamageFieldId, PeridigmField::STEP_NP1)->ExtractView(&bondDamage);

  const int *neighborListPtr = neighborhoodList;
  int numNeighbors, neighborIndex, nodeID;

  for(int iID=0 ; iID<numOwnedPoints ; ++iID){
    nodeID = ownedIDs[iID];
    numNeighbors = *neighborListPtr; neighborListPtr++;

    for(int n=0; n<numNeighbors; n++, neighborListPtr++){
      neighborIndex = *neighborListPtr;

      double weight = m_useBondDamage ? (1.0 - (*bondDamage)) : 1.0;
      if(m_useBondDamage)
        bondDamage++;

      if(weight <= 0.0)
        continue;

      double bondX = coordinates[neighborIndex*3]   - coordinates[nodeID*3];
      double bondY = coordinates[neighborIndex*3+1] - coordinates[nodeID*3+1];
      double bondZ = coordinates[neighborIndex*3+2] - coordinates[nodeID*3+2];
      double bondLength = sqrt(bondX*bondX + bondY*bondY + bondZ*bondZ);
      if(bondLength <= 0.0)
        continue;
      double xiHatX = bondX/bondLength;
      double xiHatY = bondY/bondLength;
      double xiHatZ = bondZ/bondLength;

      double relVelX = velocity[neighborIndex*3]   - velocity[nodeID*3];
      double relVelY = velocity[neighborIndex*3+1] - velocity[nodeID*3+1];
      double relVelZ = velocity[neighborIndex*3+2] - velocity[nodeID*3+2];

      // sdot = rate of bond elongation along its own current direction
      double sdot = relVelX*xiHatX + relVelY*xiHatY + relVelZ*xiHatZ;

      double neighborVolume = volume[neighborIndex];
      double c = m_viscosityCoefficient * weight * neighborVolume;

      // Quadratic in sdot (sign-preserving: sdot*|sdot|), opposing the
      // bond's own current stretch/compression rate. Dashpot sign
      // convention: when stretching (sdot>0), pull node i TOWARD its
      // neighbor (force along +xiHat) to resist further elongation; when
      // compressing (sdot<0), push i AWAY from its neighbor to resist
      // further compression. By construction magnitude already carries the
      // correct sign (same sign as sdot), so it is added, not subtracted.
      double magnitude = c * sdot * fabs(sdot);

      force[nodeID*3]   += magnitude * xiHatX;
      force[nodeID*3+1] += magnitude * xiHatY;
      force[nodeID*3+2] += magnitude * xiHatZ;
    }
  }
}
