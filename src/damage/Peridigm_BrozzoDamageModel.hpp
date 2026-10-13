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
  //!
  //! Optional "Enable Connectivity Severance" mechanism (default false):
  //! replaces/supplements the existing "Critical Damage To Neglect
  //! Material Point" trigger (a volume-weighted AVERAGE of ALL a point's
  //! bond-damage values, which empirically plateaus around 0.7-0.86 and
  //! essentially never reaches its 0.95 threshold -- see CLAUDE.md's
  //! flying-point diagnostic and geometric-stretch sweep findings) with a
  //! DIRECTIONAL criterion. A point sitting on a crack surface has roughly
  //! half its 3D neighborhood on the far side of the crack (bonds that
  //! should fail) and half on the near side (bonds to same-side neighbors,
  //! which never need to fail) -- so no scalar fraction-of-all-bonds
  //! measure can ever reach anywhere near 1.0 for genuine across-the-crack
  //! severance, regardless of how aggressively any damage/stretch
  //! criterion is tuned. Instead, every step, this computes the
  //! volume/influence-weighted resultant (vector sum, then magnitude) of
  //! each point's SURVIVING (not-yet-fully-broken) bond directions,
  //! normalized by their total weight -- 0 for bonds still spread roughly
  //! symmetrically in all directions (healthy), approaching 1 as survivors
  //! become concentrated in one direction only (lost connectivity to the
  //! other side specifically, independent of how many same-side bonds
  //! remain perfectly intact). Exposed as the "Connectivity_Asymmetry"
  //! output field. If "Connectivity Asymmetry Threshold" is exceeded, the
  //! point is marked a flying point exactly like the existing trigger
  //! (OR'd together, not a replacement in the code path -- the old trigger
  //! is harmless/inert at the thresholds tested so far), and the Bond
  //! Cleanup On Flying Point mechanism above (if also enabled) finishes
  //! off its remaining bonds the same way.
  //!
  //! IMPORTANT correction found empirically: the raw resultant asymmetry
  //! described above is NOT zero for a healthy, undamaged point near a
  //! free surface -- a point within one horizon of the specimen's outer
  //! boundary has no material beyond it, so its bond family is inherently
  //! lopsided even with zero damage (confirmed directly: ~82% of all
  //! points showed raw asymmetry > 0.1 at frame 0, before any loading at
  //! all, spanning the specimen's entire length, not just the grip ends --
  //! this dogbone's horizon is evidently large relative to its
  //! cross-sectional radius). So "Connectivity_Asymmetry" is actually
  //! defined as the INCREASE above a per-point reference baseline (see
  //! "Connectivity_Asymmetry_Baseline" below), not the raw resultant
  //! magnitude -- otherwise the trigger would fire everywhere immediately,
  //! dominated by surface geometry rather than damage.
  //!
  //! Optional "Enable Symmetric Severance Propagation" mechanism (default
  //! false): the "Stage 1" semi-Lagrangian fix for a structural gap found
  //! empirically in Connectivity Severance -- every bond-damage quantity
  //! above (Bond_Damage, Flying_Point_Flag, etc.) is tracked independently
  //! by EACH of the two points sharing a bond (the same asymmetric
  //! two-sided bookkeeping used by the whole state-based correspondence
  //! family, including BondAssociatedCorrespondenceMaterial). When point i
  //! decides it's severed and Bond Cleanup forces its own remaining bonds
  //! to full failure, point j on the other end of one of those bonds never
  //! has ITS OWN copy of that bond touched -- j still sees i as a fully
  //! intact neighbor. Empirically (see CLAUDE.md) this meant a severed
  //! point's status never propagated to its neighbors: the flying-point
  //! count jumped once at fracture onset and then stayed completely flat
  //! for hundreds of subsequent steps, with no cascading crack front.
  //! This mechanism closes that gap: each step, after every point's own
  //! Flying_Point_Flag has been decided from its own local bond state (the
  //! existing logic, unchanged), the flag is synchronized across all
  //! processors (DataManager::scatterToGhosts(), the same mechanism
  //! Peridigm already uses before rebalancing -- Flying_Point_Flag is an
  //! ELEMENT field and is NOT one of the handful of fields the core solver
  //! auto-syncs every step, unlike Coordinates/Displacement/Velocity, so
  //! this explicit sync is required for correctness on more than one
  //! rank). A second pass then gives every point visibility into whether
  //! each of ITS OWN neighbors independently decided it was severed: if
  //! so, that specific bond is forced to full failure from THIS point's
  //! own perspective too, mirroring the neighbor's decision rather than
  //! requiring this point to separately reach the same conclusion on its
  //! own. If enough of a point's bonds die this way that ITS OWN
  //! aggregate severance criterion newly crosses threshold as a result,
  //! it becomes flying too (and triggers Bond Cleanup, if enabled) in the
  //! same step -- one bond-hop of propagation per step, so a genuine
  //! multi-step cascading crack front can emerge, which the unmodified
  //! one-sided bookkeeping could never produce.
  //!
  //! IMPORTANT finding from the first test of the above (see CLAUDE.md,
  //! 2026-10-12): with an instant kill ("mirror the neighbor's death in
  //! one step"), propagation doesn't look gradual in practice, because the
  //! explicit integrator's real timestep is tiny relative to the output
  //! interval (here, ~20 actual timesteps per output frame) -- that's
  //! enough chain-reaction depth for an AVALANCHE (~20% of the whole
  //! specimen, 2426 of 12025 points) to occur within about 10 output
  //! frames of fracture onset, then freeze completely, rather than a
  //! visible multi-frame propagating front. "Symmetric Severance
  //! Propagation Rate" (default 1.0 = instant, matching that original
  //! behavior if left unset) throttles this: instead of snapping a mirrored
  //! bond straight to full failure, each step adds only this much to the
  //! bond's own accumulated damage parameter (reusing the existing
  //! ramp/Hard-Cutoff machinery exactly as if it were ordinary Brozzo
  //! accumulation), so a bond takes roughly 1/rate steps of sustained
  //! neighbor exposure to fully die instead of one. This directly tests
  //! whether the avalanche is a rate problem (fixable by slowing the
  //! per-bond propagation increment) or something more fundamental.
  //!
  //! Result (2026-10-12): throttling the mirror-kill alone only delayed
  //! the avalanche's onset by about 15 output frames -- it still saturated
  //! at the identical final count (2426/12025 points). "Bond Cleanup
  //! Rate" (default 1.0 = instant, matching the original behavior)
  //! throttles the OTHER instant action in this chain: once a point's own
  //! aggregate criterion crosses threshold (which can still happen
  //! quickly, since many neck points already sit close to threshold from
  //! real, non-propagation Brozzo accumulation), Bond Cleanup finishes off
  //! ALL of that point's remaining bonds in a single step. This ramps that
  //! action too, at the same per-step rate, instead of snapping to 1.0.
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
    bool m_connectivitySeveranceEnabled;
    double m_connectivityAsymmetryThreshold;
    bool m_symmetricSeverancePropagationEnabled;
    double m_symmetricSeverancePropagationRate;
    double m_bondCleanupRate;

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
    int m_connectivityAsymmetryFieldId;
    int m_connectivityAsymmetryBaselineFieldId;
    int m_horizonFieldId;
    int m_volumeFieldId;
    int m_coordinatesFieldId;
    int m_modelCoordinatesFieldId;
    int m_jacobianDeterminantFieldId;
    int m_undamagedWeightedVolumeFieldId;
 };

}

#endif // PERIDIGM_BROZZODAMAGEMODEL_HPP
