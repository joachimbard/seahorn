#ifndef __SEAHORN_REPAIRSPECTRE_HH__
#define __SEAHORN_REPAIRSPECTRE_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/TargetFolder.h"

#include "seahorn/SpeculativeInfo.hh"

namespace seahorn {
using namespace llvm;

typedef IRBuilder<TargetFolder> BuilderTy;

class RepairSpectre : public llvm::ModulePass {
  StringRef m_originalModuleFilename;
  raw_ostream& m_repairOutput;
  BuilderTy* m_builder;
  FunctionType* m_asmTy;
  SpeculativeInfo::FenceType m_fenceId;
  size_t m_insertedFencesNum;

  BasicBlock* addFenceBB(BasicBlock* BB);

public:
  static char ID;

  RepairSpectre(StringRef originalModuleFilename, raw_ostream& repairOutput) :
    llvm::ModulePass(ID),
    m_originalModuleFilename(originalModuleFilename),
    m_repairOutput(repairOutput),
//    Todo
//    m_fences(nullptr),
    m_fenceId(0), m_insertedFencesNum(0) {}

  virtual bool runOnModule(llvm::Module& M);
  virtual bool runOnFunction(llvm::Function& F, SpeculativeInfo& specInfo);
  virtual bool runOnBasicBlock(llvm::BasicBlock& BB, SpeculativeInfo& specInfo);

  virtual void getAnalysisUsage (llvm::AnalysisUsage& AU) const;
  virtual llvm::StringRef getPassName () const { return "RepairSpectre"; }
};
} // namespace seahorn

#endif // __SEAHORN_REPAIRSPECTRE_HH__
