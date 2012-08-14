// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_PGPFACTORY_DECL_HPP_
#define MUELU_PGPFACTORY_DECL_HPP_

#include <Xpetra_Vector_fwd.hpp>
#include <Xpetra_VectorFactory_fwd.hpp>
#include <Xpetra_Operator_fwd.hpp>
#include <Xpetra_Import_fwd.hpp>
#include <Xpetra_ImportFactory_fwd.hpp>
#include <Xpetra_Export_fwd.hpp>
#include <Xpetra_ExportFactory_fwd.hpp>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_PFactory.hpp"
#include "MueLu_TentativePFactory_fwd.hpp"
#include "MueLu_SingleLevelFactoryBase_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

  /* Options defining how to pick-up the next root node in the local aggregation procedure */
  enum MinimizationNorm {
    ANORM = 0, /* A norm   */
    L2NORM = 1, /* L2 norm */
    DINVANORM  = 2 /* Dinv A norm */
  };
  
  /*!
    @class PgPFactory class.
    @brief Factory for building Petrov-Galerkin Smoothed Aggregation prolongators.
    @ingroup MueLuTransferClasses
  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class PgPFactory : public PFactory {
#undef MUELU_PGPFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors.
    //@{

    /*! @brief Constructor.
      User can supply a factory for generating the tentative prolongator.
    */
    PgPFactory(RCP</*PFactory*/const FactoryBase> InitialPFact = Teuchos::null, RCP< const FactoryBase /* SingleLevelFactoryBase*/> AFact = Teuchos::null);

    //! Destructor.
    virtual ~PgPFactory();

    //@}

    //! @name Set methods.
    //@{

    //! Change view of diagonal.
    void SetDiagonalView(std::string const& diagView);

    //! Set minimization mode (L2NORM for cheapest, ANORM more expensive, DINVANORM = default)
    void SetMinimizationMode(MinimizationNorm minnorm);

    //! return minimization mode
    MinimizationNorm GetMinimizationMode();
    //@}

    //! @name Get methods.
    //@{

    //! Returns current view of diagonal.
    std::string GetDiagonalView();

    //@}

    //! Input
    //@{

    void DeclareInput(Level &fineLevel, Level &coarseLevel) const;
    //@}

    //! @name Build methods.
    //@{

    /*!
      @brief Build method.

      Builds smoothed aggregation prolongator and returns it in <tt>coarseLevel</tt>.
    */
    void Build(Level& fineLevel, Level &coarseLevel) const;

    void BuildP(Level &fineLevel, Level &coarseLevel) const;

    //@}

    void ReUseDampingParameters(bool bReuse);

  private:

    void MultiplySelfAll(const RCP<Operator>& Op, Teuchos::RCP<Xpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> >& InnerProdVec) const;

    void MultiplyAll(const RCP<Operator>& left, const RCP<Operator>& right, Teuchos::RCP<Xpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> >& InnerProdVec) const;

    void ComputeRowBasedOmega(Level& fineLevel, Level &coarseLevel, const RCP<Operator>& A, const RCP<Operator>& P0, const RCP<Operator>& DinvAP0, RCP<Xpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> > & RowBasedOmega) const;

  private:

    //! Input factories
    //RCP<PFactory> initialPFact_;        //! Ptentative Factory
    RCP<const FactoryBase> initialPFact_;        //! Ptentative Factory
    //RCP<SingleLevelFactoryBase> AFact_; //! A Factory
    RCP<const FactoryBase> AFact_; //! A Factory

    //! Factory parameters
    std::string diagonalView_;

    //! minimization norm
    MinimizationNorm min_norm_;

    //! flag: reuse row based omegas from prolongator for restriction operator
    bool bReUseRowBasedOmegas_;
  };

} //namespace MueLu

#define MUELU_PGPFACTORY_SHORT
#endif /* MUELU_PGPFACTORY_DECL_HPP_ */
