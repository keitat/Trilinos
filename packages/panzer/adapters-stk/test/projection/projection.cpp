// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
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
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

// Panzer STK
#include "PanzerAdaptersSTK_config.hpp"
#include "Panzer_STK_Interface.hpp"
#include "Panzer_STK_SquareQuadMeshFactory.hpp"
#include "Panzer_STK_CubeHexMeshFactory.hpp"
#include "Panzer_STK_WorksetFactory.hpp"
#include "Panzer_STKConnManager.hpp"

// Panzer
#include "Panzer_Workset.hpp"
#include "Panzer_WorksetDescriptor.hpp"
#include "Panzer_WorksetNeeds.hpp"
#include "Panzer_WorksetContainer.hpp"
#include "Panzer_IntegrationDescriptor.hpp"
#include "Panzer_BasisDescriptor.hpp"
#include "Panzer_Evaluator_DomainInterface.hpp"
#include "Panzer_Traits.hpp"
#include "Panzer_Evaluator_WithBaseImpl.hpp"
#include "Panzer_IntrepidBasisFactory.hpp"
#include "Panzer_L2Projection.hpp"
#include "Panzer_DOFManager.hpp"
#include "Panzer_BlockedDOFManager.hpp"

#include "Tpetra_Vector.hpp"
#include "Tpetra_CrsMatrix.hpp"

// Teuchos
#include "Teuchos_DefaultMpiComm.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "Teuchos_UnitTestHarness.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "Teuchos_StackedTimer.hpp"

// Solver
#include "BelosSolverFactory.hpp"
#include "BelosMultiVecTraits_Tpetra.hpp"
#include "BelosOperatorTraits_Tpetra.hpp"
#include "BelosTpetraAdapter.hpp"
#include "BelosLinearProblem.hpp"

#include <memory>
#include <vector>
#include <string>
#include <cmath>

TEUCHOS_UNIT_TEST(l2_projection, dof_manager)
{
  using namespace Teuchos;
  using namespace panzer;
  using namespace panzer_stk;

  RCP<MpiComm<int>> comm = rcp(new MpiComm<int>(MPI_COMM_WORLD));

  auto timer = Teuchos::TimeMonitor::getStackedTimer();
  timer->start("Total Time");

  const int myRank = comm->getRank();
  const int numProcs = comm->getSize();
  const int numXElements = 2;
  TEUCHOS_ASSERT(numXElements >= numProcs);

  RCP<panzer_stk::STK_Interface> mesh;
  {
    PANZER_FUNC_TIME_MONITOR("L2Projection: mesh construction");
    Teuchos::RCP<Teuchos::ParameterList> pl = rcp(new Teuchos::ParameterList);
    pl->set("X Blocks",2);
    pl->set("Y Blocks",1);
    pl->set("Z Blocks",1);
    pl->set("X Elements",numXElements);
    pl->set("Y Elements",2);
    pl->set("Z Elements",1);
    pl->set("X Procs",numProcs);
    pl->set("Y Procs",1);
    pl->set("Z Procs",1);
    pl->set("X0",0.0);
    pl->set("Y0",0.0);
    pl->set("Z0",0.0);
    pl->set("Xf",1.0);
    pl->set("Yf",1.0);
    pl->set("Zf",1.0);
    panzer_stk::CubeHexMeshFactory factory;
    factory.setParameterList(pl);
    mesh = factory.buildUncommitedMesh(MPI_COMM_WORLD);
    factory.completeMeshConstruction(*mesh,MPI_COMM_WORLD);
  }

  // Build Worksets

  const int basisOrder = 1;
  BasisDescriptor hgradBD(basisOrder,"HGrad");
  BasisDescriptor hcurlBD(basisOrder,"HCurl");
  BasisDescriptor hdivBD(basisOrder,"HDiv");

  const int intOrder = 2;
  IntegrationDescriptor integrationDescriptor(intOrder,IntegrationDescriptor::VOLUME);

  WorksetNeeds worksetNeeds;
  worksetNeeds.addBasis(hgradBD);
  worksetNeeds.addBasis(hcurlBD);
  worksetNeeds.addBasis(hdivBD);
  worksetNeeds.addIntegrator(integrationDescriptor);

  RCP<WorksetFactory> worksetFactory(new WorksetFactory(mesh));
  std::vector<std::string> eBlockNames;
  mesh->getElementBlockNames(eBlockNames);
  std::map<std::string,WorksetNeeds> eblockNeeds;
  for (const auto& block : eBlockNames)
    eblockNeeds[block] = worksetNeeds;
  RCP<WorksetContainer> worksetContainer(new WorksetContainer(worksetFactory,eblockNeeds));

  // Build Connection Manager
  using LO = int;
  using GO = panzer::Ordinal64;
  timer->start("ConnManager ctor");
  const RCP<panzer::ConnManager<LO,GO> > connManager = rcp(new panzer_stk::STKConnManager<GO>(mesh));
  timer->stop("ConnManager ctor");

  // Set up bases for projections
  auto cellTopology = mesh->getCellTopology(eBlockNames[0]);

  auto hgradBasis = panzer::createIntrepid2Basis<PHX::Device,double,double>(hgradBD.getType(),hgradBD.getOrder(),*cellTopology);
  RCP<const panzer::FieldPattern> hgradFP(new panzer::Intrepid2FieldPattern(hgradBasis));

  auto curlBasis = panzer::createIntrepid2Basis<PHX::Device,double,double>(hcurlBD.getType(),hcurlBD.getOrder(),*cellTopology);
  RCP<const panzer::FieldPattern> hcurlFP(new panzer::Intrepid2FieldPattern(curlBasis));

  auto divBasis = panzer::createIntrepid2Basis<PHX::Device,double,double>(hdivBD.getType(),hdivBD.getOrder(),*cellTopology);
  RCP<const panzer::FieldPattern> hdivFP(new panzer::Intrepid2FieldPattern(divBasis));

  // ****************
  // Testing ghosting
  // ****************
  {
    PANZER_FUNC_TIME_MONITOR("L2Projection: test ghosting (build connectivity)");
    out.setShowProcRank(true);
    out.setOutputToRootOnly(myRank);
    connManager->buildConnectivity(*hgradFP);
    for (const auto& block : eBlockNames) {
      const auto& localElements = connManager->getElementBlock(block);
      const auto& ghostElements = connManager->getNeighborElementBlock(block);
      out << "block=" << block << ", numOwnedElements = " << localElements.size()
          << ", numGhostedElements = " << ghostElements.size() << std::endl;
      for (const auto& e : localElements) {
        auto cellSize = connManager->getConnectivitySize(e);
        auto conn = connManager->getConnectivity(e);
        std::stringstream os;
        os << "owned lid=" << e << ", cellSize=" << cellSize << ", conn=";
        for (int i=0; i < cellSize; ++i)
          os << conn[i] << ",";
        out << os.str() << std::endl;
      }
      for (const auto& e : ghostElements) {
        auto cellSize = connManager->getConnectivitySize(e);
        auto conn = connManager->getConnectivity(e);
        std::stringstream os;
        os << "ghosted lid=" << e << ", cellSize=" << cellSize << ", conn=";
        for (int i=0; i < cellSize; ++i)
          os << conn[i] << ",";
        out << os.str() << std::endl;
      }
    }
    out.setOutputToRootOnly(0);
  }
  // ****************
  // ****************

  // Build source DOF Manager that mimics multi-fluid plasma dof manager
  timer->start("Build sourceGlobalIndexer");
  RCP<panzer::DOFManager<LO,GO>> sourceGlobalIndexer = rcp(new panzer::DOFManager<LO,GO>(connManager,*comm->getRawMpiComm()));
  sourceGlobalIndexer->addField("PHI",hgradFP); // Electric Potential for ES
  sourceGlobalIndexer->addField("Chaff0",hgradFP); // Dummy
  sourceGlobalIndexer->addField("Chaff1",hgradFP); // Dummy
  sourceGlobalIndexer->addField("Chaff2",hgradFP); // Dummy
  sourceGlobalIndexer->addField("E_Field",hcurlFP); // Electric Field for EM
  sourceGlobalIndexer->addField("B_Field",hdivFP); // Magnetic Field for EM
  sourceGlobalIndexer->buildGlobalUnknowns();
  timer->stop("Build sourceGlobalIndexer");

  // Build Target DOF Manager (Separate scalar fields on hgrad)
  timer->start("Build targetGlobalIndexer");
  RCP<panzer::DOFManager<LO,GO>> targetGlobalIndexer = rcp(new panzer::DOFManager<LO,GO>(connManager,*comm->getRawMpiComm()));
  targetGlobalIndexer->addField("Projection to Mesh Vertices",hgradFP);
  targetGlobalIndexer->buildGlobalUnknowns();
  timer->stop("Build targetGlobalIndexer");

  // Build projection factory
  timer->start("projectionFactory.setup()");
  panzer::L2Projection<LO,GO> projectionFactory;
  projectionFactory.setup(hgradBD,integrationDescriptor,comm,connManager,eBlockNames,worksetContainer);
  timer->stop("projectionFactory.setup()");

  // Build mass matrix
  timer->start("projectionFactory.buildMassMatrix()");
  auto massMatrix = projectionFactory.buildMassMatrix();
  timer->stop("projectionFactory.buildMassMatrix()");
  massMatrix->print(out);
  massMatrix->getRowMap()->describe(out,Teuchos::EVerbosityLevel::VERB_EXTREME);
  massMatrix->getColMap()->describe(out,Teuchos::EVerbosityLevel::VERB_EXTREME);

  // Build rhs matrix
  timer->start("projectionFactory.buildRHSMatrix()");
  const int xDir = 0;
  const int yDir = 1;
  const int zDir = 2;
  using NodeType = Kokkos::Compat::KokkosDeviceWrapperNode<PHX::Device>;
  auto rhsMatrix_PHI = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"PHI",hgradBD);          // Project value from scalar basis
  auto rhsMatrix_DPHI_DX = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"PHI",hgradBD,xDir); // Project gradient from scalar basis
  auto rhsMatrix_E0 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"E_Field",hcurlBD,xDir);  // Project value from vector basis
  auto rhsMatrix_E1 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"E_Field",hcurlBD,yDir);  // Project value from vector basis
  auto rhsMatrix_E2 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"E_Field",hcurlBD,zDir);  // Project value from vector basis
  auto rhsMatrix_B0 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"B_Field",hdivBD,xDir);   // Project value from vector basis
  auto rhsMatrix_B1 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"B_Field",hdivBD,yDir);   // Project value from vector basis
  auto rhsMatrix_B2 = projectionFactory.buildRHSMatrix(sourceGlobalIndexer,Teuchos::null,"B_Field",hdivBD,zDir);   // Project value from vector basis
  timer->stop("projectionFactory.buildRHSMatrix()");

  // Store in vector for convenience
  std::vector<RCP<Tpetra::CrsMatrix<double,LO,GO,NodeType>>> rhsMatrices;
  rhsMatrices.push_back(rhsMatrix_PHI);
  rhsMatrices.push_back(rhsMatrix_DPHI_DX);
  rhsMatrices.push_back(rhsMatrix_E0);
  rhsMatrices.push_back(rhsMatrix_E1);
  rhsMatrices.push_back(rhsMatrix_E2);
  rhsMatrices.push_back(rhsMatrix_B0);
  rhsMatrices.push_back(rhsMatrix_B1);
  rhsMatrices.push_back(rhsMatrix_B2);

  // Create a Names vector for output
  std::vector<std::string> names;
  names.push_back("PHI");
  names.push_back("DPHI_DX");
  names.push_back("E0");
  names.push_back("E1");
  names.push_back("E2");
  names.push_back("B0");
  names.push_back("B1");
  names.push_back("B2");

  // Allocate the source vector
  timer->start("Allocate Source Vector");
  using VectorType = Tpetra::Vector<double,LO,GO,NodeType>;
  auto sourceValues = rcp(new VectorType(rhsMatrix_PHI->getDomainMap(),true));
  timer->stop("Allocate Source Vector");

  // Fill the source vector.
  timer->start("Fill Source Vector");
  Kokkos::deep_copy(sourceValues->getLocalView<PHX::Device>(), 1.0);
  auto hostSourceValues = Kokkos::create_mirror_view(sourceValues->getLocalView<PHX::Device>());
  if (0) { 
    const int PHI_Index = sourceGlobalIndexer->getFieldNum("PHI");
    // const int E_Index = sourceGlobalIndexer->getFieldNum("E_Field");
    // const int B_Index = sourceGlobalIndexer->getFieldNum("B_Field");

    std::vector<std::string> elementBlockNames;
    sourceGlobalIndexer->getElementBlockIds(elementBlockNames);
    for (const auto& block : elementBlockNames) {

      panzer::WorksetDescriptor wd(block,panzer::WorksetSizeType::ALL_ELEMENTS,true,true);
      const auto worksets = worksetContainer->getWorksets(wd);
      for (const auto& workset : *worksets) {
      
        // Field Offsets        
        const std::vector<LO>& offsets = sourceGlobalIndexer->getGIDFieldOffsets(block,PHI_Index);
        PHX::View<LO*> kOffsets("projection unit test: PHI Offsets",offsets.size());
        const auto hostOffsets = Kokkos::create_mirror_view(kOffsets);
        for (const auto& i : offsets)
          hostOffsets(i) = offsets[i];
        Kokkos::deep_copy(kOffsets,hostOffsets);
        PHX::Device::fence();

        // Local Ids
        Kokkos::View<LO**,PHX::Device> localIds("projection unit test: LocalIds", workset.numOwnedCells()+workset.numGhostCells()+workset.numVirtualCells(),
                                                sourceGlobalIndexer->getElementBlockGIDCount(block));
        // Remove the ghosted cell ids or the call to getElementLocalIds will spill array bounds
        const auto cellLocalIdsNoGhost = Kokkos::subview(workset.cell_local_ids_k,std::make_pair(0,workset.numOwnedCells()));
        sourceGlobalIndexer->getElementLIDs(cellLocalIdsNoGhost,localIds);

        const auto& basisValues = workset.getBasisValues(hgradBD,integrationDescriptor);
        const auto& coords = basisValues.basis_coordinates;
        const auto& x = sourceValues->getLocalView<PHX::Device>();
        const int numBasis = static_cast<int>(kOffsets.extent(0));

        std::cout << "ROGER numBasis=" << numBasis << ", basis=" << std::endl;

        Kokkos::parallel_for(workset.numOwnedCells(),KOKKOS_LAMBDA (const int& cell) {
          for (int basis=0; basis < numBasis; ++basis) {
            const int lid = localIds(cell,kOffsets(basis));
            x(lid,0) = +coords(cell,basis,0); // linear ramp in x-dir for PHI to test gradient projection
          }
        });
      }
    }
  }

  timer->stop("Fill Source Vector");

  // Assemble RHS vectors using matvec
  timer->start("Allocate RHS Vectors");
  auto rhs_PHI = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_DPHI_DX = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_E0 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_E1 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_E2 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_B0 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_B1 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  auto rhs_B2 = rcp(new VectorType(massMatrix->getDomainMap(),true));
  timer->stop("Allocate RHS Vectors");

  // Store in vector for convenience
  std::vector<RCP<VectorType>> rhs;
  rhs.push_back(rhs_PHI);
  rhs.push_back(rhs_DPHI_DX);
  rhs.push_back(rhs_E0);
  rhs.push_back(rhs_E1);
  rhs.push_back(rhs_E2);
  rhs.push_back(rhs_B0);
  rhs.push_back(rhs_B1);
  rhs.push_back(rhs_B2);

  timer->start("Assemble RHS Vectors");
  for (size_t i=0; i < rhs.size(); ++i)
    rhsMatrices[i]->apply(*sourceValues,*rhs[i]);
  timer->stop("Assemble RHS Vectors");

  // Build RHS Multivector target for efficient consistent matrix solve
  timer->start("Copy RHS Values into MV");
  const auto targetRangeMap = massMatrix->getRangeMap();
  const int numVectors = 8; // PHI, DPHI_DX, E, B
  const auto rhsMV = rcp(new Tpetra::MultiVector<double,LO,GO,NodeType>(targetRangeMap,numVectors,false));
  const auto mvView = rhsMV->getLocalView<NodeType>();
  TEST_EQUALITY(mvView.extent(1),static_cast<size_t>(numVectors));
  for (int col=0; col < numVectors; ++col) {
    const auto source = rhs[col]->getLocalView<NodeType>();
    const int numEntries = source.extent(0);
    Kokkos::parallel_for(numEntries, KOKKOS_LAMBDA (const int& i) { mvView(i,col) = source(i,0); });
    PHX::Device::fence();
  }
  timer->stop("Copy RHS Values into MV");

  // Solve the multiple matrices
  using MV = Tpetra::MultiVector<double,LO,GO,NodeType>;
  using OP = Tpetra::Operator<double,LO,GO,NodeType>;
  const auto solutionMV = rcp(new Tpetra::MultiVector<double,LO,GO,NodeType>(targetRangeMap,numVectors,true));
  const auto problem = rcp(new Belos::LinearProblem<double,MV,OP>(massMatrix, solutionMV, rhsMV));

  problem->setProblem();

  TEST_ASSERT(nonnull(massMatrix));
  TEST_ASSERT(nonnull(solutionMV));
  TEST_ASSERT(nonnull(rhsMV));
  TEST_ASSERT(nonnull(problem));

  timer->start("Create Belos Solver");
  RCP<ParameterList> pl = parameterList("L2 Consistent Projection Linear Solver");
  pl->set("Num Blocks", 100); // Max Krylov vectors to store
  pl->set("Maximum Iterations", 100);
  pl->set("Convergence Tolerance", 1e-5);
  pl->set("output Frequency",-1); // all iterations
  pl->set("Verbosity", Belos::Errors | Belos::Warnings | Belos::IterationDetails| Belos::FinalSummary);
  Belos::SolverFactory<double,MV,OP> belosFactory;
  const auto solver = belosFactory.create("GMRES",pl);
  solver->setProblem(problem);
  timer->stop("Create Belos Solver");

  timer->start("Belos Solve");
  const auto result = solver->solve();
  timer->stop("Belos Solve");

  TEST_EQUALITY(result,Belos::Converged);

  // Check the final values on host
  timer->start("Check Final Values on Host");
  double tol = 1.0e-4; // one magnitude less than solver tolerance
  for (int field=0; field < numVectors; ++field) {
    const auto hostValues = Kokkos::create_mirror_view(solutionMV->getLocalView<PHX::Device>());
    Kokkos::deep_copy(hostValues,solutionMV->getLocalView<PHX::Device>());
    for (size_t i=0; i < hostValues.extent(0); ++i) {
      out << names[field] << "(" << i << "," << field << ")=" << hostValues(i,field) << std::endl;
      if (field == 0) { // PHI
        TEST_FLOATING_EQUALITY(hostValues(i,field), 1.0, tol);
      }
      else if (field == 1) { // DPHI_DX
        // the test floating doesn't work since it forces relative
        // error, not absolute and this is too close to zero.
        // TEST_FLOATING_EQUALITY(hostValues(i,field), 0.0, tol);
        TEST_ASSERT(std::fabs(hostValues(i,field)-0.0) < tol);
      }
    }
  }
  timer->stop("Check Final Values on Host");

  // ***************************
  // Now test lumped mass matrix
  // ***************************
  timer->start("projectionFactory.buildInverseLumpedMassMatrix()");
  auto invLumpedMassMatrix = projectionFactory.buildInverseLumpedMassMatrix();
  timer->stop("projectionFactory.buildInverseLumpedMassMatrix()");
  
  timer->start("apply lumped mass matrix");
  {
    auto x = solutionMV->getLocalView<PHX::Device>();
    auto rhsMV_k = rhsMV->getLocalView<PHX::Device>();
    const auto ilmm = invLumpedMassMatrix->getLocalView<PHX::Device>();
    const int numEntries = static_cast<int>(x.extent(0));
    Kokkos::parallel_for(numEntries,KOKKOS_LAMBDA (const int i) 
      {
        for (int field=0; field < numVectors; ++field)
          x(i,field) = ilmm(i,0) * rhsMV_k(i,field);
      });
    PHX::Device::fence();
    solutionMV->template modify<PHX::Device>();
  }
  timer->stop("apply lumped mass matrix");
  
  // Check the final values on host
  timer->start("Check Final LUMPED Values on Host");
  for (int field=0; field < numVectors; ++field) {
    const auto hostValues = Kokkos::create_mirror_view(solutionMV->getLocalView<PHX::Device>());
    Kokkos::deep_copy(hostValues,solutionMV->getLocalView<PHX::Device>());
    for (size_t i=0; i < hostValues.extent(0); ++i) {
      out << names[field] << "(" << i << "," << field << ")=" << hostValues(i,field) << std::endl;
      if (field == 0) { // PHI
        TEST_FLOATING_EQUALITY(hostValues(i,field), 1.0, tol);
      }
      else if (field == 1) { // DPHI_DX
        // the test floating doesn't work since it forces relative
        // error, not absolute and this is too close to zero.
        // TEST_FLOATING_EQUALITY(hostValues(i,field), 0.0, tol);
        TEST_ASSERT(std::fabs(hostValues(i,field)-0.0) < tol);
      }
    }
  }
  timer->stop("Check Final LUMPED Values on Host");


  timer->stop("Total Time");

  Teuchos::StackedTimer::OutputOptions options;
  options.output_fraction = true;
  options.output_minmax = true;
  options.output_histogram = false;
  options.num_histogram = 5;
  timer->report(out,comm,options);
}
