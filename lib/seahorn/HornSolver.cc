#include "seahorn/HornSolver.hh"
#include "seahorn/Expr/ExprLlvm.hh"
#include "seahorn/HornClauseDBTransf.hh"
#include "seahorn/HornDbModel.hh"
#include "seahorn/HornifyModule.hh"
#include "seahorn/Transforms/Instrumentation/Speculative.hh"

#include "seahorn/Support/Stats.hh"
#include "llvm/IR/Function.h"
//#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"

#include "boost/range/algorithm/reverse.hpp"

#include "seahorn/Support/SeaDebug.h"
#include <climits>

using namespace llvm;

static llvm::cl::opt<std::string> ChcEngine("horn-pdr-engine",
                                            llvm::cl::desc("CHC engine to use"),
                                            cl::init("spacer"), cl::Hidden);

static llvm::cl::opt<bool>
    LocalContext("horn-solver-local-ctx", cl::init(false),
                 cl::desc("Whether to use local z3 context"));

static llvm::cl::opt<bool>
    PrintAnswer("horn-answer", cl::desc("Print Horn answer"), cl::init(false));

static llvm::cl::opt<bool>
    InsertFences("insert-fences", cl::desc("Insert fences to mitigate Spectre attacks"),
                 cl::init(false));

enum FenceChoiceOpt {
  ALL,
  LATE,
  EARLY,
  OPT
};

static llvm::cl::opt<FenceChoiceOpt> FenceChoice(
    "fence-choice",
    llvm::cl::desc("Choice of the possible fences that eliminate a counterexample"),
    llvm::cl::values(
        clEnumValN(ALL, "all", "Insert a fence at all possible positions (irrespective if there is a leak). Check that this fixes all potential leaks."),
        clEnumValN(LATE, "late", "Choose the last fence out of some list."),
        clEnumValN(EARLY, "early", "Choose the first fence out of some list."),
        clEnumValN(OPT, "opt", "Choose the fence according to some heuristic.")
    ),
    llvm::cl::init(OPT));

static llvm::cl::list<seahorn::SpeculativeInfo::FenceType> FenceHints(
    "fence-hints",
    llvm::cl::desc("Give hints on where to place fences"),
    llvm::cl::CommaSeparated);

static llvm::cl::opt<bool>
    IncrementalCover("horn-incremental-cover", cl::init(true),
                     cl::desc("Reuse cover for recursive repair iterations"));

static llvm::cl::opt<bool>
    SimplifierPve("horn-tail-simplifier-pve",
                  cl::desc("Set fp.xform.tail_simplifier_pve"),
                  cl::init(false));

static llvm::cl::opt<bool> EstimateSizeInvars(
    "horn-estimate-size-invars",
    cl::desc("Give an estimation about the size of all inferred invariants"),
    cl::init(false));

static llvm::cl::opt<bool>
    SkipConstraints("horn-skip-constraints", cl::Hidden, cl::init(false),
                    cl::desc("Enabled when number of predicates exceeds 200"));

static llvm::cl::opt<bool>
    Subsumption("horn-subsumption", cl::Hidden, cl::init(true),
                cl::desc("Setting to false helps with cex"));

static llvm::cl::opt<bool> FlexTrace("horn-flex-trace", cl::Hidden,
                                     cl::init(false));

static llvm::cl::opt<unsigned> HornChildren("horn-solver-child-order",
                                            cl::Hidden, cl::init(2));

static llvm::cl::opt<unsigned> PdrContexts("horn-pdr-contexts", cl::Hidden,
                                           cl::init(500));

static llvm::cl::opt<bool> WeakAbs("horn-weak-abs", cl::Hidden, cl::init(false),
                                   cl::desc("Perform weak abstraction"));

static llvm::cl::opt<bool>
    GroundPobs("horn-solver-ground-pobs", cl::Hidden, cl::init(true),
               cl::desc("Ground proof obligations to ensure QF"));

static llvm::cl::opt<bool>
    UseMbqi("horn-use-mbqi", cl::Hidden, cl::init(false),
            cl::desc("Use model-based quantifier instantiation"));

static llvm::cl::opt<bool> KeepProxy("horn-keep-proxy", cl::Hidden,
                                     cl::init(false),
                                     cl::desc("Keep proxy variables"));

static llvm::cl::opt<unsigned>
    // 0: old implementation of unsat-core generation
    // 1: new implementation of IUC
    // 2: new implementation of IUC + min-cut optimization
    IUC("horn-iuc", cl::Hidden, cl::init(1));

static llvm::cl::opt<unsigned>
    // 0: simple Farkas plugin
    // 1: simple Farkas plugin from other partition
    // 2: Gaussian elimination optimization
    // 3: use additive IUC plugin
    IUCArith("horn-iuc-arith", cl::Hidden, cl::init(1));

namespace solver_detail {
enum invariant_usage_t {
  INACTIVE // add them but without using them (only debugging purposes)
  ,
  BG_ONLY // use invariants only in background
  ,
  ALWAYS // use invariants everywhere
};
}

static llvm::cl::opt<enum solver_detail::invariant_usage_t> UseInvariant(
    "horn-use-invs", llvm::cl::desc("Tell the solver how to use invariants"),
    llvm::cl::values(
        clEnumValN(solver_detail::INACTIVE, "inactive",
                   "Add invariants but without using them"),
        clEnumValN(solver_detail::BG_ONLY, "bg",
                   "Add invariants and using them only in background"),
        clEnumValN(solver_detail::ALWAYS, "always",
                   "Add invariants and use them everywhere")),
    llvm::cl::init(solver_detail::BG_ONLY));

static llvm::cl::opt<unsigned>
    HornMaxDepth("horn-max-depth", cl::Hidden,
                 cl::desc("Maximum exploration depth"), cl::init(UINT_MAX));

static llvm::cl::opt<bool>
    UseEufGen("horn-use-euf-gen", cl::Hidden, cl::init(true),
              cl::desc("Use euf generalizer for equalities"));

namespace seahorn {
char HornSolver::ID = 0;

bool HornSolver::runOnModule(Module &M) {
  Stats::sset("Result", "UNKNOWN");

//  std::time_t timestamp = std::time(nullptr);
//  outs() << std::ctime(&timestamp);
  outs() << "incremental cover: " << (IncrementalCover ? "true\n" : "false\n");
//  outs().flush();
  HornifyModule &hm = getAnalysis<HornifyModule>();
  auto &db = hm.getHornClauseDB();

  if (FenceChoice == FenceChoiceOpt::ALL) {
    bool changed = true;
    for (size_t i = 0; changed; ++i) {
      changed = insertFence(M, db, i);
    }
  }

  return runOnModule(M, hm, false);
}

bool HornSolver::runOnModule(Module &M, HornifyModule &hm, bool reuseCover) {
  // Load the Horn clause database
  auto &db = hm.getHornClauseDB();
  ExprMap cover;
  if (reuseCover) {
    ExprFactory &efac = db.getExprFactory();
    for (auto &r : db.getRelations()) {
      ExprVector args;
      for (unsigned i = 0, sz = bind::domainSz(r); i < sz; ++i) {
        Expr argName = mkTerm<std::string>(
            "arg_" + boost::lexical_cast<std::string>(i), efac);
        args.push_back(bind::mkConst(argName, bind::domainTy(r, i)));
      }
      Expr pred = bind::fapp(r, args);
      Expr lemma = m_fp->getCoverDelta(pred);
      // remember the cover
      cover[pred] = lemma;
      // Todo: add cover to DB instead of the above
//      db.addConstraint(pred, lemma);
    }
  }
  if (LocalContext) {
    m_local_ctx.reset(new EZ3(hm.getExprFactory()));
    m_fp.reset(new ZFixedPoint<EZ3>(*m_local_ctx));
  } else {
    m_fp.reset(new ZFixedPoint<EZ3>(hm.getZContext()));
  }
  ZFixedPoint<EZ3> &fp = *m_fp;

  ZParams<EZ3> params(fp.getContext());
  params.set(":engine", ChcEngine);
  // -- disable slicing so that we can use cover
  params.set(":xform.slice", false);
  // params.set (":spacer.reset_obligation_queue", true);
  // params.set (":pdr.flexible_trace", FlexTrace);
  params.set(":xform.inline-linear", false);
  params.set(":xform.inline-eager", false);
  // -- disable propagate_variable_equivalences in tail_simplifier
  params.set(":xform.tail_simplifier_pve", SimplifierPve);
  params.set(":xform.subsumption_checker", Subsumption);
  params.set(":spacer.order_children", HornChildren);
  params.set(":spacer.max_num_contexts", PdrContexts);
  params.set(":spacer.elim_aux", true);
  params.set(":spacer.reach_dnf", true);
  // params.set ("print_statistics", true);
  params.set(":spacer.use_bg_invs", UseInvariant == solver_detail::INACTIVE ||
                                        UseInvariant == solver_detail::BG_ONLY);
  params.set(":spacer.weak_abs", WeakAbs);
  params.set(":spacer.mbqi", UseMbqi);
  params.set(":spacer.iuc", IUC);
  params.set(":spacer.iuc.arith", IUCArith);
  // -- less incremental but constraints are popped after pushed in
  //    the solver
  params.set(":spacer.keep_proxy", KeepProxy);
  params.set(":spacer.ground_pobs", GroundPobs);
  params.set(":spacer.use_euf_gen", UseEufGen);
  params.set(":spacer.max_level", HornMaxDepth);
  fp.set(params);

  if (reuseCover) {
    db.loadZFixedPoint(fp, true);
    for (auto &p : cover) {
      fp.addCover(p.first, p.second);
    }
  } else {
    db.loadZFixedPoint(fp, SkipConstraints);
  }

//  ExprFactory &efac = db.getExprFactory();
//  for (auto &r : db.getRelations()) {
//    ExprVector args;
//    for (unsigned i = 0, sz = bind::domainSz(r); i < sz; ++i) {
//      Expr argName = mkTerm<std::string>(
//          "arg_" + boost::lexical_cast<std::string>(i), efac);
//      args.push_back(bind::mkConst(argName, bind::domainTy(r, i)));
//    }
//    Expr pred;
//    pred = bind::fapp(r, args);
//    Expr lemma = m_fp->getCoverDelta(pred);
//    outs() << "cover for " << *pred << ": " << *lemma << "\n";
//  }

  if (UseInvariant == solver_detail::INACTIVE) {
    params.set(":spacer.use_bg_invs", false);
    fp.set(params);
  }

  Stats::resume("Horn");
  m_result = fp.query();
  Stats::stop("Horn");

  if (m_result)
    outs() << "sat";
  else if (!m_result)
    outs() << "unsat";
  else
    outs() << "unknown";
  outs() << "\n";

  if (m_result)
    Stats::sset("Result", "FALSE");
  else if (!m_result)
    Stats::sset("Result", "TRUE");

  LOG("answer", if (m_result || !m_result) errs() << fp.getAnswer() << "\n";);

  if (!m_result) {
    if (PrintAnswer) {
      HornDbModel dbModel;
      initDBModelFromFP(dbModel, db, fp);
      printInvars(M, dbModel);
    }
    if (InsertFences) {
      SpeculativeInfo &specInfo = getAnalysis<SpeculativeInfoWrapperPass>().getSpecInfo();
      specInfo.setFences(m_inserted_fences);
    }
  } else if (m_result) {
    if (!InsertFences && PrintAnswer) {
//    if (PrintAnswer) {
      printCex();
    }
//    static int max_iters = 4;
//    if (InsertFences && --max_iters >= 0) {
    if (InsertFences) {
      auto fenceId = FenceChoice == OPT ? getFenceOpt() : getFenceSimple();
      if (!FenceHints.empty()) {
        std::vector<SpeculativeInfo::FenceType> fences;
        getFencesAlongTrace(fences);
        for (auto fence : fences) {
          for (auto hint : FenceHints) {
            if (fence == hint) {
              fenceId = fence;
              goto end_search;
            }
          }
        }
      }
end_search:
      bool changed = insertFence(M, db, fenceId);
      if (changed) {
        return runOnModule(M, hm, IncrementalCover);
      }
    }
  }

  if (EstimateSizeInvars)
    estimateSizeInvars(M);

//  std::time_t timestamp = std::time(nullptr);
//  outs() << "finish HornSolver: " << std::ctime(&timestamp);
//  outs().flush();
  return false;
}

bool HornSolver::insertFence(Module &M, HornClauseDB &db, SpeculativeInfo::FenceType fenceId) {
//  Function *fence = M.getFunction(name);
//  if (fence) {
//    fence->print(outs());
//    //          fence->deleteBody();
//    //          fence->setLinkage(llvm::GlobalValue::InternalLinkage);
//    LLVMContext &ctx = M.getContext();
//    IRBuilder<> B(ctx);
//    for (BasicBlock &BB : *fence) {
//      for (Instruction &I : BB) {
//        if (ReturnInst *RI = dyn_cast<ReturnInst>(&I)) {
//          RI->setOperand(0, B.getTrue());
//        }
//      }
//    }
//    //          BasicBlock *bb = BasicBlock::Create(ctx, "entry", fence);
//    //          B.SetInsertPoint(bb);
//    //          B.CreateRet(B.getTrue());
//    fence->print(outs());
//  } else { errs() << "could not insert fence " << name << " to module\n"; }
  Expr rule;
  bool changed = db.changeFenceRules(fenceId, rule);
  if (changed) {
    m_inserted_fences.push_back(fenceId);
//    std::time_t timestamp = std::time(nullptr);
//    outs() << std::ctime(&timestamp);
    outs() << "insert fence id " << fenceId << "\n";
//    outs() << m_inserted_fences.size() << " fences inserted so far\n";
//    outs().flush();
  }
  return changed;
}

void HornSolver::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<HornifyModule>();
  if (InsertFences) { AU.addRequired<SpeculativeInfoWrapperPass>(); }
  AU.setPreservesAll();
}

void HornSolver::printCex() {
  ZFixedPoint<EZ3> fp = *m_fp;
  // outs () << *fp.getCex () << "\n";

  ExprVector rules;
  fp.getCexRules(rules);
  boost::reverse(rules);
  for (Expr r : rules) {
    Expr src;
    Expr dst;

    if (isOpX<IMPL>(r)) {
      dst = r->arg(1);
      r = r->arg(0);
      // Todo: We probalby cannot just take the first conjunct.
//      src = isOpX<AND>(r) ? r->arg(0) : r;
      if (isOpX<AND>(r)) {
        bool nonEmpty = false;
        for (auto B = r->args_begin(), E = r->args_end(); B != E; ++B) {
          src = *B;
          if (!bind::isFapp(src)) { continue; }
          src = bind::fname(bind::fname(src));
          if (nonEmpty) { outs() << ", "; }
          outs() << *src;
          nonEmpty = true;
        }
        outs() << " --> ";
      } else {
        src = r;
        if (bind::isFapp(src)) {
          src = bind::fname(bind::fname(src));
          outs() << *src << " --> ";
        }
      }
    } else
      dst = r;

//    if (src) {
//      if (!bind::isFapp(src))
//        src.reset(0);
//      else
//        src = bind::fname(bind::fname(src));
//    }
//
//    if (src)
//      outs() << *src << " --> ";

    dst = bind::fname(bind::fname(dst));
    outs() << *dst << "\n";
  }
}

void HornSolver::getFencesAlongTrace(std::vector<SpeculativeInfo::FenceType> &fences) {
  ZFixedPoint<EZ3> fp = *m_fp;
  ExprVector rules;
  fp.getCexRules(rules);
  outs() << "found fences: ";
  for (Expr r : rules) {
    if (isOpX<IMPL>(r)) { continue; }
    Expr expr = bind::fname(bind::fname(r));
    std::string name = boost::lexical_cast<std::string>(*expr);
    // match fence_*@entry
    int noFence = name.compare(0, 6, "fence_");
    size_t atEntry = name.find("@entry");
    if (noFence || atEntry == std::string::npos) { continue; }
    name.erase(atEntry);
    fences.push_back(fenceNameToId(name));
    outs() << name << ',';
  }
  outs() << '\n';
}

// void HornSolver::getFenceCallMap(Module &M) {
//   outs() << "fence calls:\n";
//   for (Function &F : M) {
//     for (BasicBlock &BB : F) {
//       for (Instruction &I : BB) {
//         if (CallInst *CI = dyn_cast<CallInst>(&I)) {
//           Function *fence = CI->getCalledFunction();
//           if (fence) {
//             StringRef name = fence->getName();
//             if (name.startswith("fence_")) {
//               m_fence2call.insert(std::pair<std::string, Instruction&>(name.str(), I));
//               outs() << I << " in " << BB.getName() << " in " << F.getName() << "\n";
//             }
//           }
//         }
//       }
//     }
//   }
//   outs().flush();
// }

SpeculativeInfo::FenceType HornSolver::getFenceSimple() {
  ExprVector rules;
  m_fp->getCexRules(rules);
  ExprVector::const_iterator rulesI = rules.cbegin(), rulesE = rules.cend();
  SpeculativeInfo::FenceType fenceId = -1;
  bool fenceFound = false;
  for (; rulesI != rulesE; ++rulesI) {
    Expr r = *rulesI;
    if (isOpX<IMPL>(r)) { continue; }
    Expr expr = bind::fname(bind::fname(r));
    std::string name = boost::lexical_cast<std::string>(*expr);
    // match fence_*@entry
    int noFence = name.compare(0, 6, "fence_");
    size_t atEntry = name.find("@entry");
    if (noFence || atEntry == std::string::npos) { continue; }
    name.erase(atEntry);
    fenceId = fenceNameToId(name);
    fenceFound = true;
    if (FenceChoice == EARLY) { return fenceId; }
  }
  if (!fenceFound) { errs() << "no fence found\n"; }
  return fenceId;
}

SpeculativeInfo::FenceType HornSolver::getFenceOpt() {
  ZFixedPoint<EZ3> fp = *m_fp;
  ExprVector rules;
  fp.getCexRules(rules);
  SpeculativeInfo::FenceType fenceId = -1;
  ExprVector::const_iterator fenceI = rules.cbegin(), fenceE = rules.cend();
  bool noFenceFound = true;
  for (ExprVector::const_iterator rulesI = rules.cbegin(), rulesE = rules.cend();
       noFenceFound && rulesI != rulesE; ++rulesI) {
    // search for a rule that has fences in its body
    Expr r = *rulesI;
    if (!isOpX<IMPL>(r)) { continue; }
    Expr body = r->arg(0);
    Expr expr;
    std::set<SpeculativeInfo::FenceType> fences;
    if (isOpX<AND>(body)) {
      for (auto argsI = body->args_begin(), argsE = body->args_end(); argsI != argsE;
           ++argsI) {
        expr = bind::fname(bind::fname(*argsI));
        std::string name = boost::lexical_cast<std::string>(*expr);
        int noFence = name.compare(0, 6, "fence_");
        if (!noFence) { fences.insert(fenceNameToId(name)); }
      }
      if (fences.empty()) { continue; }
    } else {
      // only 1 predicate in body
      expr = bind::fname(bind::fname(body));
      std::string name = boost::lexical_cast<std::string>(*expr);
      int noFence = name.compare(0, 6, "fence_");
      if (!noFence) { fences.insert(fenceNameToId(name)); }
      else continue;
    }
    // search for the rules corresponding to fences
    for (; !fences.empty() && fenceI != fenceE; ++fenceI) {
      expr = *fenceI;
      if (isOpX<IMPL>(expr)) { continue; }
      else {
        expr = bind::fname(bind::fname(expr));
        std::string name = boost::lexical_cast<std::string>(*expr);
        int noFence = name.compare(0, 6, "fence_");
        size_t atEntry = name.find("@entry");
        if (!noFence) {
          if (atEntry != std::string::npos) {
            name = name.erase(atEntry);
//            if (noFenceFound) {
//              // first fence, no other to compare
//              outs() << "found: " << name << "\n";
//            } else {
//              outs() << "update to " << name << "\n";
//            }
            noFenceFound = false;
            fenceId = fenceNameToId(name);
          }
          fences.erase(fenceNameToId(name));
        }
      }
    }
    if (!noFenceFound) {
      if (!fences.empty()) {
        WARN << "possible fences not found:";
        std::string str = "  ";
        for (auto id : fences) { str.append(std::to_string(id)); }
        WARN << str;
      }
      return fenceId;
    }
  }
  errs() << "no fence found\n";
  return -1;
}

void HornSolver::estimateSizeInvars(Module &M) {
  HornifyModule &hm = getAnalysis<HornifyModule>();
  ZFixedPoint<EZ3> fp = *m_fp;

  Expr allInvars;
  bool first = true;
  unsigned numBlocks = 0;
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    for (auto &BB : F) {
      if (!hm.hasBbPredicate(BB))
        continue;
      Expr bbPred = hm.bbPredicate(BB);
      const ExprVector &live = hm.live(BB);
      Expr invars = fp.getCoverDelta(bind::fapp(bbPred, live));
      numBlocks++;
      if (first) {
        allInvars = invars;
        first = false;
      } else {
        allInvars = mk<AND>(allInvars, invars);
      }
    }
  }
  Stats::uset("NumOfBlocksWithInvariants", numBlocks);
  Stats::uset("SizeOfInvariants", (allInvars ? dagSize(allInvars) : 0));
}

void HornSolver::printInvars(Module &M, HornDbModel &model) {
  HornifyModule &hm = getAnalysis<HornifyModule>();
  for (auto &F : M)
    printInvars(F, model, hm);
}

void HornSolver::printInvars(Function &F, HornDbModel &model, HornifyModule &hm) {
  if (F.isDeclaration())
    return;

  outs() << "Function: " << F.getName() << "\n";

  // -- not used for now
  Expr summary = hm.summaryPredicate(F);
//  if (summary) {
//    outs() << "summary: " << *summary << '\n';
//  }

  ZFixedPoint<EZ3> fp = *m_fp;

  for (auto &BB : F) {
    if (!hm.hasBbPredicate(BB))
      continue;

    Expr bbPred = hm.bbPredicate(BB);

    outs() << *bind::fname(bbPred) << ":";
    const ExprVector &live = hm.live(BB);
    // Expr invars = fp.getCoverDelta (bind::fapp (bbPred, live));
    Expr invars = model.getDef(bind::fapp(bbPred, live));

    if (isOpX<AND>(invars)) {
      outs() << "\n";
      for (size_t i = 0; i < invars->arity(); ++i)
        outs() << "\t" << *invars->arg(i) << "\n";
    } else {
      outs() << " " << *invars << "\n";
    }
  }
//  outs() << "end function: " << F.getName() << "\n";
}

} // namespace seahorn
