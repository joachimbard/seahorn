#ifndef __STRIPSPECULATIVE_HH__
#define __STRIPSPECULATIVE_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/TargetFolder.h"

namespace seahorn {
using namespace llvm;

typedef IRBuilder<TargetFolder> BuilderTy;

class StripSpeculative : public llvm::ModulePass {

  // Todo: use better structure for lookup
  std::vector<std::string>* m_inserted_fences;
  BuilderTy* m_builder;
  FunctionType* m_asmTy;
  std::vector<Function*> m_functionsToRemove;
  std::vector<Instruction*> m_instructionsToRemove;

  void eraseInstructionRec(Instruction* I);

  public:
    static char ID;

    StripSpeculative() :
      llvm::ModulePass(ID),
      m_functionsToRemove(std::vector<Function*>()),
      m_instructionsToRemove(std::vector<Instruction*>()) {}

    virtual bool runOnModule(llvm::Module& M);
    virtual bool runOnFunction(llvm::Function& F);

    virtual void getAnalysisUsage (llvm::AnalysisUsage& AU) const;
    virtual llvm::StringRef getPassName () const { return "StripSpeculativeExecution"; }
};
} // namespace seahorn

#endif // __STRIPSPECULATIVE_HH__
