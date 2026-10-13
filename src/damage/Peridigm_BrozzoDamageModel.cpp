/*! \file Peridigm_BrozzoDamageModel.cpp */

//@HEADER
// ************************************************************************
//
//                             Peridigm
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?
// David J. Littlewood   djlittl@sandia.gov
// John A. Mitchell      jamitch@sandia.gov
// Michael L. Parks      mlparks@sandia.gov
// Stewart A. Silling    sasilli@sandia.gov
//
// ************************************************************************
//@HEADER

#include "Peridigm_BrozzoDamageModel.hpp"
#include "Peridigm_Field.hpp"
#include <algorithm>    // std::max
#include <cmath>        // std::pow
#include "material_utilities.h" // to use Influence Function
#include "correspondence.h" // to compute weighted volume
#include <Epetra_Comm.h> // to synchronize the Fracture Plane Cutoff trigger across ranks

using namespace std;

PeridigmNS::BrozzoDamageModel::BrozzoDamageModel(const Teuchos::ParameterList& params)
  : DamageModel(params),
    m_thresholdDamage(0.0),
    m_damageParameter1(0.0),
    m_damageParameter2(0.0),
    m_criticalDamageToNeglectMaterialPoint(0.95),
    m_hardCutoff(false),
    m_fracturePlaneEnabled(false),
    m_fracturePlaneY(0.0),
    m_fracturePlaneHalfThickness(0.25),
    m_fractureTriggerDamage(0.3),
    m_bondCleanupOnFlyingPoint(false),
    m_geometricStretchCutoffEnabled(false),
    m_geometricCriticalStretch(0.0),
    m_connectivitySeveranceEnabled(false),
    m_connectivityAsymmetryThreshold(0.0),
    m_symmetricSeverancePropagationEnabled(false),
    m_symmetricSeverancePropagationRate(1.0),
    m_bondCleanupRate(1.0),
    m_damageFieldId(-1),
    m_flyingPointFlagFieldId(-1),
    m_bondDamageFieldId(-1),
    m_bondLevelEquivalentPlasticStrainFieldId(-1),
    m_bondLevelStressTriaxialityFieldId(-1),
    m_bondLevelVonMisesStressFieldId(-1),
    m_bondLevelMaxPrincipalStressFieldId(-1),
    m_bondLevelDamageParameterFieldId(-1),
    m_brokenBondVolumeAveragedFieldId(-1),
    m_connectivityAsymmetryFieldId(-1),
    m_connectivityAsymmetryBaselineFieldId(-1),
    m_horizonFieldId(-1),
    m_volumeFieldId(-1),
    m_coordinatesFieldId(-1),
    m_modelCoordinatesFieldId(-1),
    m_jacobianDeterminantFieldId(-1),
    m_undamagedWeightedVolumeFieldId(-1)
{

  // Brozzo/Wellman ductile damage model:
  //   D_dot = d1 * <2*sigmaT/(3*(sigmaT - sigmaM))>^d2 * eqps_dot,  if D < 1
  m_damageParameter1 = params.get<double>("Damage Parameter 1");
  m_damageParameter2 = params.get<double>("Damage Parameter 2");
  m_thresholdDamage = params.get<double>("Threshold Damage");
  TEUCHOS_TEST_FOR_EXCEPT_MSG(1.0 <= m_thresholdDamage, "**** Error: Threshold Damage should be less than 1.0");

  if(params.isParameter("Critical Damage To Neglect Material Point"))
    m_criticalDamageToNeglectMaterialPoint = params.get<double>("Critical Damage To Neglect Material Point"); // default value is 95%

  if(params.isParameter("Hard Cutoff"))
    m_hardCutoff = params.get<bool>("Hard Cutoff"); // default is false (linear ramp)

  if(params.isParameter("Enable Fracture Plane Cutoff"))
    m_fracturePlaneEnabled = params.get<bool>("Enable Fracture Plane Cutoff"); // default is false
  if(params.isParameter("Fracture Plane Y"))
    m_fracturePlaneY = params.get<double>("Fracture Plane Y");
  if(params.isParameter("Fracture Plane Half Thickness"))
    m_fracturePlaneHalfThickness = params.get<double>("Fracture Plane Half Thickness");
  if(params.isParameter("Fracture Trigger Damage"))
    m_fractureTriggerDamage = params.get<double>("Fracture Trigger Damage");

  if(params.isParameter("Enable Bond Cleanup On Flying Point"))
    m_bondCleanupOnFlyingPoint = params.get<bool>("Enable Bond Cleanup On Flying Point"); // default is false

  if(params.isParameter("Enable Geometric Stretch Cutoff"))
    m_geometricStretchCutoffEnabled = params.get<bool>("Enable Geometric Stretch Cutoff"); // default is false
  if(params.isParameter("Geometric Critical Stretch"))
    m_geometricCriticalStretch = params.get<double>("Geometric Critical Stretch");
  TEUCHOS_TEST_FOR_EXCEPT_MSG(m_geometricStretchCutoffEnabled && !params.isParameter("Geometric Critical Stretch"),
    "**** Error: Enable Geometric Stretch Cutoff requires Geometric Critical Stretch to be set");

  if(params.isParameter("Enable Connectivity Severance"))
    m_connectivitySeveranceEnabled = params.get<bool>("Enable Connectivity Severance"); // default is false
  if(params.isParameter("Connectivity Asymmetry Threshold"))
    m_connectivityAsymmetryThreshold = params.get<double>("Connectivity Asymmetry Threshold");
  TEUCHOS_TEST_FOR_EXCEPT_MSG(m_connectivitySeveranceEnabled && !params.isParameter("Connectivity Asymmetry Threshold"),
    "**** Error: Enable Connectivity Severance requires Connectivity Asymmetry Threshold to be set");

  if(params.isParameter("Enable Symmetric Severance Propagation"))
    m_symmetricSeverancePropagationEnabled = params.get<bool>("Enable Symmetric Severance Propagation"); // default is false
  if(params.isParameter("Symmetric Severance Propagation Rate"))
    m_symmetricSeverancePropagationRate = params.get<double>("Symmetric Severance Propagation Rate"); // default is 1.0 (instant)
  if(params.isParameter("Bond Cleanup Rate"))
    m_bondCleanupRate = params.get<double>("Bond Cleanup Rate"); // default is 1.0 (instant)

  PeridigmNS::FieldManager& fieldManager = PeridigmNS::FieldManager::self();

  m_damageFieldId                                  = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Damage");
  m_flyingPointFlagFieldId                         = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Flying_Point_Flag");
  m_bondDamageFieldId                              = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Bond_Damage");
  m_brokenBondVolumeAveragedFieldId                = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Broken_Bond_Volume_Averaged");
  m_connectivityAsymmetryFieldId                   = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Connectivity_Asymmetry");
  m_connectivityAsymmetryBaselineFieldId           = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Connectivity_Asymmetry_Baseline");
  m_horizonFieldId                                 = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Horizon");
  m_volumeFieldId                                  = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Volume");
  m_coordinatesFieldId                             = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::TWO_STEP, "Coordinates");
  m_modelCoordinatesFieldId                        = fieldManager.getFieldId(PeridigmField::NODE,    PeridigmField::VECTOR, PeridigmField::CONSTANT, "Model_Coordinates");
  m_jacobianDeterminantFieldId                     = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Jacobian_Determinant");
  m_undamagedWeightedVolumeFieldId                 = fieldManager.getFieldId(PeridigmField::ELEMENT, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Undamaged_Weighted_Volume");

  m_fieldIds.push_back(m_damageFieldId);
  m_fieldIds.push_back(m_flyingPointFlagFieldId);
  m_fieldIds.push_back(m_bondDamageFieldId);
  m_fieldIds.push_back(m_brokenBondVolumeAveragedFieldId);
  m_fieldIds.push_back(m_connectivityAsymmetryFieldId);
  m_fieldIds.push_back(m_connectivityAsymmetryBaselineFieldId);
  m_fieldIds.push_back(m_horizonFieldId);
  m_fieldIds.push_back(m_volumeFieldId);
  m_fieldIds.push_back(m_coordinatesFieldId);
  m_fieldIds.push_back(m_modelCoordinatesFieldId);
  m_fieldIds.push_back(m_jacobianDeterminantFieldId);
  m_fieldIds.push_back(m_undamagedWeightedVolumeFieldId);

  m_bondLevelEquivalentPlasticStrainFieldId        = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Bond_Equivalent_Plastic_Strain");
  m_bondLevelStressTriaxialityFieldId              = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Bond_Stress_Triaxiality");
  m_bondLevelVonMisesStressFieldId                 = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Bond_Von_Mises_Stress");
  m_bondLevelMaxPrincipalStressFieldId             = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::CONSTANT, "Bond_Max_Principal_Stress");
  m_bondLevelDamageParameterFieldId                = fieldManager.getFieldId(PeridigmField::BOND, PeridigmField::SCALAR, PeridigmField::TWO_STEP, "Bond_Damage_Parameter");
  m_fieldIds.push_back(m_bondLevelEquivalentPlasticStrainFieldId);
  m_fieldIds.push_back(m_bondLevelStressTriaxialityFieldId);
  m_fieldIds.push_back(m_bondLevelVonMisesStressFieldId);
  m_fieldIds.push_back(m_bondLevelMaxPrincipalStressFieldId);
  m_fieldIds.push_back(m_bondLevelDamageParameterFieldId);
}

PeridigmNS::BrozzoDamageModel::~BrozzoDamageModel()
{
}

void
PeridigmNS::BrozzoDamageModel::initialize(const double dt,
                                          const int numOwnedPoints,
                                          const int* ownedIDs,
                                          const int* neighborhoodList,
                                          PeridigmNS::DataManager& dataManager) const
{
  dataManager.getData(m_damageFieldId, PeridigmField::STEP_N)->PutScalar(0.0);
  dataManager.getData(m_damageFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);

  dataManager.getData(m_bondLevelDamageParameterFieldId, PeridigmField::STEP_N)->PutScalar(0.0);
  dataManager.getData(m_bondLevelDamageParameterFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);
  dataManager.getData(m_brokenBondVolumeAveragedFieldId, PeridigmField::STEP_N)->PutScalar(0.0);
  dataManager.getData(m_brokenBondVolumeAveragedFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);
  dataManager.getData(m_connectivityAsymmetryFieldId, PeridigmField::STEP_N)->PutScalar(0.0);
  dataManager.getData(m_connectivityAsymmetryFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);

  // Connectivity Severance baseline (see header comment): computed once,
  // here, from the undeformed reference geometry with zero damage assumed
  // everywhere (true at this point in the simulation) -- captures the
  // purely-geometric resultant asymmetry a healthy point has just from
  // being near a free surface, so computeDamage() can later subtract it
  // out and isolate the damage-induced component.
  double *modelCoordinates, *horizon, *volume, *connectivityAsymmetryBaseline;
  dataManager.getData(m_modelCoordinatesFieldId, PeridigmField::STEP_NONE)->ExtractView(&modelCoordinates);
  dataManager.getData(m_horizonFieldId, PeridigmField::STEP_NONE)->ExtractView(&horizon);
  dataManager.getData(m_volumeFieldId, PeridigmField::STEP_NONE)->ExtractView(&volume);
  dataManager.getData(m_connectivityAsymmetryBaselineFieldId, PeridigmField::STEP_NONE)->ExtractView(&connectivityAsymmetryBaseline);

  const double* coord = modelCoordinates;
  const double* delta = horizon;
  const int* neighborListPtr = neighborhoodList;

  for(int iID=0 ; iID<numOwnedPoints ; ++iID, coord+=3, ++delta, ++connectivityAsymmetryBaseline){

    int numNeighbors = *neighborListPtr; neighborListPtr++;

    double sumWeightedDirX = 0.0, sumWeightedDirY = 0.0, sumWeightedDirZ = 0.0, sumWeight = 0.0;

    for(int n=0; n<numNeighbors; n++, neighborListPtr++){
      int neighborIndex = *neighborListPtr;
      const double* neighborCoord = modelCoordinates + 3*neighborIndex;

      double bondX = *(neighborCoord)   - *(coord);
      double bondY = *(neighborCoord+1) - *(coord+1);
      double bondZ = *(neighborCoord+2) - *(coord+2);
      double bondLength = sqrt(bondX*bondX + bondY*bondY + bondZ*bondZ);

      if(bondLength > 0.0){
        double omega = MATERIAL_EVALUATION::scalarInfluenceFunction(bondLength, *delta);
        double weight = omega * volume[neighborIndex];
        sumWeight += weight;
        sumWeightedDirX += weight * bondX / bondLength;
        sumWeightedDirY += weight * bondY / bondLength;
        sumWeightedDirZ += weight * bondZ / bondLength;
      }
    }

    if(sumWeight > 0.0){
      double resultantMag = sqrt(sumWeightedDirX*sumWeightedDirX +
                                 sumWeightedDirY*sumWeightedDirY +
                                 sumWeightedDirZ*sumWeightedDirZ);
      *connectivityAsymmetryBaseline = resultantMag / sumWeight;
    }
    else{
      *connectivityAsymmetryBaseline = 0.0;
    }
  }
}

void
PeridigmNS::BrozzoDamageModel::computeDamage(const double dt,
                                             const int numOwnedPoints,
                                             const int* ownedIDs,
                                             const int* neighborhoodList,
                                             PeridigmNS::DataManager& dataManager) const
{
  //NOTE: this fixes the inter-processor and inter-block double counting problem
  dataManager.getData(m_damageFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);

  double *damageN, *damageNP1, *brokenBondVolumeAveragedNP1, *connectivityAsymmetryNP1, *connectivityAsymmetryBaseline;
  dataManager.getData(m_damageFieldId, PeridigmField::STEP_N)->ExtractView(&damageN);
  dataManager.getData(m_damageFieldId, PeridigmField::STEP_NP1)->ExtractView(&damageNP1);
  dataManager.getData(m_brokenBondVolumeAveragedFieldId, PeridigmField::STEP_NP1)->ExtractView(&brokenBondVolumeAveragedNP1);
  dataManager.getData(m_connectivityAsymmetryFieldId, PeridigmField::STEP_NP1)->ExtractView(&connectivityAsymmetryNP1);
  dataManager.getData(m_connectivityAsymmetryBaselineFieldId, PeridigmField::STEP_NONE)->ExtractView(&connectivityAsymmetryBaseline);

  // Stable base pointers (never advanced), needed below by the Symmetric
  // Severance Propagation second pass, which re-visits every point and
  // must be able to index any point's aggregate fields directly by its
  // local ID -- unlike the main loop below, which consumes these same
  // Extracted pointers via per-point/per-bond pointer arithmetic.
  double* damageNP1Base = damageNP1;
  double* brokenBondVolumeAveragedNP1Base = brokenBondVolumeAveragedNP1;
  double* connectivityAsymmetryNP1Base = connectivityAsymmetryNP1;
  double* connectivityAsymmetryBaselineBase = connectivityAsymmetryBaseline;

  double *horizon, *volume, *coordinates, *modelCoordinates;
  double *undamagedWeightedVolume, *jacobianDeterminant;
  dataManager.getData(m_horizonFieldId, PeridigmField::STEP_NONE)->ExtractView(&horizon);
  dataManager.getData(m_volumeFieldId, PeridigmField::STEP_NONE)->ExtractView(&volume);
  dataManager.getData(m_coordinatesFieldId, PeridigmField::STEP_NP1)->ExtractView(&coordinates);
  dataManager.getData(m_modelCoordinatesFieldId, PeridigmField::STEP_NONE)->ExtractView(&modelCoordinates);
  dataManager.getData(m_undamagedWeightedVolumeFieldId, PeridigmField::STEP_NONE)->ExtractView(&undamagedWeightedVolume);
  dataManager.getData(m_jacobianDeterminantFieldId, PeridigmField::STEP_N)->ExtractView(&jacobianDeterminant);

  double* undamagedWeightedVolumeBase = undamagedWeightedVolume;

  double* coord = coordinates;
  double* neighborCoord;
  double* delta = horizon;

  double deformedBondX, deformedBondY, deformedBondZ, deformedBondLength;
  double neighborVolume, omega;

  double deqps, sigmaT, sigmaM, denom, ratio, deltaDamage;

  const int *neighborListPtr = neighborhoodList;
  int numNeighbors, neighborIndex;

  // Zero out the bond damage to compute intact weighted volume (to
  // eventually evaluate damage of material points)
  dataManager.getData(m_bondDamageFieldId, PeridigmField::STEP_NP1)->PutScalar(0.0);
  dataManager.getData(m_flyingPointFlagFieldId, PeridigmField::STEP_NP1)->PutScalar(-1.0);

  double *bondDamage, *flyingPointFlag;
  dataManager.getData(m_flyingPointFlagFieldId, PeridigmField::STEP_NP1)->ExtractView(&flyingPointFlag);
  dataManager.getData(m_bondDamageFieldId, PeridigmField::STEP_NP1)->ExtractView(&bondDamage);

  // Stable base pointers for the Symmetric Severance Propagation second
  // pass (see below): flyingPointFlagBase is indexed by LOCAL ID (owned or
  // ghost -- same convention already used for "coordinates + 3*neighborIndex"
  // elsewhere in this file), and bondDamageBase/damageParameterNP1Base are
  // indexed by a running bond offset that mirrors the main loop's own
  // per-bond traversal order.
  double* flyingPointFlagBase = flyingPointFlag;
  double* bondDamageBase = bondDamage;

  // Set the bond damage to the previous value
  *(dataManager.getData(m_bondDamageFieldId, PeridigmField::STEP_NP1)) = *(dataManager.getData(m_bondDamageFieldId, PeridigmField::STEP_N));

  // Fracture Plane Cutoff: a preliminary, rank-local scan to decide whether
  // ANY point owned by THIS rank, within the fixed geometric band around
  // Fracture Plane Y, has already accumulated enough damage to count as
  // "fracture has occurred here". Uses damageN (the fully-settled
  // previous-step value) so this doesn't depend on the order in which
  // points are visited later in this same call.
  bool planeTriggered = false;
  if(m_fracturePlaneEnabled){
    const double* scanCoord = coordinates;
    const double* scanDamageN = damageN;
    for(int iID=0 ; iID<numOwnedPoints ; ++iID, scanCoord+=3, ++scanDamageN){
      if(std::abs(scanCoord[1] - m_fracturePlaneY) <= m_fracturePlaneHalfThickness &&
         *scanDamageN > m_fractureTriggerDamage){
        planeTriggered = true;
        break;
      }
    }

    // Synchronize across ranks: if ANY rank found a qualifying point, ALL
    // ranks must treat the plane as triggered, so severance of bonds that
    // cross a processor boundary stays symmetric/consistent on both sides.
    // Standard Epetra collective, same pattern used elsewhere in Peridigm
    // (e.g. Peridigm_ExodusDiscretization.cpp's MaxAll calls).
    double localTriggered = planeTriggered ? 1.0 : 0.0;
    double globalTriggered = 0.0;
    dataManager.getEpetraComm()->MaxAll(&localTriggered, &globalTriggered, 1);
    planeTriggered = (globalTriggered >= 0.5);
  }

  // Update the bond damage directly based on its own properties
  double *equivalentPlasticStrainN, *equivalentPlasticStrainNP1, *stressTriaxiality, *vonMisesStress, *maxPrincipalStress;
  double *damageParameterN, *damageParameterNP1, *brokenBondVolumeAveragedN;
  dataManager.getData(m_bondLevelEquivalentPlasticStrainFieldId, PeridigmField::STEP_N)->ExtractView(&equivalentPlasticStrainN);
  dataManager.getData(m_bondLevelEquivalentPlasticStrainFieldId, PeridigmField::STEP_NP1)->ExtractView(&equivalentPlasticStrainNP1);
  dataManager.getData(m_bondLevelStressTriaxialityFieldId, PeridigmField::STEP_NONE)->ExtractView(&stressTriaxiality);
  dataManager.getData(m_bondLevelVonMisesStressFieldId, PeridigmField::STEP_NONE)->ExtractView(&vonMisesStress);
  dataManager.getData(m_bondLevelMaxPrincipalStressFieldId, PeridigmField::STEP_NONE)->ExtractView(&maxPrincipalStress);
  dataManager.getData(m_bondLevelDamageParameterFieldId, PeridigmField::STEP_N)->ExtractView(&damageParameterN);
  dataManager.getData(m_bondLevelDamageParameterFieldId, PeridigmField::STEP_NP1)->ExtractView(&damageParameterNP1);
  dataManager.getData(m_brokenBondVolumeAveragedFieldId, PeridigmField::STEP_N)->ExtractView(&brokenBondVolumeAveragedN);

  double* damageParameterNP1Base = damageParameterNP1;

  for(int iID=0 ; iID<numOwnedPoints ; ++iID, ++damageN, ++damageNP1,
        ++flyingPointFlag, ++delta, ++undamagedWeightedVolume, coord+=3,
        ++brokenBondVolumeAveragedN, ++brokenBondVolumeAveragedNP1,
        ++connectivityAsymmetryNP1, ++connectivityAsymmetryBaseline)
  {
    if(*flyingPointFlag < 0.0){
      // damage = volume-average of broken bonds
      *damageNP1 = 0.0;
      *brokenBondVolumeAveragedNP1 = 0.0;
      *connectivityAsymmetryNP1 = 0.0;

      // Connectivity Severance accumulators (see header comment): the
      // volume/influence-weighted resultant of this point's SURVIVING bond
      // directions, built up alongside the existing per-bond loop below.
      double sumWeightedDirX = 0.0, sumWeightedDirY = 0.0, sumWeightedDirZ = 0.0, sumIntactWeight = 0.0;

      // Loop over the neighbors and compute bond damage
      numNeighbors = *neighborListPtr; neighborListPtr++;

      // Captured for the optional bond-cleanup-on-flying-point pass below,
      // which (if triggered) needs to revisit this same point's bonds after
      // the loop finishes. Both arrays must be fixed up: bondDamage is what
      // this step's force computation reads, but it gets recomputed fresh
      // every step from damageParameterNP1 (the persistent raw per-bond D
      // state carried forward as next step's damageParameterN), so leaving
      // the underlying D value untouched would let the ramp formula
      // silently undo the cleanup on the very next step.
      double* bondDamageStartForThisPoint = bondDamage;
      double* damageParameterNP1StartForThisPoint = damageParameterNP1;
      int numNeighborsForThisPoint = numNeighbors;

      for(int n=0; n<numNeighbors; n++, neighborListPtr++,
            bondDamage++, equivalentPlasticStrainN++,
            equivalentPlasticStrainNP1++, stressTriaxiality++,
            vonMisesStress++, maxPrincipalStress++,
            damageParameterN++, damageParameterNP1++){

        neighborIndex = *neighborListPtr;

        // D is a non-decreasing function
        // D_dot = d1 * <2*sigmaT/(3*(sigmaT - sigmaM))>^d2 * eqps_dot
        *damageParameterNP1 = *damageParameterN;

        // Damage model is evaluated before material model. The updated
        // values from the previous time step have been swapped between N
        // and NP1 states (same convention as the Johnson Cook model).
        deqps = *equivalentPlasticStrainN - *equivalentPlasticStrainNP1;

        sigmaT = *maxPrincipalStress;
        // hydrostatic stress recovered from the two fields the material
        // already exposes: triaxiality = sigmaM/vonMises => sigmaM = triaxiality*vonMises
        sigmaM = (*stressTriaxiality) * (*vonMisesStress);

        deltaDamage = 0.0;
        if(deqps > 0.0 && sigmaT > 0.0){
          // sigmaT (max principal) >= sigmaM (mean) always, with equality
          // only at a pure hydrostatic stress state; guard that singularity
          // with a relative (not absolute) floor so this works regardless
          // of the stress unit scale in play (same reasoning as the
          // relative-tolerance fix applied to the plasticity return-mapping
          // solver elsewhere in this code base).
          denom = std::max(sigmaT - sigmaM, 1.0e-6*sigmaT);
          ratio = 2.0*sigmaT / (3.0*denom);
          deltaDamage = m_damageParameter1 * std::pow(ratio, m_damageParameter2) * deqps;
        }

        if(deltaDamage > 0.0)
          *damageParameterNP1 += deltaDamage;

        if(*damageParameterNP1 >= 1.0){
          *damageParameterNP1 = 1.0;
        }

        // Fracture Plane Cutoff: once triggered (see scan above), force any
        // bond touching the plane band to full failure this step, checked
        // symmetrically from both endpoints (this point's own coordinate OR
        // its neighbor's) so the same physical bond is severed consistently
        // whichever of its two owning points processes it.
        if(planeTriggered){
          double thisY = *(coord+1);
          double neighborY = *(coordinates + 3*neighborIndex + 1);
          if(std::abs(thisY - m_fracturePlaneY) <= m_fracturePlaneHalfThickness ||
             std::abs(neighborY - m_fracturePlaneY) <= m_fracturePlaneHalfThickness){
            *damageParameterNP1 = 1.0;
          }
        }

        neighborVolume = jacobianDeterminant[neighborIndex] * volume[neighborIndex];
        neighborCoord = coordinates + 3*neighborIndex;

        deformedBondX = *(neighborCoord)   - *(coord);
        deformedBondY = *(neighborCoord+1) - *(coord+1);
        deformedBondZ = *(neighborCoord+2) - *(coord+2);
        deformedBondLength = sqrt(deformedBondX*deformedBondX +
                                  deformedBondY*deformedBondY +
                                  deformedBondZ*deformedBondZ);

        // Geometric Stretch Cutoff (opt-in, see header comment): a
        // lightweight approximation of semi-Lagrangian re-neighboring's
        // "drop a bond once its points have moved far enough apart"
        // direction. Computed fresh every step from CURRENT geometry, same
        // formula as the Critical Stretch damage model -- no dependence on
        // Brozzo's own accumulated, stress-history-driven D, so it directly
        // catches bonds that have geometrically separated without ever
        // triggering Brozzo's own damage law (the "patchy holdout" bonds
        // behind Broken_Bond_Volume_Averaged's observed plateau).
        if(m_geometricStretchCutoffEnabled){
          const double* thisModelCoord = modelCoordinates + 3*iID;
          const double* neighborModelCoord = modelCoordinates + 3*neighborIndex;
          double initialBondX = *(neighborModelCoord)   - *(thisModelCoord);
          double initialBondY = *(neighborModelCoord+1) - *(thisModelCoord+1);
          double initialBondZ = *(neighborModelCoord+2) - *(thisModelCoord+2);
          double initialBondLength = sqrt(initialBondX*initialBondX +
                                          initialBondY*initialBondY +
                                          initialBondZ*initialBondZ);
          double relativeExtension = (deformedBondLength - initialBondLength) / initialBondLength;
          if(relativeExtension > m_geometricCriticalStretch){
            *damageParameterNP1 = 1.0;
          }
        }

        if(*damageParameterNP1 > m_thresholdDamage){
          // Hard Cutoff: step function matching the original paper's own
          // damage law (omega_D = 1 if D < Dc, else 0) -- a bond snaps
          // directly from full strength to fully broken. Default (ramp):
          // strength degrades linearly as D climbs from Threshold Damage to
          // 1.0, the same mechanic used by the Johnson Cook damage model.
          // A bond forced to D=1 by the Fracture Plane Cutoff or Geometric
          // Stretch Cutoff above always lands at bondDamage=1.0 here
          // regardless of Hard Cutoff, since (1-thresholdDamage)/(1-
          // thresholdDamage) = 1.0 -- both mechanisms are genuine full
          // failures, not partial ramp points.
          *bondDamage = m_hardCutoff ? 1.0 : (*damageParameterNP1 - m_thresholdDamage) / (1.0 - m_thresholdDamage);
        }
        else{
          *bondDamage = 0.0;
        }

        omega = MATERIAL_EVALUATION::scalarInfluenceFunction(deformedBondLength, *delta);

        *damageNP1 += *damageParameterNP1 * omega / *undamagedWeightedVolume * neighborVolume;
        *brokenBondVolumeAveragedNP1 += *bondDamage * omega / *undamagedWeightedVolume * neighborVolume;

        // Connectivity Severance accumulation (see header comment): weight
        // this bond's contribution to the resultant direction by how
        // INTACT it still is (1-bondDamage), same omega/volume weighting
        // as everything else here. A bond already fully broken
        // (bondDamage=1) contributes nothing, exactly as it should --
        // only surviving bonds should pull the resultant in their
        // direction.
        if(deformedBondLength > 0.0){
          double intactWeight = (1.0 - *bondDamage) * omega * neighborVolume;
          sumIntactWeight += intactWeight;
          sumWeightedDirX += intactWeight * deformedBondX / deformedBondLength;
          sumWeightedDirY += intactWeight * deformedBondY / deformedBondLength;
          sumWeightedDirZ += intactWeight * deformedBondZ / deformedBondLength;
        }
      }

      // Connectivity Severance (opt-in, see header comment): 0 when
      // surviving bonds are still spread roughly symmetrically in all
      // directions relative to this point's own undamaged baseline
      // (healthy), approaching 1 as they become concentrated in one
      // direction only (lost connectivity to the other side). The
      // baseline subtraction is essential -- see header comment -- a
      // point near the specimen's free surface has a nonzero raw
      // resultant even with zero damage, purely from mesh geometry.
      if(sumIntactWeight > 0.0){
        double resultantMag = sqrt(sumWeightedDirX*sumWeightedDirX +
                                   sumWeightedDirY*sumWeightedDirY +
                                   sumWeightedDirZ*sumWeightedDirZ);
        double rawAsymmetry = resultantMag / sumIntactWeight;
        *connectivityAsymmetryNP1 = std::max(0.0, rawAsymmetry - *connectivityAsymmetryBaseline);
      }

      bool shouldGoFlying = (*brokenBondVolumeAveragedNP1 > m_criticalDamageToNeglectMaterialPoint) || // to prevent cases that only a couple of bonds are left, causing instability issues.
                            (m_connectivitySeveranceEnabled && *connectivityAsymmetryNP1 > m_connectivityAsymmetryThreshold);

      if(shouldGoFlying){
        *flyingPointFlag = 1.0;

        // Bond cleanup on flying point (opt-in, see header comment): this
        // point just got excluded from force computation for having lost
        // almost all its bond strength, but a handful of its bonds may not
        // individually have reached full failure yet. Finish them off now,
        // in this same step, so the point is actually fully disconnected
        // from its surviving neighbors rather than leaving a few
        // under-threshold bonds dangling indefinitely.
        if(m_bondCleanupOnFlyingPoint){
          for(int n=0; n<numNeighborsForThisPoint; n++){
            if(damageParameterNP1StartForThisPoint[n] < 1.0){
              // Rate-limited cleanup (see header comment): ramp this
              // lagging bond's own damage parameter by the configured
              // rate instead of snapping it straight to full failure.
              damageParameterNP1StartForThisPoint[n] = std::min(1.0, damageParameterNP1StartForThisPoint[n] + m_bondCleanupRate);
              if(damageParameterNP1StartForThisPoint[n] > m_thresholdDamage){
                bondDamageStartForThisPoint[n] = m_hardCutoff ? 1.0 : (damageParameterNP1StartForThisPoint[n] - m_thresholdDamage) / (1.0 - m_thresholdDamage);
              }
            }
          }
          if(m_bondCleanupRate >= 1.0){
            *brokenBondVolumeAveragedNP1 = 1.0;
            *damageNP1 = 1.0;
            *connectivityAsymmetryNP1 = 1.0;
          }
        }
      }
    }
    else{
      // for the visualization purposes
      *damageNP1 = *damageN;
      *brokenBondVolumeAveragedNP1 = *brokenBondVolumeAveragedN;
      *connectivityAsymmetryNP1 = 1.0;

      // adjust the neighborhood pointer for the next point
      numNeighbors = *neighborListPtr;
      neighborListPtr += numNeighbors+1;

      bondDamage += numNeighbors;
      equivalentPlasticStrainN += numNeighbors;
      equivalentPlasticStrainNP1 += numNeighbors;
      stressTriaxiality += numNeighbors;
      vonMisesStress += numNeighbors;
      maxPrincipalStress += numNeighbors;
      damageParameterN += numNeighbors;
      damageParameterNP1 += numNeighbors;
    }
  }

  // Symmetric Severance Propagation (opt-in, see header comment): every
  // point above decided Flying_Point_Flag using only its OWN bonds' damage
  // state. This second pass gives each point visibility into whether its
  // NEIGHBORS independently reached that same conclusion, and if so,
  // mirrors that decision onto this point's own copy of the connecting
  // bond -- closing the one-sided-bookkeeping gap described in the header
  // comment. Flying_Point_Flag is an ELEMENT field and is NOT among the
  // handful of fields the core solver auto-syncs every step (unlike
  // Coordinates/Displacement/Velocity), so an explicit sync is required
  // before ghost-owned neighbors' flags can be read correctly here.
  if(m_symmetricSeverancePropagationEnabled){

    dataManager.scatterToGhosts();

    const int* neighborListPtr2 = neighborhoodList;
    int bondOffset = 0;

    for(int iID=0 ; iID<numOwnedPoints ; ++iID){

      int numNeighbors2 = *neighborListPtr2; neighborListPtr2++;

      if(flyingPointFlagBase[iID] < 0.0){

        bool anyNewKill = false;
        for(int n=0; n<numNeighbors2; n++){
          int neighborIndex2 = neighborListPtr2[n];
          int idx = bondOffset + n;
          if(flyingPointFlagBase[neighborIndex2] > 0.0 && damageParameterNP1Base[idx] < 1.0){
            // Rate-limited mirroring (see header comment): instead of
            // snapping straight to full failure, add only the configured
            // rate's worth of damage this step, reusing the same
            // ramp/Hard-Cutoff formula ordinary Brozzo accumulation uses,
            // so a bond takes ~1/rate steps of sustained neighbor exposure
            // to fully die rather than one.
            damageParameterNP1Base[idx] = std::min(1.0, damageParameterNP1Base[idx] + m_symmetricSeverancePropagationRate);
            if(damageParameterNP1Base[idx] > m_thresholdDamage){
              bondDamageBase[idx] = m_hardCutoff ? 1.0 : (damageParameterNP1Base[idx] - m_thresholdDamage) / (1.0 - m_thresholdDamage);
            }
            anyNewKill = true;
          }
        }

        if(anyNewKill){
          // Recompute this point's aggregates from the now-updated
          // bondDamage, exactly mirroring the formulas in the main loop
          // above, then re-check whether it should now go flying too --
          // this is what allows a genuine multi-step cascade to emerge.
          double newDamage = 0.0, newBrokenBondVolumeAveraged = 0.0;
          double sumWeightedDirX2 = 0.0, sumWeightedDirY2 = 0.0, sumWeightedDirZ2 = 0.0, sumIntactWeight2 = 0.0;
          const double* myCoord2 = coordinates + 3*iID;
          double myDelta2 = horizon[iID];

          for(int n=0; n<numNeighbors2; n++){
            int neighborIndex2 = neighborListPtr2[n];
            int idx = bondOffset + n;
            double bd = bondDamageBase[idx];
            double nVol2 = jacobianDeterminant[neighborIndex2] * volume[neighborIndex2];
            const double* nCoord2 = coordinates + 3*neighborIndex2;
            double dx2 = nCoord2[0] - myCoord2[0];
            double dy2 = nCoord2[1] - myCoord2[1];
            double dz2 = nCoord2[2] - myCoord2[2];
            double len2 = sqrt(dx2*dx2 + dy2*dy2 + dz2*dz2);
            double omega2 = MATERIAL_EVALUATION::scalarInfluenceFunction(len2, myDelta2);

            newDamage += damageParameterNP1Base[idx] * omega2 / undamagedWeightedVolumeBase[iID] * nVol2;
            newBrokenBondVolumeAveraged += bd * omega2 / undamagedWeightedVolumeBase[iID] * nVol2;

            if(len2 > 0.0){
              double intactWeight2 = (1.0 - bd) * omega2 * nVol2;
              sumIntactWeight2 += intactWeight2;
              sumWeightedDirX2 += intactWeight2 * dx2 / len2;
              sumWeightedDirY2 += intactWeight2 * dy2 / len2;
              sumWeightedDirZ2 += intactWeight2 * dz2 / len2;
            }
          }

          damageNP1Base[iID] = newDamage;
          brokenBondVolumeAveragedNP1Base[iID] = newBrokenBondVolumeAveraged;

          double newAsymmetry = 0.0;
          if(sumIntactWeight2 > 0.0){
            double resultantMag2 = sqrt(sumWeightedDirX2*sumWeightedDirX2 +
                                        sumWeightedDirY2*sumWeightedDirY2 +
                                        sumWeightedDirZ2*sumWeightedDirZ2);
            newAsymmetry = std::max(0.0, resultantMag2 / sumIntactWeight2 - connectivityAsymmetryBaselineBase[iID]);
          }
          connectivityAsymmetryNP1Base[iID] = newAsymmetry;

          bool shouldGoFlyingNow = (newBrokenBondVolumeAveraged > m_criticalDamageToNeglectMaterialPoint) ||
                                    (m_connectivitySeveranceEnabled && newAsymmetry > m_connectivityAsymmetryThreshold);

          if(shouldGoFlyingNow){
            flyingPointFlagBase[iID] = 1.0;
            if(m_bondCleanupOnFlyingPoint){
              for(int n=0; n<numNeighbors2; n++){
                int cleanupIdx = bondOffset+n;
                if(damageParameterNP1Base[cleanupIdx] < 1.0){
                  // Rate-limited cleanup (see header comment), same as the
                  // main loop's equivalent block above.
                  damageParameterNP1Base[cleanupIdx] = std::min(1.0, damageParameterNP1Base[cleanupIdx] + m_bondCleanupRate);
                  if(damageParameterNP1Base[cleanupIdx] > m_thresholdDamage){
                    bondDamageBase[cleanupIdx] = m_hardCutoff ? 1.0 : (damageParameterNP1Base[cleanupIdx] - m_thresholdDamage) / (1.0 - m_thresholdDamage);
                  }
                }
              }
              if(m_bondCleanupRate >= 1.0){
                brokenBondVolumeAveragedNP1Base[iID] = 1.0;
                damageNP1Base[iID] = 1.0;
                connectivityAsymmetryNP1Base[iID] = 1.0;
              }
            }
          }
        }
      }

      bondOffset += numNeighbors2;
      neighborListPtr2 += numNeighbors2;
    }
  }
}
