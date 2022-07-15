//
// Created by joachim on 15.07.22.
//

#include "seahorn/SpeculativeInfo.hh"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace seahorn {
using namespace llvm;

char SpeculativeInfo::ID = 0;

bool SpeculativeInfo::runOnModule(Module& M) {
  errs() << "SpeculativeInfo\n";
  m_originalModule = CloneModule(M);
  return false;
}

void SpeculativeInfo::releaseMemory() {
  // Todo
  // m_originalModule.reset(nullptr);
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createSpeculativeInfo() { return new SpeculativeInfo(); }
} // namespace seahorn