/*
 * Repair the found Spectre leaks.
 */

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InlineAsm.h"

#include "seahorn/Transforms/Instrumentation/RepairSpectre.hh"
#include "seahorn/Transforms/Instrumentation/Speculative.hh"
#include "seahorn/HornSolver.hh"

namespace seahorn {
using namespace llvm;

char RepairSpectre::ID = 0;

bool RepairSpectre::runOnModule(Module& M) {
  Speculative& sp = getAnalysis<Speculative>();
  Module& repairModule = *sp.getOriginalModule();
  outs() << "module before repair\n";
  repairModule.print(outs(), nullptr);
  const DataLayout& DL = repairModule.getDataLayout();
  LLVMContext& ctx = repairModule.getContext();
  BuilderTy B(ctx, TargetFolder(DL));
  m_builder = &B;
  m_asmTy = FunctionType::get(B.getVoidTy(), false);

  HornSolver& hs = getAnalysis<HornSolver>();
  m_fences = hs.getInsertedFences();
  outs() << "adding the following fences in RepairSpectre pass: ";
  for (std::string fence : *m_fences) {
    outs() << fence << ",";
  }
  outs() << "\n";
  bool change = false;

  for (Function& F : repairModule) {
    runOnFunction(F);
  }
  // Todo: print repairModule
  repairModule.print(m_repairOutput, nullptr);
  outs() << "repaired code printed\n";
  repairModule.print(outs(), nullptr);
  return change;
}

bool RepairSpectre::runOnFunction(Function& F) {
  bool change = false;
  std::vector<Instruction*> Worklist;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
      for (std::string fence : *m_fences) {
        StringRef possibleFenceName = "fence_" + m_fenceNum;
        if (possibleFenceName.equals(fence)) {
          Worklist.push_back(I);
          outs() << "inserting " << fence << "\n";
        }
      }
    }
    ++m_fenceNum;
  }
  for (Instruction* I : Worklist) {
    change = true;
    StringRef constraints = "~{dirflag},~{fpsr},~{flags}";
    InlineAsm *fenceAsm = InlineAsm::get(m_asmTy, "lfence", constraints, true);
    m_builder->SetInsertPoint(I);
    // add lfence
    Value *fenceInst = m_builder->CreateCall(fenceAsm, None);
    outs() << "inserted " << *fenceInst << "\n";
  }
  return change;
}

void RepairSpectre::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
  // Todo: check this
  AU.setPreservesAll();
  AU.addRequired<seahorn::Speculative>();
  AU.addRequired<seahorn::HornSolver>();
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createRepairSpectre(raw_ostream &repairOutput) {
  return new RepairSpectre(repairOutput);
}
} // namespace seahorn
