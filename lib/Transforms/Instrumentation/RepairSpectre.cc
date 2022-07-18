/*
 * Repair the found Spectre leaks.
 */

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include "seahorn/Transforms/Instrumentation/RepairSpectre.hh"
#include "seahorn/HornSolver.hh"

namespace seahorn {
using namespace llvm;

char RepairSpectre::ID = 0;

bool RepairSpectre::runOnModule(Module& M) {
  errs() << "RepairSpectre\n";
  SpeculativeInfo& specInfo = getAnalysis<SpeculativeInfoWrapperPass>().getSpecInfo();
  Module& repairModule = specInfo.getOriginalModule();
//  llvm::SMDiagnostic err;
//  llvm::LLVMContext context;
//  std::unique_ptr<Module> originalModule = llvm::parseIRFile(m_originalModuleFilename, err, context);
//  if (!originalModule) {
//    if (llvm::errs().has_colors())
//      llvm::errs().changeColor(llvm::raw_ostream::RED);
//    llvm::errs() << "error: "
//                 << "Bitcode was not properly read; " << err.getMessage()
//                 << "\n";
//    if (llvm::errs().has_colors())
//      llvm::errs().resetColor();
//    return false;
//  }
//
//  Module& repairModule = *originalModule;
  outs() << "module before repair\n";
  repairModule.print(outs(), nullptr);

  const DataLayout& DL = repairModule.getDataLayout();
  LLVMContext& ctx = repairModule.getContext();
  BuilderTy B(ctx, TargetFolder(DL));
  m_builder = &B;
  m_asmTy = FunctionType::get(B.getVoidTy(), false);

  bool change = false;
  outs() << "inserting the following fences in RepairSpectre: ";
  specInfo.printFences(outs());

  for (Function& F : repairModule) {
    change |= runOnFunction(F, specInfo);
  }
  repairModule.print(m_repairOutput, nullptr);
  outs() << "repaired code printed\n";
  repairModule.print(outs(), nullptr);
  // Todo: check that all fences are inserted
  return change;
}

bool RepairSpectre::runOnFunction(Function& F, SpeculativeInfo& specInfo) {
  if (F.isDeclaration()) { return false; }
  errs() << "runOnFunction called on " << F.getName() << "\n";
  bool change = false;
  std::vector<Instruction*> Worklist;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
      StringRef possibleFenceName = "fence_" + std::to_string(m_fenceNum++);
      outs() << "trying " << possibleFenceName << "\n";
      if (specInfo.isFenceID(possibleFenceName)) {
        Worklist.push_back(I);
        outs() << "inserting " << possibleFenceName << "\n";
      }
    }
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
  AU.addRequired<seahorn::HornSolver>();
  AU.addRequired<seahorn::SpeculativeInfoWrapperPass>();
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createRepairSpectre(StringRef originalModuleFilename, raw_ostream &repairOutput) {
  return new RepairSpectre(originalModuleFilename, repairOutput);
}
} // namespace seahorn
