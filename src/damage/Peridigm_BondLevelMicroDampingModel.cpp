/*! \file Peridigm_BondLevelMicroDampingModel.cpp */

#include "Peridigm_BondLevelMicroDampingModel.hpp"
#include "Peridigm_Field.hpp"

using namespace std;

PeridigmNS::BondLevelMicroDampingModel::BondLevelMicroDampingModel(const Teuchos::ParameterList& params)
  : DampingModel(params),
    m_dampingCoefficient(0.0),
    m_useBondDamage(false),
    m_volumeFieldId(-1),
    m_velocityFieldId(-1),
    m_forceDensityFieldId(-1),
    m_bondDamageFieldId(-1)
{
  m_dampingCoefficient = params.get<double>("Damping Coefficient");

  PeridigmNS::FieldManager& fieldManager = PeridigmNS::FieldManager::self();

  m_volumeFieldId       = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Volume");
  m_velocityFieldId     = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Velocity");
  m_forceDensityFieldId = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Force_Density");

  m_fieldIds.push_back(m_volumeFieldId);
  m_fieldIds.push_back(m_velocityFieldId);
  m_fieldIds.push_back(m_forceDensityFieldId);

  // Only fade damping by bond damage if some damage model elsewhere in this
  // run has already registered the field -- a block using this damping
  // model without any damage model simply gets undiminished damping.
  m_useBondDamage = fieldManager.hasField("Bond_Damage");
  if(m_useBondDamage){
    m_bondDamageFieldId = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Bond_Damage");
    m_fieldIds.push_back(m_bondDamageFieldId);
  }
}

PeridigmNS::BondLevelMicroDampingModel::~BondLevelMicroDampingModel()
{
}

void
PeridigmNS::BondLevelMicroDampingModel::computeDamping(const double dt,
                                                        const int numOwnedPoints,
                                                        const int* ownedIDs,
                                                        const int* neighborhoodList,
                                                        PeridigmNS::DataManager& dataManager) const
{
  double *volume, *velocity, *force;
  dataManager.getData(m_volumeFieldId, PeridigmField::STEP_NONE)->ExtractView(&volume);
  dataManager.getData(m_velocityFieldId, PeridigmField::STEP_NP1)->ExtractView(&velocity);
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

      double neighborVolume = volume[neighborIndex];
      double c = m_dampingCoefficient * weight * neighborVolume;

      force[nodeID*3]   -= c * (velocity[nodeID*3]   - velocity[neighborIndex*3]);
      force[nodeID*3+1] -= c * (velocity[nodeID*3+1] - velocity[neighborIndex*3+1]);
      force[nodeID*3+2] -= c * (velocity[nodeID*3+2] - velocity[neighborIndex*3+2]);
    }
  }
}
