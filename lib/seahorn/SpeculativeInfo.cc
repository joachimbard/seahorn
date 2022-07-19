//
// Created by joachim on 15.07.22.
//

#include "seahorn/SpeculativeInfo.hh"
#include "seahorn/InitializePasses.hh"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace seahorn {
using namespace llvm;

char SpeculativeInfo::ID = 0;

void SpeculativeInfo::setOriginalModule(Module &M) {
  m_originalModule = CloneModule(M);
  errs() << "SpeculativeInfo::setOriginalModule\n";
}

void SpeculativeInfo::setFences(std::vector<std::string>& fences) {
  std::sort(fences.begin(), fences.end());
  m_fences = fences;
  errs() << "SpeculativeInfo::setFences\n";
  printFences(errs());
}

void SpeculativeInfo::printFences(raw_ostream &OS) {
  for (auto& fence : m_fences) {
    OS << fence << ",";
  }
  OS << "\n";
}

void SpeculativeInfo::releaseMemory() {
  errs() << "SpeculativeInfo::releaseMemory\n";
  m_originalModule->dropAllReferences();
  m_originalModule.reset(nullptr);
  m_fences.clear();
}

char SpeculativeInfoWrapperPass::ID = 0;
SpeculativeInfoWrapperPass::SpeculativeInfoWrapperPass() : ImmutablePass(ID) {
  initializeSpeculativeInfoWrapperPassPass(*PassRegistry::getPassRegistry());
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createSpeculativeInfoWrapperPass() { return new SpeculativeInfoWrapperPass(); }
} // namespace seahorn

using namespace seahorn;
INITIALIZE_PASS(SpeculativeInfoWrapperPass, "spec-info",
                "Information of results related to speculative execution",
                false, true)
