/*! \file Peridigm_BrozzoDamageModel.hpp */

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

#ifndef PERIDIGM_BROZZODAMAGEMODEL_HPP
#define PERIDIGM_BROZZODAMAGEMODEL_HPP

#include "Peridigm_DamageModel.hpp"
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Epetra_Vector.h>
#include <Epetra_Map.h>

namespace PeridigmNS {

  //! Brozzo/Wellman-style ductile damage model (Behzadinasab & Foster 2020,
  //! "Revisiting the third Sandia Fracture Challenge"):
  //!   D_dot = d1 * <2*sigmaT / (3*(sigmaT - sigmaM))>^d2 * eqps_dot,  if D < 1
  //! where sigmaT is the bond-level maximum principal stress, sigmaM is the
  //! bond-level hydrostatic stress, and <.> is a Heaviside on sigmaT (damage
  //! only accumulates in tension). By default, bond strength is ramped down
  //! linearly as D climbs from Threshold Damage to 1.0, the same mechanic
  //! used by the Johnson Cook damage model in this code base. Setting the
  //! optional "Hard Cutoff" parameter to true instead applies a step-function
  //! cutoff matching the original paper's own damage law (omega_D = 1 if
  //! D < Dc, else 0): a bond goes from full strength directly to fully
  //! broken the instant its own accumulated damage D crosses Threshold
  //! Damage, rather than degrading gradually over the ramp.
  //!
  //! Optional "Fracture Plane Cutoff" mechanism (enabled via "Enable
  //! Fracture Plane Cutoff: true"): once any point within a fixed geometric
  //! band ("Fracture Plane Y" +/- "Fracture Plane Half Thickness") reaches
  //! "Fracture Trigger Damage", every bond touching that band (checked from
  //! both endpoints, so severance is symmetric/consistent for both points
  //! sharing a bond) is forced to full failure (D=1) in a single step --
  //! a genuine, complete cross-section severance rather than relying on
  //! individual bonds to each separately, gradually accumulate to failure.
  //! Each rank scans only its own owned points, then synchronizes the
  //! trigger across all ranks via an Epetra_Comm::MaxAll reduction, so this
  //! is correct for any number of ranks, including a fracture plane split
  //! across a processor boundary. Bonds crossing a processor boundary are
  //! handled correctly too, since Peridigm's neighbor-list infrastructure
  //! already keeps each rank's neighbors' coordinates synced via ghost
  //! exchange regardless of which rank owns them.
  //!
  //! Optional "Enable Bond Cleanup On Flying Point" mechanism (default
  //! false, so existing calibrated results are unaffected unless explicitly
  //! opted in): a point already accumulating damage on most of its bonds
  //! but never quite finishing (a few lagging survivor bonds) is exactly
  //! the mechanism that leaves a specimen's cross-section "patchy" instead
  //! of genuinely severed -- see CLAUDE.md's "Root cause identified
  //! (2026-09-30)" discussion. This mechanism piggybacks on the existing
  //! "Critical Damage To Neglect Material Point" flying-point trigger: at
  //! the exact moment a point's bond-weighted broken fraction first crosses
  //! that threshold and it gets marked a flying point (no longer
  //! contributing force), its handful of remaining not-yet-fully-broken
  //! bonds are also forced to full failure (Bond_Damage = 1.0) in that same
  //! step. This is local (per-point, no new geometric trigger or global
  //! synchronization needed) and gradual (each point crosses its own
  //! threshold at its own time as damage naturally evolves there, so
  //! different points complete at different steps, producing an emergent
  //! propagating-looking crack front) -- unlike the Fracture Plane Cutoff
  //! above, which forces an entire geometric plane to sever in one step
  //! regardless of each point's actual local damage state.
  //!
  //! Optional "Enable Geometric Stretch Cutoff" mechanism (default false):
  //! a lightweight approximation of the "Revisiting" paper's semi-Lagrangian
  //! re-neighboring (Behzadinasab & Foster 2020b), scoped to just the
  //! "drop a bond once its points have moved far enough apart" direction
  //! (not the "form new bonds between points that moved closer together"
  //! direction, which would require rebuilding Peridigm's actual neighbor
  //! search/ghost-exchange infrastructure -- out of scope here). Every
  //! step, each bond's CURRENT relative stretch (recomputed fresh from
  //! current geometry, same formula as the Critical Stretch damage model --
  //! no accumulated/path-dependent state) is compared against "Geometric
  //! Critical Stretch"; if exceeded, the bond is forced to full failure
  //! (D=1) regardless of what Brozzo's own accumulated damage parameter
  //! says. This targets the "patchy holdout" bonds directly: bonds that
  //! have geometrically opened up a lot (because the crack is genuinely
  //! separating there) but whose stress state doesn't happen to drive
  //! Brozzo's own D_dot (see CLAUDE.md's flying-point diagnostic finding --
  //! Broken_Bond_Volume_Averaged plateaus around 0.855 under the
  //! unmodified damage law and never reaches the flying-point threshold).
  class BrozzoDamageModel : public DamageModel{

  public:

    //! Standard constructor.
    BrozzoDamageModel(const Teuchos::ParameterList& params);

    //! Destructor.
    virtual ~BrozzoDamageModel();

    //! Return name of the model.
    virtual std::string Name() const { return("Brozzo"); }

    //! Returns a vector of field IDs corresponding to the variables associated with the model.
    virtual std::vector<int> FieldIds() const { return m_fieldIds; }

    //! Initialize the damage model.
    virtual void
    initialize(const double dt,
               const int numOwnedPoints,
               const int* ownedIDs,
               const int* neighborhoodList,
               PeridigmNS::DataManager& dataManager) const ;

    //! Evaluate the damage
    virtual void
    computeDamage(const double dt,
                  const int numOwnedPoints,
                  const int* ownedIDs,
                  const int* neighborhoodList,
                  PeridigmNS::DataManager& dataManager) const ;

  protected:

    double m_thresholdDamage;
    double m_damageParameter1;
    double m_damageParameter2;
    double m_criticalDamageToNeglectMaterialPoint;
    bool m_hardCutoff;
    bool m_fracturePlaneEnabled;
    double m_fracturePlaneY;
    double m_fracturePlaneHalfThickness;
    double m_fractureTriggerDamage;
    bool m_bondCleanupOnFlyingPoint;
    bool m_geometricStretchCutoffEnabled;
    double m_geometricCriticalStretch;

    // field ids for all relevant data
    std::vector<int> m_fieldIds;
    int m_damageFieldId;
    int m_flyingPointFlagFieldId;
    int m_bondDamageFieldId;
    int m_bondLevelEquivalentPlasticStrainFieldId;
    int m_bondLevelStressTriaxialityFieldId;
    int m_bondLevelVonMisesStressFieldId;
    int m_bondLevelMaxPrincipalStressFieldId;
    int m_bondLevelDamageParameterFieldId;
    int m_brokenBondVolumeAveragedFieldId;
    int m_horizonFieldId;
    int m_volumeFieldId;
    int m_coordinatesFieldId;
    int m_modelCoordinatesFieldId;
    int m_jacobianDeterminantFieldId;
    int m_undamagedWeightedVolumeFieldId;
 };

}

#endif // PERIDIGM_BROZZODAMAGEMODEL_HPP
