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
//  Speculative& sp = getAnalysis<Speculative>();
//  Module& repairModule = *sp.getOriginalModule();
  llvm::SMDiagnostic err;
  llvm::LLVMContext context;
  std::unique_ptr<Module> originalModule = llvm::parseIRFile(m_originalModuleFilename, err, context);
  if (!originalModule) {
    if (llvm::errs().has_colors())
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    llvm::errs() << "error: "
                 << "Bitcode was not properly read; " << err.getMessage()
                 << "\n";
    if (llvm::errs().has_colors())
      llvm::errs().resetColor();
    return false;
  }

  Module& repairModule = *originalModule;
  outs() << "module before repair\n";
  repairModule.print(outs(), nullptr);

  const DataLayout& DL = repairModule.getDataLayout();
//  LLVMContext& ctx = repairModule.getContext();
  BuilderTy B(context, TargetFolder(DL));
  m_builder = &B;
  m_asmTy = FunctionType::get(B.getVoidTy(), false);

  HornSolver& hs = getAnalysis<HornSolver>();
  m_fences = hs.getInsertedFences();
  outs() << "adding the following fences in RepairSpectre pass: ";
  for (const std::string& fence : *m_fences) {
    outs() << fence << ",";
  }
  outs() << "\n";
  bool change = false;

  for (Function& F : repairModule) {
    change |= runOnFunction(F);
  }
  // Todo: clear m_originalModuleFilename
  // std::ofstream file(m_originalModuleFilename, std::ofstream::out | std::ofstream::trunc);
  // file.close();
  repairModule.print(m_repairOutput, nullptr);
  outs() << "repaired code printed\n";
  repairModule.print(outs(), nullptr);
  return change;
}

bool RepairSpectre::runOnFunction(Function& F) {
  if (F.isDeclaration()) { return false; }
  errs() << "runOnFunction called on " << F.getName() << "\n";
  bool change = false;
  std::vector<Instruction*> Worklist;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
      StringRef possibleFenceName = "fence_" + std::to_string(m_fenceNum++);
      outs() << "trying " << possibleFenceName << "\n";
      for (const std::string& fence : *m_fences) {
        if (possibleFenceName.equals(fence)) {
          Worklist.push_back(I);
          outs() << "inserting " << fence << "\n";
          break;
        }
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
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createRepairSpectre(StringRef originalModuleFilename, raw_ostream &repairOutput) {
  return new RepairSpectre(originalModuleFilename, repairOutput);
}
} // namespace seahorn
