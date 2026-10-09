/*! \file Peridigm_DampingModelFactory.hpp */

#ifndef PERIDIGM_DAMPINGMODELFACTORY_HPP
#define PERIDIGM_DAMPINGMODELFACTORY_HPP

#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include "Peridigm_DampingModel.hpp"

namespace PeridigmNS {

  class DampingModelFactory {
  public:
    DampingModelFactory(){}
    Teuchos::RCP<PeridigmNS::DampingModel> create(const Teuchos::ParameterList& dampingModelParams);
  };
}

#endif // PERIDIGM_DAMPINGMODELFACTORY_HPP
