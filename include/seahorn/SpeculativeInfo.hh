#ifndef __SEAHORN_SPECULATIVEINFO_HH__
#define __SEAHORN_SPECULATIVEINFO_HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <vector>

namespace seahorn {

enum class FencePlaceOpt {
  BEFORE_MEMORY,
  AFTER_BRANCH,
  EVERY_INST
};

using namespace llvm;

class SpeculativeInfo {
public:
  using FenceType = size_t;

private:
  FencePlaceOpt m_fencePlacement;
  std::unique_ptr<Module> m_originalModule;
  std::vector<FenceType> m_fences;

public:
  static char ID;

  void releaseMemory();

  FencePlaceOpt getFencePlacement() { return m_fencePlacement; }
  void setFencePlacement(FencePlaceOpt fencePlacement) { m_fencePlacement = fencePlacement; }
  Module& getOriginalModule() { return *m_originalModule; }
  void setOriginalModule(Module& M);
  bool isFenceID(FenceType id) { return std::binary_search(m_fences.begin(), m_fences.end(), id); }
  size_t getFenceCount() { return m_fences.size(); }
  void setFences(std::vector<FenceType>& fences);
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
