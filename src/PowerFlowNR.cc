#include <algorithm>
#include <ostream>
#include <sstream>
#include "PowerFlowNR.h"
#include "SparseSolver.h"

namespace SmartGridToolbox
{
   BusNR::BusNR(const std::string & id, BusType type, Phases phases, const UblasVector<Complex> & V,
                const UblasVector<Complex> & Ys, const UblasVector<Complex> & Ic, const UblasVector<Complex> & S) :
      id_(id),
      type_(type),
      phases_(phases),
      V_(V),
      S_(S),
      Ys_(Ys),
      Ic_(Ic)
   {
      assert(V.size() == phases.size());
      assert(Ys.size() == phases.size());
      assert(Ic.size() == phases.size());
      assert(S.size() == phases.size());

      for (int i = 0; i < phases.size(); ++i)
      {
         nodes_.push_back(new NodeNR(*this, i));
      }
   }

   BusNR::~BusNR()
   {
      for (auto node : nodes_) delete node;
   }

   NodeNR::NodeNR(BusNR & bus, int phaseIdx) :
      bus_(&bus),
      phaseIdx_(phaseIdx),
      V_(bus.V_(phaseIdx)),
      S_(bus.S_(phaseIdx)),
      Ys_(bus.Ys_(phaseIdx)),
      Ic_(bus.Ic_(phaseIdx)),
      idx_(-1)
   {
      // Empty.
   }

   BranchNR::BranchNR(const std::string & id0, const std::string & id1, Phases phases0, Phases phases1,
                      const UblasMatrix<Complex> & Y) :
      nPhase_(phases0.size()),
      ids_{id0, id1},
      phases_{phases0, phases1},
      Y_(Y)
   {
      assert(phases1.size() == nPhase_);
      int nTerm = 2 * nPhase_;
      assert(Y.size1() == nTerm);
      assert(Y.size2() == nTerm);
   }

   PowerFlowNR::~PowerFlowNR()
   {
      for (auto pair : busses_) delete pair.second;
      for (auto branch : branches_) delete branch;
   }

   void PowerFlowNR::addBus(const std::string & id, BusType type, Phases phases, const UblasVector<Complex> & V,
         const UblasVector<Complex> & Y, const UblasVector<Complex> & I, const UblasVector<Complex> & S)
   {
      SGT_DEBUG(debug() << "PowerFlowNR : add bus " << id << std::endl);
      busses_[id] = new BusNR(id, type, phases, V, Y, I, S);
   }

   void PowerFlowNR::addBranch(const std::string & idBus0, const std::string & idBus1, Phases phases0, Phases phases1,
                               const UblasMatrix<Complex> & Y)
   {
      SGT_DEBUG(debug() << "PowerFlowNR : addBranch " << idBus0 << " " << idBus1 << std::endl);
      branches_.push_back(new BranchNR(idBus0, idBus1, phases0, phases1, Y));
   }

   void PowerFlowNR::reset()
   {
      SGT_DEBUG(debug() << "PowerFlowNR : reset." << std::endl);
      busses_ = BusMap();
      branches_ = BranchVec();
   }

   void PowerFlowNR::validate()
   {
      SGT_DEBUG(debug() << "PowerFlowNR : validate." << std::endl);

      // Make Nodes:
      NodeVec PQNodes = NodeVec();
      NodeVec PVNodes = NodeVec();
      NodeVec SLNodes = NodeVec();
      for (auto & busPair : busses_)
      {
         BusNR & bus = *busPair.second;
         NodeVec * vec = nullptr;
         if (bus.type_ == BusType::PQ)
         {
            vec = &PQNodes;
         }
         else if (bus.type_ == BusType::PV)
         {
            vec = &PVNodes;
         }
         else if (bus.type_ == BusType::SL)
         {
            vec = &SLNodes;
         }
         else
         {
            error() << "Unsupported bus type " << busType2Str(bus.type_) << std::endl;
            abort();
         }
         for (NodeNR * node : bus.nodes_)
         {
            vec->push_back(node);
         }
      }

      // Determine sizes:
      nPQ_ = PQNodes.size();
      nPV_ = PVNodes.size();
      nSL_ = SLNodes.size();
      assert(nSL_ > 0); // TODO: What is correct here?

      // Insert nodes into ordered list of all nodes. Be careful of ordering!
      nodes_ = NodeVec();
      nodes_.reserve(nNode());
      nodes_.insert(nodes_.end(), SLNodes.begin(), SLNodes.end());
      nodes_.insert(nodes_.end(), PQNodes.begin(), PQNodes.end());
      nodes_.insert(nodes_.end(), PVNodes.begin(), PVNodes.end());
      // Index all nodes:
      for (int i = 0; i < nNode(); ++i)
      {
         nodes_[i]->idx_ = i;
      }

      Y_.resize(nNode(), nNode(), false);

      // Branch admittances:
      for (BranchNR * branch : branches_)
      {
         auto it0 = busses_.find(branch->ids_[0]);
         if (it0 == busses_.end())
         {
            error() << "Branch " << branch->ids_[0] << " " << branch->ids_[1] << " contains a non-existent bus "
                    << branch->ids_[0] << std::endl;
            abort();
         }
         auto it1 = busses_.find(branch->ids_[1]);
         if (it1 == busses_.end())
         {
            error() << "Branch " << branch->ids_[0] << " " << branch->ids_[1] << " contains a non-existent bus "
                    << branch->ids_[1] << std::endl;
            abort();
         }
         const BusNR * busses[] = {it0->second, it1->second};
         int nTerm = 2 * branch->nPhase_;

         // There is one link per distinct pair of bus/phase pairs.
         for (int i = 0; i < nTerm; ++i)
         {
            int busIdxI = i / branch->nPhase_; // 0 or 1
            int branchPhaseIdxI = i % branch->nPhase_; // 0 to nPhase of branch.
            const BusNR * busI = busses[busIdxI];
            int busPhaseIdxI = busI->phases_.phaseIndex(branch->phases_[busIdxI][branchPhaseIdxI]);
            const NodeNR * nodeI = busI->nodes_[busPhaseIdxI];
            int idxNodeI = nodeI->idx_;

            // Only count each diagonal element in branch->Y_ once!
            Y_(idxNodeI, idxNodeI) += branch->Y_(i, i) + nodeI->Ys_;

            for (int k = i + 1; k < nTerm; ++k)
            {
               int busIdxK = k / branch->nPhase_; // 0 or 1
               int branchPhaseIdxK = k % branch->nPhase_; // 0 to nPhase of branch.
               const BusNR * busK = busses[busIdxK];
               int busPhaseIdxK = busK->phases_.phaseIndex(branch->phases_[busIdxK][branchPhaseIdxK]);
               const NodeNR * nodeK = busK->nodes_[busPhaseIdxK];
               int idxNodeK = nodeK->idx_;

               Y_(idxNodeI, idxNodeK) += branch->Y_(i, k);
               Y_(idxNodeK, idxNodeI) += branch->Y_(k, i);
            }
         }
      } // Loop over branches.
      SGT_DEBUG(debug() << "Y_.nnz() = " << Y_.nnz() << std::endl);

      // Load quantities.
      Ic_.resize(nNode(), false);
      for (int i = 0; i < nNode(); ++i)
      {
         Ic_(i) = nodes_[i]->Ic_;
      }

      SGT_DEBUG(debug() << "PowerFlowNR : validate complete." << std::endl);
      SGT_DEBUG(printProblem());
   }

   /// Initialize voltages:
   void PowerFlowNR::initV(UblasVector<double> & Vr, UblasVector<double> & Vi) const
   {
      for (int i = 0; i < nNode(); ++i)
      {
         const NodeNR & node = *nodes_[i];
         Vr(i) = node.V_.real();
         Vi(i) = node.V_.imag();
      }
   }

   void PowerFlowNR::initS(UblasVector<double> & P, UblasVector<double> & Q) const
   {
      for (int i = 0; i < nNode(); ++i)
      {
         const NodeNR & node = *nodes_[i];
         P(i) = node.S_.real();
         Q(i) = node.S_.imag();
      }
   }

   /// Set the part of J that doesn't update at each iteration.
   /** At this stage, we are treating J as if all busses were PQ. */
   void PowerFlowNR::initJC(UblasCMatrix<double> & JC) const
   {
      const auto G = real(Y_);
      const auto B = imag(Y_);

      const auto GPQPV = project(G, selPQPVFromAll(), selPQPVFromAll());
      const auto BPQPV = project(B, selPQPVFromAll(), selPQPVFromAll());

      for (auto it1 = GPQPV.begin1(); it1 != GPQPV.end1(); ++it1)
      {
         for (auto it2 = it1.begin(); it2 != it1.end(); ++it2)
         {
            int i = it2.index1();
            int k = it2.index2();
            JC(if_Ir(i), ix_Vr(k)) = -(*it2);
            JC(if_Ii(i), ix_Vi(k)) = -(*it2);
         }
      }

      for (auto it1 = BPQPV.begin1(); it1 != BPQPV.end1(); ++it1)
      {
         for (auto it2 = it1.begin(); it2 != it1.end(); ++it2)
         {
            int i = it2.index1();
            int k = it2.index2();
            JC(if_Ir(i), ix_Vi(k)) = *it2;
            JC(if_Ii(i), ix_Vr(k)) = -(*it2);
         }
      }
   }

   // At this stage, we are treating f as if all busses were PQ. PV busses will be taken into account later.
   void PowerFlowNR::calcf(UblasVector<double> & f,
                           const UblasVector<double> & Vr, const UblasVector<double> & Vi,
                           const UblasVector<double> & P, const UblasVector<double> & Q,
                           const UblasVector<double> & M2PV) const
   {
      const auto G = real(Y_);
      const auto B = imag(Y_);

      // PQ busses:
      const auto GPQ = project(G, selPQFromAll(), selAllFromAll());
      const auto BPQ = project(B, selPQFromAll(), selAllFromAll());

      const auto VrPQ = project(Vr, selPQFromAll());
      const auto ViPQ = project(Vi, selPQFromAll());

      const auto PPQ = project(P, selPQFromAll());
      const auto QPQ = project(Q, selPQFromAll());
      
      const auto IcrPQ = project(real(Ic_), selPQFromAll());
      const auto IciPQ = project(imag(Ic_), selPQFromAll());

      UblasVector<double> M2PQ = element_prod(VrPQ, VrPQ) + element_prod(ViPQ, ViPQ);

      project(f, selIrPQFromf()) = element_div(element_prod(VrPQ, PPQ) + element_prod(ViPQ, QPQ), M2PQ)
                                 + IcrPQ - prod(GPQ, Vr) + prod(BPQ, Vi);
      project(f, selIiPQFromf()) = element_div(element_prod(ViPQ, PPQ) - element_prod(VrPQ, QPQ), M2PQ)
                                 + IciPQ - prod(GPQ, Vi) - prod(BPQ, Vr);
      
      // PV busses. Note that these differ in that M2PV is considered a constant.
      const auto GPV = project(G, selPVFromAll(), selAllFromAll());
      const auto BPV = project(B, selPVFromAll(), selAllFromAll());

      const auto VrPV = project(Vr, selPVFromAll());
      const auto ViPV = project(Vi, selPVFromAll());

      const auto PPV = project(P, selPVFromAll());
      const auto QPV = project(Q, selPVFromAll());
      
      const auto IcrPV = project(real(Ic_), selPVFromAll());
      const auto IciPV = project(imag(Ic_), selPVFromAll());

      project(f, selIrPVFromf()) = element_div(element_prod(VrPV, PPV) + element_prod(ViPV, QPV), M2PV)
                                 + IcrPV - prod(GPV, Vr) + prod(BPV, Vi);
      project(f, selIiPVFromf()) = element_div(element_prod(ViPV, PPV) - element_prod(VrPV, QPV), M2PV)
                                 + IciPV - prod(GPV, Vi) - prod(BPV, Vr);
   }

   // At this stage, we are treating f as if all busses were PQ. PV busses will be taken into account later.
   void PowerFlowNR::updateJ(UblasCMatrix<double> & J, const UblasCMatrix<double> & JC,
                             const UblasVector<double> & Vr, const UblasVector<double> & Vi,
                             const UblasVector<double> & P, const UblasVector <double> & Q,
                             const UblasVector<double> & M2PV) const
   {
      // Elements in J that have no non-constant part will be initialized to the corresponding term in JC at the
      // start of the calculation, and will not change. Thus, only set elements that have a non-constant part.

      // Reset PV Q columns:
      for (int k = 0; k < nPV_; ++k)
      {
         column(J, ixPV_Vr(k)) = column(JC, ixPV_Vr(k));
      }

      // Block diagonal:
      for (int i = 0; i < nPQ_; ++i)
      {
         int iPQi = iPQ(i);

         double PVr_p_QVi = P(iPQi) * Vr(iPQi) + Q(iPQi) * Vi(iPQi);
         double PVi_m_QVr = P(iPQi) * Vi(iPQi) - Q(iPQi) * Vr(iPQi);
         double M2 = Vr(iPQi) * Vr(iPQi) + Vi(iPQi) * Vi(iPQi);
         double M4 = M2 * M2;
         double VrdM4 = Vr(iPQi) / M4;
         double VidM4 = Vi(iPQi) / M4;
         double PdM2 = P(iPQi) / M2;
         double QdM2 = Q(iPQi) / M2;

         J(if_Ir(i), ix_Vr(i)) = JC(if_Ir(i), ix_Vr(i)) - (2 * VrdM4 * PVr_p_QVi) + PdM2;
         J(if_Ir(i), ix_Vi(i)) = JC(if_Ir(i), ix_Vi(i)) - (2 * VidM4 * PVr_p_QVi) + QdM2;
         J(if_Ii(i), ix_Vr(i)) = JC(if_Ii(i), ix_Vr(i)) - (2 * VrdM4 * PVi_m_QVr) - QdM2;
         J(if_Ii(i), ix_Vi(i)) = JC(if_Ii(i), ix_Vi(i)) - (2 * VidM4 * PVi_m_QVr) + PdM2;
      }

      // For PV busses, M^2 is constant, and therefore we can write the Jacobian more simply.
      for (int i = 0; i < nPV_; ++i)
      {
         int iPVi = iPV(i);

         J(if_Ir(i), ix_Vr(i)) = JC(if_Ir(i), ix_Vr(i)) + P(iPVi) / M2PV(i); // This could be put in JC if we wanted.
         J(if_Ir(i), ix_Vi(i)) = JC(if_Ir(i), ix_Vi(i)) + Q(iPVi) / M2PV(i);
         J(if_Ii(i), ix_Vr(i)) = JC(if_Ii(i), ix_Vr(i)) - Q(iPVi) / M2PV(i);
         J(if_Ii(i), ix_Vi(i)) = JC(if_Ii(i), ix_Vi(i)) + P(iPVi) / M2PV(i);
      }

   }

   // Modify J and f to take into account PV busses.
   void PowerFlowNR::modifyForPV(UblasCMatrix<double> & J, UblasVector<double> & f,
                                 const UblasVector<double> & Vr, const UblasVector<double> & Vi,
                                 const UblasVector<double> & M2PV)
   {
      const auto VrPV = project(Vr, selPVFromAll());
      const auto ViPV = project(Vi, selPVFromAll());

      for (int k = 0; k < nPV_; ++k)
      {
         auto colAllVrk = column(J, ixPV_Vr(k));
         auto colAllVik = column(J, ixPV_Vi(k));
         
         // Modify f:
         f += colAllVrk * (0.5 * (M2PV(k) - VrPV(k) * VrPV(k) - ViPV(k) * ViPV(k)) / VrPV(k));

         // Modify Vi column in J:
         colAllVik -= colAllVrk * (ViPV(k) / VrPV(k));

         // Now turn Vr column into Q column.
         for (auto it = colAllVrk.begin(); it != colAllVrk.end(); ++it)
         {
            *it = 0;
         }
         J(ifPV_Ir(k), ixPV_Vr(k)) = ViPV(k) / M2PV(k);
         J(ifPV_Ii(k), ixPV_Vr(k)) = -VrPV(k) / M2PV(k);
      }
   }

   bool PowerFlowNR::solve()
   {
      SGT_DEBUG(debug() << "PowerFlowNR : solve." << std::endl);

      const double tol = 1e-10;
      const int maxiter = 20;

      UblasVector<double> Vr(nNode());
      UblasVector<double> Vi(nNode());
      initV(Vr, Vi);

      UblasVector<double> M2PV = element_prod(project(Vr, selPVFromAll()), project(Vr, selPVFromAll()))
                               + element_prod(project(Vi, selPVFromAll()), project(Vi, selPVFromAll()));

      UblasVector<double> P(nNode());
      UblasVector<double> Q(nNode());
      initS(P, Q);

      UblasCMatrix<double> JC(nVar(), nVar()); ///< The part of J that doesn't update at each iteration.
      initJC(JC);
      SGT_DEBUG(debug() << "After initialization, JC.nnz() = " << JC.nnz() << std::endl);
      debug() << "\tAfter initialization: JC = " << std::endl;
      for (int i = 0; i < nVar(); ++i)
      {
         debug() << "\t\t" << std::setprecision(5) << std::setw(9) << row(JC, i) << std::endl;
      }

      UblasVector<double> f(nVar()); ///< Current mismatch function.

      UblasCMatrix<double> J = JC; ///< Jacobian, d f_i / d x_i.
      SGT_DEBUG(debug() << "Initial : J.nnz() = " << J.nnz() << std::endl);

      bool wasSuccessful = false;
      double err = 0;
      for (int i = 0; i < maxiter; ++ i)
      {
         SGT_DEBUG(debug() << "\tIteration = " << i << std::endl);

         calcf(f, Vr, Vi, P, Q, M2PV);
         err = norm_inf(f);
         SGT_DEBUG(debug() << "\tf  = " << std::setprecision(5) << std::setw(9) << f << std::endl);
         SGT_DEBUG(debug() << "\tError = " << err << std::endl);
         if (err <= tol)
         {
            SGT_DEBUG(debug() << "\tSuccess at iteration " << i << ". Error = " << err << std::endl);
            wasSuccessful = true;
            break;
         }

         updateJ(J, JC, Vr, Vi, P, Q, M2PV);
         SGT_DEBUG(debug() << "After updateJ : J.nnz() = " << J.nnz() << std::endl);

         modifyForPV(J, f, Vr, Vi, M2PV);
         SGT_DEBUG(debug() << "After modifyForPV : J.nnz() = " << J.nnz() << std::endl);

         UblasVector<double> x;

         SGT_DEBUG
         (
            debug() << "\tBefore KLUSolve: Vr  = " << std::setprecision(5) << std::setw(9) << Vr << std::endl;
            debug() << "\tBefore KLUSolve: Vi  = " << std::setprecision(5) << std::setw(9) << Vi << std::endl;
            debug() << "\tBefore KLUSolve: M^2 = " << std::setprecision(5) << std::setw(9)
                    << (element_prod(Vr, Vr) + element_prod(Vi, Vi)) << std::endl;
            debug() << "\tBefore KLUSolve: P   = " << std::setprecision(5) << std::setw(9) << P << std::endl;
            debug() << "\tBefore KLUSolve: Q   = " << std::setprecision(5) << std::setw(9) << Q << std::endl;
            debug() << "\tBefore KLUSolve: f   = " << std::setprecision(5) << std::setw(9) << f << std::endl;
            debug() << "\tBefore KLUSolve: J   = " << std::endl;
            for (int i = 0; i < nVar(); ++i)
            {
               debug() << "\t\t" << std::setprecision(5) << std::setw(9) << row(J, i) << std::endl;
            }
         );

         bool ok = KLUSolve(J, -f, x);
         SGT_DEBUG(debug() << "\tAfter KLUSolve: ok = " << ok << ", x = " << std::setprecision(5) << std::setw(9)
                           << x << std::endl);
         if (!ok)
         {
            error() << "KLUSolve failed." << std::endl;
            abort();
         }

         // Update the current values of V from the solution:
         project(Vr, selPQFromAll()) += project(x, selVrPQFromx());
         project(Vi, selPQFromAll()) += project(x, selViPQFromx());

         // Explicitly deal with the voltage magnitude constraint by updating VrPV by hand.
         auto VrPV = project(Vr, selPVFromAll());
         auto ViPV = project(Vi, selPVFromAll());
         const auto DeltaViPV = project(x, selViPVFromx());
         VrPV += element_div(M2PV - element_prod(VrPV, VrPV) - element_prod(ViPV, ViPV) 
                             - 2 * element_prod(ViPV, DeltaViPV), 2 * VrPV);
         ViPV += DeltaViPV;

         // Update Q for PV busses based on the solution.
         project(Q, selPVFromAll()) += project(x, selQPVFromx());

         SGT_DEBUG(debug() << "\tUpdated Vr  = " << std::setprecision(5) << std::setw(9) << Vr << std::endl);
         SGT_DEBUG(debug() << "\tUpdated Vi  = " << std::setprecision(5) << std::setw(9) << Vi << std::endl);
         SGT_DEBUG(debug() << "\tUpdated M^2 = " << std::setprecision(5) << std::setw(9)
                           << (element_prod(Vr, Vr) + element_prod(Vi, Vi)) << std::endl);
         SGT_DEBUG(debug() << "\tUpdated P   = " << std::setprecision(5) << std::setw(9) << P << std::endl);
         SGT_DEBUG(debug() << "\tUpdated Q   = " << std::setprecision(5) << std::setw(9) << Q << std::endl);
      }
      if (wasSuccessful)
      {
         UblasVector<Complex> V(nNode());
         UblasVector<Complex> S(nNode());

         for (int i = 0; i < nNode(); ++i)
         {
            V(i) = {Vr(i), Vi(i)};
            S(i) = {P(i), Q(i)};
         }

         // Set the slack power.
         auto SSL = project(S, selSLFromAll());

         auto VSL = project(V, selSLFromAll());
         auto IcSL = project(Ic_, selSLFromAll());

         auto YStar = conj(project(Y_, selSLFromAll(), selAllFromAll()));
         auto VStar = conj(V);
         auto IcStar = conj(project(Ic_, selSLFromAll()));

         SSL = element_prod(VSL, prod(YStar, VStar)) - element_prod(VSL, IcStar);

         // Update nodes and busses.
         for (int i = 0; i < nNode(); ++i)
         {
            NodeNR * node = nodes_[i];
            node->V_ = V(i);
            node->S_ = S(i);
            node->bus_->V_[node->phaseIdx_] = node->V_;
            node->bus_->S_[node->phaseIdx_] = node->S_;
         }
      }
      else
      {
         warning() << "PowerFlowNR: Newton-Raphson method failed to converge. Error = " << err << "." << std::endl; 
      }
      SGT_DEBUG(debug() << "PowerFlowNR : solve finished. Was successful = " << wasSuccessful << std::endl);
      return wasSuccessful;
   }

   bool PowerFlowNR::printProblem()
   {
      debug() << "PowerFlowNR::printProblem()" << std::endl;
      debug() << "\tNodes:" << std::endl;
      for (const NodeNR * nd : nodes_)
      {
         debug() << "\t\tNode:" << std::endl;
         debug() << "\t\t\tId    : " << nd->bus_->id_ << std::endl;
         debug() << "\t\t\tType  : " << nd->bus_->type_ << std::endl;
         debug() << "\t\t\tPhase : " << nd->bus_->phases_[nd->phaseIdx_] << std::endl;
         debug() << "\t\t\tV     : " << nd->V_ << std::endl;
         debug() << "\t\t\tS     : " << nd->S_ << std::endl;
         debug() << "\t\t\tYs    : " << nd->Ys_ << std::endl;
         debug() << "\t\t\tIc    : " << nd->Ic_ << std::endl;
      }
      debug() << "\tBranches:" << std::endl;
      for (const BranchNR * branch : branches_)
      {
         debug() << "\t\tBranch:" << std::endl;
         debug() << "\t\t\tBusses : " << branch->ids_[0] << ", " << branch->ids_[1] << std::endl;
         debug() << "\t\t\tPhases : " << branch->phases_[0] << ", " << branch->phases_[1] << std::endl;
         debug() << "\t\t\tY      :" << std::endl;
         for (int i = 0; i < branch->Y_.size1(); ++i)
         {
            debug() << "\t\t\t\t" << std::setprecision(5) << std::setw(20) << row(branch->Y_, i) << std::endl;
         }
      }
      debug() << "\tY:" << std::endl;
      for (int i = 0; i < Y_.size1(); ++i)
      {
         debug() << "\t\t\t\t" << std::setprecision(5) << std::setw(20) << row(Y_, i) << std::endl;
      }
   }
}
