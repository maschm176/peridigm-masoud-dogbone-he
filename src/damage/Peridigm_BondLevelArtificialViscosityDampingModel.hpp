/*! \file Peridigm_BondLevelArtificialViscosityDampingModel.hpp */

#ifndef PERIDIGM_BONDLEVELARTIFICIALVISCOSITYDAMPINGMODEL_HPP
#define PERIDIGM_BONDLEVELARTIFICIALVISCOSITYDAMPINGMODEL_HPP

#include "Peridigm_DampingModel.hpp"
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>

namespace PeridigmNS {

  //! Bond-level quadratic artificial viscosity -- the peridynamic analog of
  //! von Neumann-Richtmyer artificial viscosity, the standard shock-ringing
  //! suppression technique used in explicit hydrocodes. For every bonded
  //! pair (i,j), computes the bond's own stretch rate (rate of elongation
  //! along its current direction),
  //!   sdot = (v_j - v_i) . xihat
  //! and adds a force opposing that stretch/compression rate, QUADRATIC in
  //! magnitude rather than linear:
  //!   f_i -= c * sdot * |sdot| * xihat * Volume_j
  //! This differs structurally from Bond-Level Micro-Damping (linear in
  //! relative velocity, f_i -= c*(v_i-v_j)), which was found to have no
  //! usable coefficient window: too weak to matter at ordinary deformation
  //! rates, but by the time it was strong enough to meaningfully act during
  //! the brief post-fracture oscillation, it was already too fast for the
  //! explicit (Verlet) integrator to resolve as true dissipation (see
  //! CLAUDE.md, "Bond-Level Micro-Damping" section). A term quadratic in
  //! sdot is self-limiting in a different way: it is negligible for the
  //! small, smooth stretch rates of ordinary loading (including the mild
  //! elastic stress-wave noise already observed in every run), but grows
  //! rapidly for the large, fast-reversing stretch rates that characterize
  //! the actual ringing -- giving it a real chance at a usable window
  //! without requiring a semi-implicit integrator change.
  //!
  //! Like Bond-Level Micro-Damping, only depends on RELATIVE velocity (zero
  //! under rigid-body translation/rotation), and fades out via
  //! (1 - Bond_Damage) if a damage model is present, so a fully-failed bond
  //! contributes neither stress nor artificial viscosity.
  class BondLevelArtificialViscosityDampingModel : public DampingModel{

  public:

    //! Standard constructor.
    BondLevelArtificialViscosityDampingModel(const Teuchos::ParameterList& params);

    //! Destructor.
    virtual ~BondLevelArtificialViscosityDampingModel();

    //! Return name of the model.
    virtual std::string Name() const { return("Bond-Level Artificial Viscosity"); }

    //! Returns a vector of field IDs corresponding to the variables associated with the model.
    virtual std::vector<int> FieldIds() const { return m_fieldIds; }

    //! Evaluate the damping force and add it into Force_Density.
    virtual void
    computeDamping(const double dt,
                   const int numOwnedPoints,
                   const int* ownedIDs,
                   const int* neighborhoodList,
                   PeridigmNS::DataManager& dataManager) const ;

  protected:

    double m_viscosityCoefficient;
    bool m_useBondDamage;

    std::vector<int> m_fieldIds;
    int m_volumeFieldId;
    int m_velocityFieldId;
    int m_coordinatesFieldId;
    int m_forceDensityFieldId;
    int m_bondDamageFieldId;
 };

}

#endif // PERIDIGM_BONDLEVELARTIFICIALVISCOSITYDAMPINGMODEL_HPP
