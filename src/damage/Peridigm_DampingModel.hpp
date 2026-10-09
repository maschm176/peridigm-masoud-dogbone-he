/*! \file Peridigm_DampingModel.hpp */

#ifndef PERIDIGM_DAMPINGMODEL_HPP
#define PERIDIGM_DAMPINGMODEL_HPP

#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Epetra_Vector.h>
#include <Epetra_Map.h>
#include "Peridigm_DataManager.hpp"

namespace PeridigmNS {

  //! Base class defining the Peridigm damping model interface. A damping
  //! model runs after the material model's force computation for a block
  //! and adds an additional, purely numerical dissipative contribution
  //! directly into the Force_Density field -- it does not represent a real
  //! material property, just a way to remove kinetic energy that the
  //! underlying material/damage formulation has no mechanism to absorb.
  class DampingModel{

  public:

    //! Standard constructor.
    DampingModel(const Teuchos::ParameterList & params){}

    //! Destructor.
    virtual ~DampingModel(){}

    //! Return name of the model.
    virtual std::string Name() const = 0;

    //! Returns a vector of field IDs corresponding to the variables associated with the model.
    virtual std::vector<int> FieldIds() const = 0;

    //! Evaluate the damping force and add it into Force_Density.
    virtual void
    computeDamping(const double dt,
                   const int numOwnedPoints,
                   const int* ownedIDs,
                   const int* neighborhoodList,
                   PeridigmNS::DataManager& dataManager) const = 0;

  private:

    //! Default constructor with no arguments, private to prevent use.
    DampingModel(){}
  };
}

#endif // PERIDIGM_DAMPINGMODEL_HPP
