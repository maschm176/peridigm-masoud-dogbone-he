/*! \file Peridigm_BondLevelMicroDampingModel.hpp */

#ifndef PERIDIGM_BONDLEVELMICRODAMPINGMODEL_HPP
#define PERIDIGM_BONDLEVELMICRODAMPINGMODEL_HPP

#include "Peridigm_DampingModel.hpp"
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>

namespace PeridigmNS {

  //! Bond-level viscous micro-damping: for every bonded pair (i,j) in the
  //! material's own neighborhood (not a separate contact/proximity list),
  //! adds a force proportional to the RELATIVE velocity between i and j,
  //!   f_i -= c * (v_i - v_j) * Volume_j
  //! accumulated directly into Force_Density. Since it only depends on
  //! relative velocity, this is zero under pure rigid-body translation or
  //! rotation (every bonded pair moves together, v_i - v_j = 0) -- it only
  //! dissipates energy from genuine internal/relative motion (e.g. the
  //! post-fracture ringing of a ligament that has nothing else to absorb
  //! the energy released by sudden local stiffness loss), unlike a global
  //! velocity-proportional (Rayleigh-style) damping, which would also
  //! resist the overall prescribed stretching motion and likely distort an
  //! already-calibrated pre-fracture fit.
  //!
  //! If a "Bond_Damage" field exists in this run (i.e. some block has an
  //! active damage model), the contribution from each bond is scaled by
  //! (1 - Bond_Damage) so damping fades out at the same rate bond stiffness
  //! does -- a fully-failed bond contributes no damping, same as it
  //! contributes no stress, avoiding adding artificial coupling across
  //! material that the damage model has already broken. If no damage model
  //! is present anywhere in the run, damping simply applies at full
  //! strength to every bond.
  class BondLevelMicroDampingModel : public DampingModel{

  public:

    //! Standard constructor.
    BondLevelMicroDampingModel(const Teuchos::ParameterList& params);

    //! Destructor.
    virtual ~BondLevelMicroDampingModel();

    //! Return name of the model.
    virtual std::string Name() const { return("Bond-Level Micro-Damping"); }

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

    double m_dampingCoefficient;
    bool m_useBondDamage;

    std::vector<int> m_fieldIds;
    int m_volumeFieldId;
    int m_velocityFieldId;
    int m_forceDensityFieldId;
    int m_bondDamageFieldId;
 };

}

#endif // PERIDIGM_BONDLEVELMICRODAMPINGMODEL_HPP
