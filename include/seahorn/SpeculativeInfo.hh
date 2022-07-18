#ifndef __SEAHORN_SPECULATIVEINFO_HH__
#define __SEAHORN_SPECULATIVEINFO_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace seahorn {
using namespace llvm;

class SpeculativeInfo {
  std::unique_ptr<Module> m_originalModule;
  std::vector<std::string> m_fences;

public:
  static char ID;

  void releaseMemory();

  Module& getOriginalModule() { return *m_originalModule; }
  void setOriginalModule(Module& M);
  bool isFenceID(std::string id) { return std::binary_search(m_fences.begin(), m_fences.end(), id); }
  void setFences(std::vector<std::string>& fences);
  void printFences(raw_ostream& OS);
};

class SpeculativeInfoWrapperPass : public llvm::ImmutablePass {
  SpeculativeInfo m_specInfo;

public:
  static char ID;

  SpeculativeInfoWrapperPass();

  virtual void releaseMemory() { m_specInfo.releaseMemory(); }
  virtual StringRef getPassName () const { return "SpeculativeInfo"; }

  SpeculativeInfo& getSpecInfo() { return m_specInfo; }
};
} // namespace seahorn

#endif // __SEAHORN_SPECULATIVEINFO_HH__
