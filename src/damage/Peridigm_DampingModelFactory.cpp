/*! \file Peridigm_DampingModelFactory.cpp */

#include <Teuchos_Assert.hpp>
#include "Peridigm_DampingModelFactory.hpp"
#include "Peridigm_BondLevelMicroDampingModel.hpp"
#include "Peridigm_BondLevelArtificialViscosityDampingModel.hpp"

using namespace std;

Teuchos::RCP<PeridigmNS::DampingModel>
PeridigmNS::DampingModelFactory::create(const Teuchos::ParameterList& dampingModelParams)
{
  Teuchos::RCP<PeridigmNS::DampingModel> dampingModel;

  const string& dampingModelName = dampingModelParams.get<string>("Damping Model");

  if(dampingModelName == "Bond-Level Micro-Damping")
    dampingModel = Teuchos::rcp( new BondLevelMicroDampingModel(dampingModelParams) );
  else if(dampingModelName == "Bond-Level Artificial Viscosity")
    dampingModel = Teuchos::rcp( new BondLevelArtificialViscosityDampingModel(dampingModelParams) );
  else {
    string invalidDampingModel("\n**** Unrecognized damping model type: ");
    invalidDampingModel += dampingModelName;
    invalidDampingModel += ", must be \"Bond-Level Micro-Damping\" or \"Bond-Level Artificial Viscosity\".\n";
    TEUCHOS_TEST_FOR_EXCEPT_MSG(true, invalidDampingModel);
  }

  return dampingModel;
}
