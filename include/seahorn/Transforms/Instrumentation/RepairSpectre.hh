#ifndef __SEAHORN_REPAIRSPECTRE_HH__
#define __SEAHORN_REPAIRSPECTRE_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/TargetFolder.h"

namespace seahorn {
using namespace llvm;

typedef IRBuilder<TargetFolder> BuilderTy;

class RepairSpectre : public llvm::ModulePass {
  // Todo: use better structure for lookup
  raw_ostream& m_repairOutput;
  BuilderTy* m_builder;
  FunctionType* m_asmTy;
  std::vector<std::string>* m_fences;
  size_t m_fenceNum;

public:
  static char ID;

  RepairSpectre(raw_ostream& repairOutput) :
    llvm::ModulePass(ID),
    m_repairOutput(repairOutput),
    m_fences(),
    m_fenceNum(0) {}

  virtual bool runOnModule(llvm::Module& M);
  virtual bool runOnFunction(llvm::Function& F);

  virtual void getAnalysisUsage (llvm::AnalysisUsage& AU) const;
  virtual llvm::StringRef getPassName () const { return "RepairSpectre"; }
};
} // namespace seahorn

#endif // __SEAHORN_REPAIRSPECTRE_HH__
