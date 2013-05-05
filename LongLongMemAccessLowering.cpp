#include "LongLongMemAccessLowering.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <vector>

using namespace arsenal;
using namespace llvm;

///
/// Implementation of LongLongMemAccessLowering
///

///
/// runOnFunction 
///
bool LongLongMemAccessLowering::runOnFunction(Function &F) {
  std::vector<StoreInst *> storeInstV;
  std::vector<LoadInst *> loadInstV;

  for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
    for (BasicBlock::iterator BI = I->begin(), BE = I->end(); BI != BE; ++BI) {
      Instruction *inst = BI;
      if (StoreInst *storeInst = dyn_cast<StoreInst>(inst)) {
        if (!storeInst->getValueOperand()->getType()->isIntegerTy(64)) {
          continue;
        }
        assert(cast<PointerType>(storeInst->getPointerOperand()->getType())
            ->getElementType()->isIntegerTy(64));
        storeInstV.push_back(storeInst);
      }
      else if (LoadInst *loadInst = dyn_cast<LoadInst>(inst)) {
        if (!loadInst->getType()->isIntegerTy(64)) {
          continue;
        }
        assert(cast<PointerType>(loadInst->getPointerOperand()->getType())
            ->getElementType()->isIntegerTy(64));
        loadInstV.push_back(loadInst);
      }
    }
  }

  for (unsigned i = 0; i < storeInstV.size(); ++i) {
    StoreInst *inst = storeInstV.at(i);
    Value *storeVal = inst->getValueOperand();
    Value *storePointer = inst->getPointerOperand();

    // Insert new instructions
    BitCastInst *storeAddrLo = new BitCastInst(storePointer, Type::getInt32PtrTy(F.getContext()), "", inst);
    ConstantInt *offsetConst = ConstantInt::getSigned(Type::getInt32Ty(F.getContext()), 1);
    std::vector<Value *> gepIdxList(1, offsetConst);
    GetElementPtrInst *storeAddrHi = GetElementPtrInst::Create(storeAddrLo, gepIdxList, "", inst);
    TruncInst *storeValLo = new TruncInst(storeVal, Type::getInt32Ty(F.getContext()), "", inst);
    Value *aShrOffset = ConstantInt::getSigned(Type::getInt64Ty(F.getContext()), 32);
    BinaryOperator *storeValAShr = BinaryOperator::Create(Instruction::AShr, storeVal,
        aShrOffset, "", inst);
    TruncInst *storeValHi = new TruncInst(storeValAShr, Type::getInt32Ty(F.getContext()), "", inst);

    StoreInst *storeLo = new StoreInst(storeValLo, storeAddrLo, inst);
    StoreInst *storeHi = new StoreInst(storeValHi, storeAddrHi, inst);
    storeLo->setAlignment(4);
    storeHi->setAlignment(4);
    
    // Remove inst
    inst->eraseFromParent();
  }

  for (unsigned i = 0; i < loadInstV.size(); ++i) {
    LoadInst *inst = loadInstV.at(i);
    Value *loadPointer = inst->getPointerOperand();

    // Insert new instructions
    BitCastInst *loadAddrLo = new BitCastInst(loadPointer, Type::getInt32PtrTy(F.getContext()), "", inst);
    ConstantInt *offsetConst = ConstantInt::getSigned(Type::getInt32Ty(F.getContext()), 1);
    std::vector<Value *> gepIdxList(1, offsetConst);
    GetElementPtrInst *loadAddrHi = GetElementPtrInst::Create(loadAddrLo, gepIdxList, "", inst);

    LoadInst *loadLo = new LoadInst(loadAddrLo, "", inst);
    LoadInst *loadHi = new LoadInst(loadAddrHi, "", inst);

    ZExtInst *loadLoLL = new ZExtInst(loadLo, Type::getInt64Ty(F.getContext()), "", inst);
    ZExtInst *loadHiLL = new ZExtInst(loadHi, Type::getInt64Ty(F.getContext()), "", inst);

    Value *shlOffset = ConstantInt::getSigned(Type::getInt64Ty(F.getContext()), 32);
    BinaryOperator *loadHiLLShl = BinaryOperator::Create(Instruction::Shl, loadHiLL, shlOffset, "", inst);
    BinaryOperator *loadValue = BinaryOperator::Create(Instruction::Or, loadLoLL, loadHiLLShl, "");

    // Replace inst with new "loaded" value, the old value is deleted
    ReplaceInstWithInst(inst, loadValue);
  }

  return true; // function is modified
}

char LongLongMemAccessLowering::ID = 0;

static RegisterPass<LongLongMemAccessLowering> X("long_long_lo", 
    "Lowering i64 store to two i32 store, lowering i64 load to two i32 load.", 
    false,  // not CFGOnly
    false); // not is_analysis
