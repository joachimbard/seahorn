/*
 * Insert Speculative Execution Logic.

 * Limitation: Currently not taking into account PHI nodes.
 */

#include "seahorn/Transforms/Instrumentation/Speculative.hh"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/CFG.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"

#include "llvm/CodeGen/MachineDominators.h"

static llvm::cl::opt<bool> HasErrorFunc(
    "speculative-exe-has-error-function",
    llvm::cl::desc("Available verifier.error function to denote error."),
    llvm::cl::init(true));

static llvm::cl::opt<seahorn::FencePlaceOpt> FencePlacement(
    "fence-placement",
    llvm::cl::desc("Location of the possible fence placements"),
    llvm::cl::values(
        clEnumValN(seahorn::FencePlaceOpt::BEFORE_MEMORY, "memory", "Insert fences directly before memory operations"),
        clEnumValN(seahorn::FencePlaceOpt::AFTER_BRANCH, "branch", "Insert fences directly after branches"),
        clEnumValN(seahorn::FencePlaceOpt::EVERY_INST, "every-inst", "Insert fences before every instruction")
    ),
    llvm::cl::init(seahorn::FencePlaceOpt::BEFORE_MEMORY));

namespace seahorn {
using namespace llvm;

char Speculative::ID = 0;

void Speculative::insertFenceFunction(Module *M, Value *globalSpec) {
  FunctionType *fenceType = FunctionType::get(m_BoolTy, false);
  std::string fenceName = "fence_" + std::to_string(m_numOfFences++);
  Function *fenceFkt = Function::Create(fenceType, Function::ExternalLinkage, fenceName, M);
  fenceFkt->addFnAttr(Attribute::NoInline);
  fenceFkt->addFnAttr(Attribute::NoUnwind);

  // Todo: add debug location to fence call
  CallInst *fenceCall = m_Builder->CreateCall(fenceFkt);
//  addFenceCall(fenceName, *fenceCall);
  // update call graph
  if (m_CG) {
    Function *F = fenceCall->getFunction();
    auto f1 = m_CG->getOrInsertFunction(F);
    auto f2 = m_CG->getOrInsertFunction(fenceFkt);
    f1->addCalledFunction(fenceCall, f2);
  }
  Value *specAndFence = m_Builder->CreateAnd(globalSpec, fenceCall, "");
  Function *assumeNotFn = SBI->mkSeaBuiltinFn(SeaBuiltinsOp::ASSUME_NOT, *M);
  m_Builder->CreateCall(assumeNotFn, specAndFence, "");

  LLVMContext &ctx = M->getContext();
  BasicBlock *fenceBB = BasicBlock::Create(ctx, "entry", fenceFkt);
  m_Builder->SetInsertPoint(fenceBB);
  Value *nd = m_Builder->CreateCall(m_ndBoolFn);
  m_Builder->CreateRet(nd);
//  m_Builder->CreateRet(m_Builder->getFalse());
  m_Builder->ClearInsertionPoint();
}

BasicBlock *Speculative::addSpeculationBB(std::string name, Value *spec, BasicBlock *bb) {
  LLVMContext &ctx = m_Builder->getContext();
  BasicBlock *specBB = BasicBlock::Create(ctx, name + "__bb", bb->getParent(), bb);
  m_Builder->SetInsertPoint(specBB);
  Value *globalSpec = m_Builder->CreateAlignedLoad(m_spec, 1);
  globalSpec = m_Builder->CreateOr(globalSpec, spec);
  m_Builder->CreateAlignedStore(globalSpec, m_spec, 1);
  if (FencePlacement == FencePlaceOpt::AFTER_BRANCH) {
    insertFenceFunction(specBB->getModule(), globalSpec);
    m_Builder->SetInsertPoint(specBB);
  }
  m_Builder->CreateBr(bb);
  return specBB;
}

void Speculative::initSpecCount(LoadInst & spec) {
  LLVMContext &ctx = m_Builder->getContext();

  BasicBlock *Orig = spec.getParent();
  BasicBlock *Cont = Orig->splitBasicBlock(spec.getNextNode());

  outs() << "Spliting BB\n";
  BasicBlock *initSpecCount = BasicBlock::Create(ctx, "spec_count_init", Orig->getParent(), Cont);
  m_Builder->SetInsertPoint(initSpecCount);

  outs() << "New BB created\n";
  Value *nd = m_Builder->CreateCall(m_ndBoolFn, None, "spec_init");
  nd = m_Builder->CreateSExtOrBitCast(nd, Type::getInt32Ty(ctx), "spec_cast");
  m_Builder->CreateAlignedStore(nd, m_SpecCount, 4);

  outs() << "Spec Count initialized\n";
  BranchInst::Create(Cont, initSpecCount);

  Orig->getTerminator()->eraseFromParent();
  m_Builder->SetInsertPoint(Orig);
  LoadInst *loadSpecCount = m_Builder->CreateAlignedLoad(m_SpecCount, 4);
  Value *initCond = m_Builder->CreateAnd(
		  &spec,
		  m_Builder->CreateICmpEQ(loadSpecCount, ConstantInt::get(ctx, APInt(32,0)), "spec_count_is_zero"),
		  "spec_count_init_cond");
  BranchInst::Create(initSpecCount, Cont, initCond, Orig);

}

void Speculative::incrementSpecCount(Instruction &inst) {

  // XXX
  // XXX The Cost should be computed relative to where speculation started
  // XXX
  unsigned cost = 1; //getInstructionCost()
  m_Builder->SetInsertPoint(&inst);
  Value *Load = m_Builder->CreateAlignedLoad(m_SpecCount, 4, "load_count");
  Value *Inc = m_Builder->CreateAdd(
		  Load,
		  ConstantInt::get(inst.getContext(), APInt(32, cost)),
		  "spec_count_int");
  Value *Store = m_Builder->CreateAlignedStore(Inc, m_SpecCount, 4, false);
}

bool Speculative::insertSpeculation(BranchInst &BI) {
  BasicBlock *thenBB = BI.getSuccessor(0);
  BasicBlock *elseBB = BI.getSuccessor(1);

    outs() << "Here1...\n";
  // If any of the branches is an Error Block (meaning, an assertion)
  // do not add speculation
  if (isErrorBB(thenBB) || isErrorBB(elseBB))
	  return false;

  outs() << "Here...\n";

  std::string name = "spec" + std::to_string(m_numOfSpec++);
  Value *cond = BI.getCondition();
  cond->setName(name + "__cond");
  m_Builder->SetInsertPoint(&BI);
  Value *negCond = m_Builder->CreateNot(cond, name + "__cond_neg");

  Value *condNd = m_Builder->CreateCall(m_ndBoolFn, None, name + "__cond_nd");
  BI.setCondition(condNd);

  BasicBlock *specThenBB = addSpeculationBB(name + "__then", negCond, thenBB);
  BasicBlock *specElseBB = addSpeculationBB(name + "__else", cond, elseBB);
  BI.setSuccessor(0, specThenBB);
  BI.setSuccessor(1, specElseBB);
  // Fix phi nodes in thenBB and elseBB
  BasicBlock *currBB = BI.getParent();
  thenBB->replacePhiUsesWith(currBB, specThenBB);
  elseBB->replacePhiUsesWith(currBB, specElseBB);

  // Now initialize the counter
  // Todo: uncomment it again
//  initSpecCount(B, *spec);

  // Todo: this might be unnecessary for the new version with a global spec variable
  // Store the Speculation variable associated with this conditional branch.
  // This should help us add the proper assertions.
  //m_bb2spec.emplace(&BI, cast<Value>(specVar));

  return true;
}

bool Speculative::isFenced(BranchInst & inst) {
  Value *cond = inst.getCondition();
  assert (cond != nullptr && "Branch condition is expected to be an instruction.");

  BasicBlock *BB = inst.getParent();
  BasicBlock::iterator I = BB->begin();
  for (;I != BB->end(); I++) {
	Instruction *curr = cast<Instruction>(I);
	if (CallInst *CI = dyn_cast<CallInst>(curr)) {
	  Function *f = CI->getCalledFunction();
	  if (f == nullptr) {
		  f = dyn_cast<Function>(CI->getCalledValue()->stripPointerCasts());
	  }
	  StringRef funcName = f->getName();
	  if (funcName.equals("spec_fence")) {
		return true;
	  }
	}
  }

  return false;
}

void Speculative::splitSelectInst(Function &F, SelectInst *SI) {

  m_Builder->SetInsertPoint(SI);

  BasicBlock *orig = SI->getParent();
  BasicBlock *Cont = orig->splitBasicBlock(m_Builder->GetInsertPoint());

  BasicBlock *thenBB = BasicBlock::Create(
    m_Builder->getContext(), "SplitSelect_then_" + SI->getName().str(), &F, Cont);
  BasicBlock *elseBB = BasicBlock::Create(
	m_Builder->getContext(), "SplitSelect_else_" + SI->getName().str(), &F, Cont);

  Instruction *term = orig->getTerminator ();
  m_Builder->SetInsertPoint(term);
  BranchInst *newTerm = m_Builder->CreateCondBr(SI->getCondition(), thenBB, elseBB);
  term->eraseFromParent();

  m_Builder->SetInsertPoint(thenBB);
  m_Builder->CreateBr(Cont);
  m_Builder->SetInsertPoint(elseBB);
  m_Builder->CreateBr(Cont);

  m_Builder->SetInsertPoint(Cont->getFirstNonPHI());
  PHINode *phi = m_Builder->CreatePHI(SI->getType(), 2);
  phi->addIncoming(SI->getTrueValue(), thenBB);
  phi->addIncoming(SI->getFalseValue(), elseBB);

  SI->replaceAllUsesWith(phi);

  SI->eraseFromParent();
}

bool Speculative::runOnBasicBlock(BasicBlock &BB) {
  bool changed = false;
  BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator());

  if (FencePlacement == FencePlaceOpt::EVERY_INST) {
    changed = true;
    Module *M = BB.getModule();
    m_Builder->SetInsertPoint(&BB.front());
    Value *globalSpec = m_Builder->CreateAlignedLoad(m_spec, 1);
    auto I = BB.begin();
    ++I; // skip globalSpec instruction which is created before
    auto E = BB.end();
    for (; I != E; ++I) {
      //
      m_Builder->SetInsertPoint(&*I);
      insertFenceFunction(M, globalSpec);
    }
  }
  if (BI == nullptr || !BI->isConditional() /* || !m_taint.isTainted(BI) */)
    return changed;

  changed |= insertSpeculation(*BI);
  return changed;
}

bool Speculative::runOnFunction(Function &F) {
  if (F.isDeclaration())
    return false;

  auto *M = F.getParent();
  const auto &DL = M->getDataLayout();
  SBI = &getAnalysis<seahorn::SeaBuiltinsInfoWrapperPass>().getSBI();

  LLVMContext &ctx = F.getContext();
  m_ErrorBB = nullptr;
  BuilderTy B(ctx, TargetFolder(DL));
  m_Builder = &B;

  if (!m_assumeFn || !m_ndBoolFn) {
    assert(false);
  }

  // First collect selection instructions
  std::vector<Instruction*> WorkList;
  // Todo: Don't change select instructions for now
//  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
//    if (isa<SelectInst>(&*i)) WorkList.push_back(&*i);
//  }
//
//  // Select Instructions have a condition by which a value is picked.
//  // Break them down to different basic blocks.
//  for (Instruction *I : WorkList)
//	  splitSelectInst(F, cast<SelectInst>(I));

  // Collect all memory instructions.
  // This work list does not include the added speculation instructions
  WorkList.clear();
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
    if (isa<LoadInst>(I) || isa<StoreInst>(I)) {
      WorkList.push_back(I);
    }
  }

  // existing fences stop speculation
  std::vector<Instruction*> manualFences;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction *I = &*i;
    if (CallInst *CI = dyn_cast<CallInst>(I)) {
      Value *V = CI->getCalledValue();
      if (InlineAsm *Asm = dyn_cast<InlineAsm>(V)) {
        StringRef name = Asm->getAsmString();
        // Todo: What if lfence is part of larger asm
        if (name.equals("lfence")) {
          manualFences.push_back(I);
        }
      }
    }
  }
  Function *assumeNotFn = SBI->mkSeaBuiltinFn(SeaBuiltinsOp::ASSUME_NOT, *M);
  for (Instruction *I : manualFences) {
    m_Builder->SetInsertPoint(I);
    Value *globalSpec = m_Builder->CreateAlignedLoad(m_spec, 1);
    m_Builder->CreateCall(assumeNotFn, globalSpec, "");
  }

  // Collect all basic blocks
  std::vector<BasicBlock*> BBWorkList;
  bool changed = false;
  for (auto & B : F) {
	  BBWorkList.push_back(&B);
  }

  // Iterate through the basic blocks
  for (auto * B : BBWorkList)
	  changed |= runOnBasicBlock(*B);

  addAssertions(F, WorkList);

  return changed;
}

void Speculative::collectCOI(Instruction *src, std::set<Value*> & coi) {
  outs() << "Collecting for: "; src->print(outs()); outs() << "\n";
  if (StoreInst *SI = dyn_cast<StoreInst>(src)) {
    if (Instruction *I = dyn_cast<Instruction>(src->getOperand(0))) {
	  if (coi.find(I) == coi.end()) {
	    coi.insert(I);
		collectCOI(I, coi);
	  }
	}
  } else {
    for (Use &U : src->operands()) {
	  Value *v = U.get();
	  if (Instruction *I = dyn_cast<Instruction>(v)) {
	    if (coi.find(I) == coi.end()) {
		  coi.insert(I);
		  collectCOI(I, coi);
	    }
	  }
    }
  }

  /*for (User *U : src->users()) {
	outs() << "User: "; U->print(outs()); outs() << "\n";
	if (StoreInst *SI = dyn_cast<StoreInst>(U))
	  if (!src->isSameOperationAs(SI)) {
		  outs() << "Adding: "; U->print(outs()); outs() << "\n";
		  coi.insert(SI);
	  }
  }*/

  BasicBlock *BB = src->getParent();
  for (BasicBlock *Pred : predecessors(BB)) {
    if (BranchInst *BI = dyn_cast<BranchInst>(Pred->getTerminator())) {
	  Value *cond = BI->isConditional() ? BI->getCondition() : nullptr;
	  if (cond && isa<Instruction>(cond) && coi.find(cond) == coi.end()) {
		coi.insert(cond);
		collectCOI(cast<Instruction>(cond), coi);
	  }
    }
  }
}

// void Speculative::getSpecForInst(Instruction *I, std::set<Value*> & spec) {
//   std::set<BasicBlock*> processed;
//   getSpecForInst_rec(I, spec, processed);
// }
//
// void Speculative::getSpecForInst_rec(Instruction *I, std::set<Value*> & spec, std::set<BasicBlock*> & processed) {
//   BasicBlock *BB = I->getParent();
//   if (processed.find(BB) != processed.end()) return;
//   processed.insert(BB);
//   for (BasicBlock *Pred : predecessors(BB)) {
// 	if (BranchInst *BI = dyn_cast<BranchInst>(Pred->getTerminator())) {
// 		if (m_bb2spec.find(BI) != m_bb2spec.end())
// 			spec.insert(m_bb2spec[BI]);
//                 getSpecForInst_rec(BI, spec, processed);
// 	}
//   }
// }

void Speculative::addAssertions(Function &F , std::vector<Instruction*> & WorkList) {
  outs() << "Starting to add assertions for " << F.getName() << "\n";
  for (Instruction *I : WorkList) {
      insertSpecCheck(F, *I);
  }
}

bool Speculative::runOnModule(llvm::Module &M) {
  if (M.begin() == M.end())
    return false;

  errs() << "Speculative\n";

  if (m_repair) {
    SpeculativeInfo& specInfo = getAnalysis<SpeculativeInfoWrapperPass>().getSpecInfo();
    specInfo.setFencePlacement(FencePlacement);
    specInfo.setOriginalModule(M);
  }

  LLVMContext &ctx = M.getContext();

  m_BoolTy = Type::getInt1Ty(ctx);


  m_SpecCount = new GlobalVariable(
		  M,
		  Type::getInt32Ty(ctx),
		  false,
		  GlobalValue::CommonLinkage,
		  ConstantInt::get(ctx, APInt(32,0)),
		  "__Spec_Counter__");
  m_SpecCount->setAlignment(4);

  m_spec = new GlobalVariable(M, m_BoolTy, false, GlobalValue::CommonLinkage,
                              ConstantInt::get(ctx, APInt(1, 0)),
                              "__global_spec__");
  m_spec->setAlignment(1);

  if (HasErrorFunc) {

    AttrBuilder B;
    AttributeList as = AttributeList::get(ctx, AttributeList::FunctionIndex, B);

    m_assumeFn = dyn_cast<Function>(M.getOrInsertFunction(
   		"verifier.assume", as, Type::getVoidTy (ctx), m_BoolTy).getCallee());

    m_assertFn = dyn_cast<Function>(M.getOrInsertFunction(
		"verifier.assert", as, Type::getVoidTy (ctx), m_BoolTy).getCallee());

    m_ndBoolFn = dyn_cast<Function>(M.getOrInsertFunction(
		"verifier.nondet.bool", as, m_BoolTy).getCallee());

    B.addAttribute(Attribute::NoReturn);
    B.addAttribute(Attribute::ReadNone);
  }

//  Todo: removing taint for now
//  outs() << "Computing taint...\n";
//  m_taint.runOnModule(M);
//  outs() << "Done - computing taint...\n";

  bool changed = false;
//  llvm::Module::FunctionListType Funcs;
  std::vector<Function*> Funcs;
  for (Function &F : M) {
    Funcs.push_back(&F);
  }
  for (Function *F : Funcs) {
    changed |= runOnFunction(*F);
  }

  outs() << "-- Inserted " << m_numOfSpec << " speculations.\n";
  if (m_dump) {
    M.print(outs(), nullptr);
  }

  return changed;
}

void Speculative::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  // Todo: The pass changes the code
//  AU.setPreservesAll();
  AU.addRequired<seahorn::SeaBuiltinsInfoWrapperPass>();
  if (m_repair) { AU.addRequired<SpeculativeInfoWrapperPass>(); }
}

BasicBlock *Speculative::createErrorBlock(Function &F) {
  BasicBlock *errBB = BasicBlock::Create(m_Builder->getContext(), "spec_error_bb", &F);

  m_Builder->SetInsertPoint(errBB);
  auto errorFn = SBI->mkSeaBuiltinFn(seahorn::SeaBuiltinsOp::ERROR, *F.getParent());
  CallInst *CI = m_Builder->CreateCall(errorFn);
//  Type *retType = F.getReturnType();
//  if (retType->isVoidTy()) {
//    B.CreateRetVoid();
//  } else {
//    B.CreateRet(ConstantInt::get(retType, 0));
//  }
  m_Builder->CreateUnreachable();

  // update call graph
  if (m_CG) {
    auto f1 = m_CG->getOrInsertFunction(&F);
    auto f2 = m_CG->getOrInsertFunction(errorFn);
    f1->addCalledFunction(CI, f2);
  }
  return errBB;
}

/// getErrorBB - create a basic block that traps. All overflowing conditions
/// branch to this block. There's only one trap block per function.
BasicBlock *Speculative::getErrorBB(Instruction *I) {
  if (m_ErrorBB)
    return m_ErrorBB;

  Function *Fn = I->getParent()->getParent();
  Module &M = *Fn->getParent();
  LLVMContext &ctx = M.getContext();
  IRBuilder<>::InsertPointGuard Guard(*m_Builder);
  m_ErrorBB = BasicBlock::Create(ctx, "spec_error_bb", Fn);
  BasicBlock *retBB = BasicBlock::Create(ctx, "ret_bb", Fn);
  m_Builder->SetInsertPoint(m_ErrorBB);
  m_Builder->CreateBr(retBB);
  m_Builder->SetInsertPoint(retBB);
  PHINode *phi = m_Builder->CreatePHI(m_Builder->getInt1Ty(), 2);
  phi->addIncoming(m_Builder->getTrue(), m_ErrorBB);
  Function *assumeFn = SBI->mkSeaBuiltinFn(seahorn::SeaBuiltinsOp::ASSUME, M);
  m_Builder->CreateCall(assumeFn, phi);

  AttrBuilder AB;
  AB.addAttribute(Attribute::NoReturn);
  AttributeList as = AttributeList::get(ctx, AttributeList::FunctionIndex, AB);
  auto errorFn = SBI->mkSeaBuiltinFn(seahorn::SeaBuiltinsOp::FAIL, M);
  CallInst *TrapCall = m_Builder->CreateCall(errorFn);
  //TrapCall->setDoesNotReturn();
  //TrapCall->setDoesNotThrow();
  TrapCall->setDebugLoc(I->getDebugLoc());
  //m_Builder->CreateUnreachable();
  for (auto & bb : *I->getParent()->getParent()) {
    if (!bb.getTerminator()) continue;
    if (ReturnInst *ret = dyn_cast<ReturnInst>(bb.getTerminator())) {
      if (ret->getReturnValue())
        m_Builder->CreateRet(ret->getOperand(0));
      else
        m_Builder->CreateRetVoid();
      ret->eraseFromParent();
      m_Builder->SetInsertPoint(&bb);
      m_Builder->CreateBr(retBB);
      phi->addIncoming(m_Builder->getFalse(), &bb);
      break;
    }
  }

  return m_ErrorBB;
}

/// emitBranchToTrap - emit a branch instruction to a trap block.
/// If Cmp is non-null, perform a jump only if its value evaluates to true.
void Speculative::emitBranchToTrap(Instruction *I, Value *Cmp) {
//  // check if the comparison is always false
//  ConstantInt *C = dyn_cast_or_null<ConstantInt>(Cmp);
//  if (C) {
//    //++ChecksSkipped;
//    if (!C->getZExtValue())
//      return;
//    else
//      Cmp = nullptr; // unconditional branch
//  }
//  //++ChecksAdded;

  BasicBlock *OldBB = I->getParent();
  BasicBlock *Cont = OldBB->splitBasicBlock(I);
  OldBB->getTerminator()->eraseFromParent();

  if (Cmp)
    BranchInst::Create(getErrorBB(I), Cont, Cmp, OldBB);
  else
    BranchInst::Create(getErrorBB(I), OldBB);
}

void Speculative::insertSpecCheck(Function &F, Instruction &inst) {

  m_Builder->SetInsertPoint(&inst);
  outs() << "Insertion point set...\n";

//  BasicBlock *OldBB0 = inst.getParent();
//  BasicBlock *Cont0 = OldBB0->splitBasicBlock(B.GetInsertPoint());
//  OldBB0->getTerminator()->eraseFromParent();
//  BranchInst::Create(Cont0, OldBB0);
//
//  B.SetInsertPoint(Cont0->getFirstNonPHI());

//  outs() << "Iterating over specs...\n";
//
//  /// --- spec: add check var_spec == false
//  Value *specOr = ConstantInt::get(m_BoolTy, 0);
//  for (Value *s : S) {
//    specOr = B.CreateBinOp(
//			Instruction::Or,
//			specOr,
//			B.CreateAlignedLoad(s,1));
//  }
//
//  outs() << "Create or of specs...\n";
//
//  Value *specCheck = B.CreateICmpEQ(specOr, ConstantInt::get(m_BoolTy, 0), "spec_check");

  Value *globalSpec = m_Builder->CreateAlignedLoad(m_spec, 1);
  if (FencePlacement == FencePlaceOpt::BEFORE_MEMORY) {
    insertFenceFunction(inst.getModule(), globalSpec);
  }

  //auto assertFn = SBI->mkSeaBuiltinFn(seahorn::SeaBuiltinsOp::ASSERT_NOT, *F.getParent());
  //m_Builder->SetInsertPoint(&inst);
  //Value *ndSpec = m_Builder->CreateCall(m_ndBoolFn, None);
  //m_Builder->CreateCall(assertFn, ndSpec, "");

  emitBranchToTrap(&inst, globalSpec);

//  outs() << "Assertion expression created...\n";
//  BasicBlock *BB = inst.getParent();
//  // Todo: maybe use SplitBlock or its variants
//  BasicBlock *Cont = BB->splitBasicBlock(&inst);
//  Instruction *terminator = BB->getTerminator();
//  B.SetInsertPoint(terminator);
//  Instruction *br = B.CreateCondBr(globalSpec, m_errorBB, Cont);
//  br->setDebugLoc(inst.getDebugLoc());
//  terminator->eraseFromParent();

//  BasicBlock *OldBB1 = Cont0;
//  BasicBlock *Cont1 = OldBB1->splitBasicBlock(B.GetInsertPoint());
//  OldBB1->getTerminator()->eraseFromParent();
//  BranchInst::Create(Cont1, err_spec_bb, specCheck, OldBB1);

  outs() << "Added A SPECULATION check...\n";
}

} // namespace seahorn

namespace seahorn {
llvm::Pass *createSpeculativeExe(bool repair) { return new Speculative(repair, true); }
} // namespace seahorn

static llvm::RegisterPass<seahorn::Speculative> X("speculative-exe",
                                                 "Insert speculative execution logic");
