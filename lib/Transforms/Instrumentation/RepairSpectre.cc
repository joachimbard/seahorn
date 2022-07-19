/*
 * Repair the found Spectre leaks.
 */

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Verifier.h"
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

  bool changed = false;
  outs() << "inserting the following fences in RepairSpectre: ";
  specInfo.printFences(outs());

  for (Function& F : repairModule) {
    changed |= runOnFunction(F, specInfo);
  }
  repairModule.print(m_repairOutput, nullptr);
  outs() << "repaired code printed\n";
  repairModule.print(outs(), nullptr);
  // Todo: check that all fences are inserted
  size_t fenceCount = specInfo.getFenceCount();
  if (m_insertedFencesNum != fenceCount) {
    errs() << "RepairSpectre failed: only " << m_insertedFencesNum
           << " out of " << fenceCount << " fences inserted\n";
  }

  bool broken = verifyModule(repairModule, &errs());
  errs() << "repaired module is " << (broken ? "broken\n" : "OK\n");
  return changed;
}

bool RepairSpectre::runOnFunction(Function& F, SpeculativeInfo& specInfo) {
  if (F.isDeclaration()) { return false; }
  errs() << "runOnFunction called on " << F.getName() << "\n";
  bool changed = false;
  switch (specInfo.getFencePlacement()) {
  case FencePlaceOpt::BEFORE_MEMORY: {
    std::vector<Instruction*> Worklist;
    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
      Instruction *I = &*i;
      if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
        StringRef possibleFenceName = "fence_" + std::to_string(m_fenceId++);
        outs() << "trying " << possibleFenceName << "\n";
        if (specInfo.isFenceID(possibleFenceName)) {
          Worklist.push_back(I);
          outs() << "inserting " << possibleFenceName << "\n";
        }
      }
    }
    for (Instruction *I : Worklist) {
      changed = true;
      StringRef constraints = "~{dirflag},~{fpsr},~{flags}";
      InlineAsm *fenceAsm =
          InlineAsm::get(m_asmTy, "lfence", constraints, true);
      m_builder->SetInsertPoint(I);
      // add lfence
      Value *fenceInst = m_builder->CreateCall(fenceAsm, None);
      outs() << "inserted " << *fenceInst << "\n";
      ++m_insertedFencesNum;
    }
    return changed;
  }
  case FencePlaceOpt::AFTER_BRANCH: {
    std::vector<BasicBlock*> BBs;
    for (BasicBlock& BB : F) {
      BBs.push_back(&BB);
    }
    for (BasicBlock* BB : BBs) {
      changed |= runOnBasicBlock(*BB, specInfo);
    }
    return changed;
  }
  default:
    errs() << "NOT IMPLEMENTED YET\n";
  }
}

bool RepairSpectre::runOnBasicBlock(BasicBlock &BB, SpeculativeInfo &specInfo) {
  bool changed = false;
  BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator());
  if (!BI || !BI->isConditional()) { return changed; }
  std::string possibleFenceName = "fence_" + std::to_string(m_fenceId++);
  if (specInfo.isFenceID(possibleFenceName)) {
    changed = true;
    BasicBlock* thenBB = BI->getSuccessor(0);
    BasicBlock* newThenBB = addFenceBB(thenBB);
    // fix branching
    BI->setSuccessor(0, newThenBB);
    BasicBlock *currBB = BI->getParent();
    thenBB->replacePhiUsesWith(currBB, newThenBB);
  }
  possibleFenceName = "fence_" + std::to_string(m_fenceId++);
  if (specInfo.isFenceID(possibleFenceName)) {
    changed = true;
    BasicBlock* elseBB = BI->getSuccessor(1);
    BasicBlock* newElseBB = addFenceBB(elseBB);
    // fix branching
    BI->setSuccessor(1, newElseBB);
    BasicBlock *currBB = BI->getParent();
    elseBB->replacePhiUsesWith(currBB, newElseBB);
  }
  return changed;
}

BasicBlock* RepairSpectre::addFenceBB(BasicBlock *BB) {
  LLVMContext &ctx = m_builder->getContext();
  BasicBlock* fenceBB = BasicBlock::Create(ctx, "", BB->getParent(), BB);
  m_builder->SetInsertPoint(fenceBB);
  StringRef constraints = "~{dirflag},~{fpsr},~{flags}";
  InlineAsm *fenceAsm =
      InlineAsm::get(m_asmTy, "lfence", constraints, true);
  Value *fenceInst = m_builder->CreateCall(fenceAsm, None);
  outs() << "inserted " << *fenceInst << "\n";
  ++m_insertedFencesNum;
  m_builder->CreateBr(BB);
  return fenceBB;
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
