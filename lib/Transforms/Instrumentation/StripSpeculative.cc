/*
 * Remove Speculative Execution Logic inserted by the Speculative pass.
 */

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Verifier.h"

#include "seahorn/Transforms/Instrumentation/StripSpeculative.hh"
#include "seahorn/HornSolver.hh"

namespace seahorn {
using namespace llvm;

char StripSpeculative::ID = 0;

bool StripSpeculative::runOnModule(llvm::Module& M) {
  const DataLayout& DL = M.getDataLayout();
  LLVMContext& ctx = M.getContext();
  BuilderTy B(ctx, TargetFolder(DL));
  m_builder = &B;
  m_asmTy = FunctionType::get(B.getVoidTy(), false);

  HornSolver& hs = getAnalysis<HornSolver>();
  m_inserted_fences = hs.getInsertedFences();
  outs() << "adding the following fences in StripSpeculative pass: ";
  for (std::string fence : *m_inserted_fences) {
    outs() << fence << ",";
  }
  outs() << "\n";
  bool change = false;
  for (llvm::Function& F : M) {
    change |= runOnFunction(F);
  }
  for (Function* F : m_functionsToRemove) {
    errs() << "erase " << F->getName() << "\n";
    F->eraseFromParent();
  }
  outs() << "module after StripSpeculative\n";
  outs().flush();
  M.print(outs(), nullptr);
  verifyModule(M, &errs());
  return change;
}

bool StripSpeculative::runOnFunction(llvm::Function& F) {
  m_instructionsToRemove.clear();
  StringRef name = F.getName();
  outs() << "run on function " << name << "\n";
  if (name.startswith("fence_")) {
    for (User* U : F.users()) {
      outs() << "process user " << *U << "\n";
      Instruction* I = dyn_cast<Instruction>(U);
      if (!I) {
        errs() << "user not an instruction " << *U << "\n";
        errs().flush();
      }
      // if name is contained in the set of inserted fences then insert lfence
      for (std::string fenceName : *m_inserted_fences) {
        if (name.equals(fenceName)) {
          StringRef constraints = "~{dirflag},~{fpsr},~{flags}";
          InlineAsm *fenceAsm = InlineAsm::get(m_asmTy, "lfence", constraints, true);
          m_builder->SetInsertPoint(I);
          // add lfence
          Value *fenceInst = m_builder->CreateCall(fenceAsm, None, fenceName);
          outs() << "inserted " << *fenceInst << "\n";
          break;
        }
      }
      m_instructionsToRemove.push_back(I);
    }
    for (Instruction* I : m_instructionsToRemove) {
      eraseInstructionRec(I);
    }
    m_functionsToRemove.push_back(&F);
    return true;
  }
  if (name.startswith("sea.")) {
    // Todo: remove also functions starting with seahorn., verifier., sea.,...
    m_functionsToRemove.push_back(&F);
    return true;
  }
  return false;
}

void StripSpeculative::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
  //AU.setPreservesAll();
  AU.addRequired<seahorn::HornSolver>();
}

void StripSpeculative::eraseInstructionRec(Instruction* I) {
  for (User* U : I->users()) {
    if (Instruction* I = dyn_cast<Instruction>(U)) {
      eraseInstructionRec(I);
    }
  }
  outs() << "erase " << *I << "\n";
  I->eraseFromParent();
}

} // namespace seahorn

namespace seahorn {
llvm::Pass* createStripSpeculativeExe() { return new StripSpeculative(); }
}