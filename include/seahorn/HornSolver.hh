#ifndef HORN_SOLVER__HH_
#define HORN_SOLVER__HH_

#include "boost/logic/tribool.hpp"
#include "seahorn/HornDbModel.hh"
#include "llvm/IR/Module.h"
//#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"

#include "seahorn/Expr/Smt/EZ3.hh"

namespace seahorn {
using namespace llvm;

class HornSolver : public llvm::ModulePass {
  boost::tribool m_result;
  std::unique_ptr<EZ3> m_local_ctx;
  std::unique_ptr<ZFixedPoint<EZ3>> m_fp;
  std::vector<std::string> m_inserted_fences;
//  std::map<std::string, Instruction&> m_fence2call;

  bool runOnModule(Module &M, HornifyModule &hm, bool reuseCover);

  void printCex();
  void getFencesAlongTrace(std::vector<std::string> &fences);
  std::string getFenceSimple();
  std::string getFenceOpt();
  void estimateSizeInvars(Module &M);

  void printInvars(Function &F, HornDbModel &model);
  void printInvars(Module &M, HornDbModel &model);

public:
  static char ID;

  HornSolver() : ModulePass(ID), m_result(boost::indeterminate), m_inserted_fences() {}
  virtual ~HornSolver() {}

  virtual bool runOnModule(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual StringRef getPassName() const { return "HornSolver"; }
  ZFixedPoint<EZ3> &getZFixedPoint() { return *m_fp; }

  boost::tribool getResult() { return m_result; }
  void releaseMemory() {
    m_fp.reset(nullptr);
    m_local_ctx.reset(nullptr);
    m_inserted_fences.clear();
//    m_fence2call.clear();
  }
};

} // namespace seahorn

#endif /* HORN_SOLVER__HH_ */
