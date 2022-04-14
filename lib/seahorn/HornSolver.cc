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
  LATE,
  EARLY,
  DOM
};

static llvm::cl::opt<FenceChoiceOpt> FenceChoice(
    "fence-choice",
    llvm::cl::desc("Choice of the possible fences that eliminate a counterexample"),
    llvm::cl::values(
        clEnumValN(LATE, "late", "Choose the latest possible fences"),
        clEnumValN(EARLY, "early", "Choose the earliest possible fence"),
        clEnumValN(DOM, "dom", "Choose the fence that is dominated by all other possibilities")
    ),
    llvm::cl::init(LATE));

static llvm::cl::list<std::string> FenceHints(
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

  errs() << "incremental cover: " << IncrementalCover << "\n";
  HornifyModule &hm = getAnalysis<HornifyModule>();
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

  if (PrintAnswer && !m_result) {
    HornDbModel dbModel;
    initDBModelFromFP(dbModel, db, fp);
    printInvars(M, dbModel);
    if (InsertFences) {
      outs() << "inserted fences: ";
      for (std::string fences : m_inserted_fences) { outs() << fences << ','; }
      outs() << '\n';
    }
  } else if (m_result) {
    if (PrintAnswer) {
      printCex();
    }
    if (InsertFences) {
      std::vector<std::string> fences;
      getFencesAlongTrace(fences);
      if (!fences.empty()) {
        std::string name;
        switch (FenceChoice) {
        case LATE:
          name = fences.back();
          break;
        case EARLY:
          name = fences.front();
          break;
        case DOM:
          name = getFence();
//          if (fences.size() > 1) {
//            // YV: maybe use WtoOrder: gives order of predicates
//            //const BasicBlock &bb = hm.predicateBb(dst);
          break;
        }
        for (std::string &fence : fences) {
          for (std::string &hint : FenceHints) {
            if (fence.compare(hint) == 0) {
              name = fence;
              goto end_search;
            }
          }
        }
end_search:
        m_inserted_fences.push_back(name);
        outs() << "insert fence at " << name << '\n';
        //      Function *fence = M.getFunction(name);
        //      if (fence) {
        //        fence->print(outs());
        //        fence->deleteBody();
        //        fence->setLinkage(llvm::GlobalValue::InternalLinkage);
        //        LLVMContext &ctx = M.getContext();
        //        IRBuilder<> B(ctx);
        //        BasicBlock *bb = BasicBlock::Create(ctx, "entry", fence);
        //        B.SetInsertPoint(bb);
        //        B.CreateRet(B.getTrue());
        //        fence->print(outs());
        //      }
        Expr rule;
        bool changed = db.changeFenceRules(name, rule);
        if (changed) {
          return runOnModule(M, hm, IncrementalCover);
        }
      } else {
        errs() << "Program not secure, but no place for a fence found\n";
      }
    }
  }

  if (EstimateSizeInvars)
    estimateSizeInvars(M);

  return false;
}

void HornSolver::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<HornifyModule>();
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

void HornSolver::getFencesAlongTrace(std::vector<std::string> &fences) {
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
    fences.push_back(name);
    outs() << name << ',';
  }
  outs() << '\n';
}

std::string HornSolver::getFence() {
  ZFixedPoint<EZ3> fp = *m_fp;
  ExprVector rules;
  fp.getCexRules(rules);
  std::string fenceName;
  auto fenceI = rules.begin(), fenceE = rules.end();
  bool noFenceFound = true;
  for (auto rulesI = rules.begin(), rulesE = rules.end();
       noFenceFound && rulesI != rulesE; ++rulesI) {
    // search for a rule that has fences in its body
    Expr r = *rulesI;
    if (!isOpX<IMPL>(r)) { continue; }
    Expr body = r->arg(0);
    Expr expr;
    std::set<std::string> fences;
    if (isOpX<AND>(body)) {
      for (auto argsI = body->args_begin(), argsE = body->args_end(); argsI != argsE;
           ++argsI) {
        expr = bind::fname(bind::fname(*argsI));
        std::string name = boost::lexical_cast<std::string>(*expr);
        int noFence = name.compare(0, 6, "fence_");
        if (!noFence) { fences.insert(name); }
      }
      if (fences.empty()) { continue; }
    } else {
      // only 1 predicate in body
      expr = bind::fname(bind::fname(body));
      std::string name = boost::lexical_cast<std::string>(*expr);
      int noFence = name.compare(0, 6, "fence_");
      if (!noFence) { fences.insert(name); }
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
            if (noFenceFound) {
              // first fence, no other to compare
              outs() << "found: " << name << "\n";
//              BasicBlock &curr = *m_fence2call.at(name).getParent();
//              m_dm.recalculate(*curr.getParent());
//              m_pdm.recalculate(*curr.getParent());
            } else {
//              BasicBlock *prev = m_fence2call.at(fenceName).getParent();
//              BasicBlock *curr = m_fence2call.at(name).getParent();
//              Function *currF = curr->getParent();
//              Function *prevF = prev->getParent();
//              if (prevF == currF) {
//                if (m_dm.dominates(prev, curr)) {
//                  if (prev == curr) { errs() << "equal basic blocks\n"; }
//                  outs() << fenceName << " dominates " << name << "\n";
//                } else {
//                  errs() << fenceName << " does not dominate " << name << "\n";
//                  outs() << fenceName << " does not dominate " << name << " (still update?)\n";
//                  // fences.erase(name);
//                  // continue;
//                }
//                if (m_pdm.dominates(curr, prev)) {
//                  if (prev == curr) { errs() << "equal basic blocks\n"; }
//                  outs() << name << " post-dominates " << fenceName << "\n";
//                } else {
//                  errs() << name << " does not post-dominate " << fenceName << "\n";
//                  outs() << name << " does not post-dominate " << fenceName << " (still update?)\n";
//                  // fences.erase(name);
//                  // continue;
//                }
//              } else {
//                errs() << "dominance not possible for different functions\n";
//                errs() << "got " << fenceName << " in " << *prevF << " and ";
//                errs() << name << " in " << *currF << "\n";
//              }
              outs() << "update to " << name << "\n";
            }
            noFenceFound = false;
            fenceName = name;
          }
          fences.erase(name);
        }
      }
    }
    if (!noFenceFound) {
      if (!fences.empty()) {
        WARN << "possible fences not found:";
        std::string str = "  ";
        for (std::string name : fences) { str.append(name); }
        WARN << str;
      }
      return fenceName;
    }
  }
  errs() << "no fence found\n";
  return "";
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
  for (auto &F : M)
    printInvars(F, model);
}

void HornSolver::printInvars(Function &F, HornDbModel &model) {
  if (F.isDeclaration())
    return;

  HornifyModule &hm = getAnalysis<HornifyModule>();
  outs() << "Function: " << F.getName() << "\n";

  // -- not used for now
  Expr summary = hm.summaryPredicate(F);

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
      outs() << "\n\t";
      for (size_t i = 0; i < invars->arity(); ++i)
        outs() << "\t" << *invars->arg(i) << "\n";
    } else {
      outs() << " " << *invars << "\n";
    }
  }
//  outs() << "end function: " << F.getName() << "\n";
}

} // namespace seahorn
