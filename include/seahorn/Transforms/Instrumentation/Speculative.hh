#ifndef __SPECULATIVE__HH__
#define __SPECULATIVE__HH__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "seahorn/Analysis/SeaBuiltinsInfo.hh"
#include "seahorn/Analysis/StaticTaint.hh"
#include "seahorn/SpeculativeInfo.hh"

#include <map>
#include <vector>
#include <set>

namespace seahorn
{
  using namespace llvm;

  typedef IRBuilder<TargetFolder> BuilderTy;
  
  class Speculative : public llvm::ModulePass
  {

    bool m_dump;
    bool m_repair;
    Function * m_assumeFn;
    Function * m_assertFn;
    Function * m_ndBoolFn;
    CallGraph * m_CG; // Call graph of the program
    StaticTaint m_taint;

    BasicBlock * m_ErrorBB;
    BuilderTy * m_Builder;
    seahorn::SeaBuiltinsInfo *SBI;

//    std::map<BranchInst*, Value*> m_bb2spec;
//    std::map<std::string, CallInst&> m_fenceCallMap;
//    Value * m_nd;
    Type * m_BoolTy;
    ConstantInt * m_zero;
    ConstantInt * m_specDepth;
    GlobalVariable * m_SpecCounter;
    GlobalVariable * m_globalSpec;

    size_t m_numOfSpec;
    SpeculativeInfo::FenceType m_numOfFences;

    raw_ostream& m_originalModuleOutput;

    Value* createNdBoolean (IRBuilder<>& B);
    unsigned getId (const Instruction *n);

    void insertFenceFunction(Module* M, Value* globalSpec);
    BasicBlock* addSpeculationBB(std::string name, Value *localSpec, BasicBlock* bb);
    bool insertSpeculation(BranchInst& inst);

    BasicBlock* createErrorBlock (Function &F);
    void insertSpecCheck(Function &F, Instruction &inst);

    bool isErrorBB(BasicBlock *bb) {
    	Instruction *inst = bb->getFirstNonPHI();
    	if (CallInst *call = dyn_cast<CallInst>(inst)) {
          auto errorFn = SBI->mkSeaBuiltinFn(seahorn::SeaBuiltinsOp::ERROR, *bb->getParent()->getParent());
          if (call->getCalledFunction() != nullptr &&
              call->getCalledFunction() == errorFn)//   ->getName().contains("verifier.error"))
            return true;
    	}
    	return false;
    }

    BasicBlock* getErrorBB(Instruction *I);
    void emitBranchToTrap(Instruction *I, Value *Cmp);

    bool isFenced(BranchInst & inst);

    void collectCOI(Instruction *src, std::set<Value*> & coi);
    void getSpecForInst(Instruction *I, std::set<Value*> & spec);
    void getSpecForInst_rec(Instruction *I, std::set<Value*> & spec, std::set<BasicBlock*> & processed);

    void splitSelectInst(Function &F, SelectInst *SI);

    void initSpecCounter(BranchInst &br);
    void decrementSpecCounter(Module &M, int num);

//    void addFenceCall(std::string name, CallInst &CI) {
//      m_fenceCallMap.insert(std::pair<std::string, CallInst&>(name, CI));
//    }

  public:

    static char ID;

    Speculative (raw_ostream &originalModuleOutput = outs(), bool repair = false, bool dump = false) :
        llvm::ModulePass (ID),
	m_dump(dump),
        m_repair(repair),
        m_originalModuleOutput(originalModuleOutput),
	m_assumeFn(nullptr),
	m_assertFn(nullptr),
	m_ndBoolFn(nullptr),
        m_ErrorBB(nullptr),
        m_CG (nullptr),
//        m_bb2spec(),
//        m_fenceCallMap(std::map<std::string, CallInst&>()),
//        m_nd(nullptr),
        m_BoolTy(nullptr),
        m_zero(nullptr),
        m_specDepth(nullptr),
        m_numOfSpec(0),
        m_numOfFences(0) { }

    virtual bool runOnModule (llvm::Module &M);
    virtual bool runOnFunction (Function &F);
    virtual bool runOnBasicBlock(BasicBlock &B);

    void addAssertions(Function &F, std::vector<Instruction*> & WorkList);

    virtual void getAnalysisUsage (llvm::AnalysisUsage &AU) const;
    virtual StringRef getPassName () const { return "SpeculativeExecution"; }

//    std::map<std::string, CallInst&>& getFenceCallMap() { return m_fenceCallMap; }
  };

}

#endif
