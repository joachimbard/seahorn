#ifndef __SEAHORN_SPECULATIVEINFO_HH__
#define __SEAHORN_SPECULATIVEINFO_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"

namespace seahorn {
using namespace llvm;

class SpeculativeInfo : public llvm::ModulePass {
  std::unique_ptr<Module> m_originalModule;

public:
  static char ID;

  SpeculativeInfo() :
    llvm::ModulePass(ID) {}

  virtual bool runOnModule(Module& M);
  virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
  virtual StringRef getPassName () const { return "SpeculativeInfo"; }
  virtual void releaseMemory();

  Module& getOriginalModule() { return *m_originalModule; }
};
} // namespace seahorn

#endif // __SEAHORN_SPECULATIVEINFO_HH__
