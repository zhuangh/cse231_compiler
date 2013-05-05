#include "SetMipsCallConv.h"

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Attributes.h"
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;
using namespace arsenal;

FunctionType* SetMipsCallConv::getFunctionType(Function *F) {
  std::vector<Type *> retParamTypes;
  FunctionType *funcType = F->getFunctionType();
  assert(!funcType->isVarArg() && "Functions with var args will be supported later");
  unsigned argRegID = 0;
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end();
      I != E && argRegID < 4; /* ++I */) {
    Argument *param = I;
    Type *paramType = param->getType();
    if (paramType->isIntegerTy(64)) {
      // Args of type "long long" need padding and take 2 args
      if (argRegID & 1 != 0) {
        // padding
        retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
        argRegID += 1;
        continue;
      }
      // Two Int32 to represent Int64
      retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
      retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
      argRegID += 2;
      ++I;
    }
    else if (paramType->isIntegerTy(8) || paramType->isIntegerTy(16) 
        || paramType->isIntegerTy(32) || paramType->isFloatTy()) {
      // Args of type "char, short, int, float" take 1 arg
      retParamTypes.push_back(paramType);
      argRegID += 1;
      ++I;
    }
    else if (paramType->isPointerTy()) {
      PointerType *ptrType = dyn_cast<PointerType>(paramType);
      Type *elementType = ptrType->getElementType();
      if (elementType->isStructTy() && param->hasByValAttr()) {
        unsigned align = param->getParamAlignment();
        if (align == 8) {
          // padding is needed in this case
          if (argRegID & 1 != 0) {
            // padding
            retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
            argRegID += 1;
            continue;
          }
        }
        StructType *structType = dyn_cast<StructType>(elementType);
 
        // FIXME - structByteSize is not correct!!!!
        const StructLayout *SLO = DataLayout.getStructLayout(structType);
        // Ensure enough arg regs are used for struct
        unsigned structByteSize = (SLO->getSizeInBits() + 7) >> 3;
        // Int32 args are used to pass the struct
        for (unsigned i = 0; i < structByteSize && argRegID < 4; ++i) {
          retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
          ++argRegID;
        }
        ++I;
      }
      else {
        // Args of type "pointer" take 1 arg
        retParamTypes.push_back(paramType);
        argRegID += 1;
        ++I;
      }
    }
    else {
      assert(false && "Only arguments with the type of \"Int8, Int16, Int32, Int64, Pointer\" are supported");
    }
  }

  // If the number of arguments of initial function is less than 4, dummy
  // arguments are added to keep a unique calling convention
  while (argRegID < 4) {
    retParamTypes.push_back(Type::getInt32Ty(F->getContext()));
    ++argRegID;
  }

  // argSP
  retParamTypes.push_back(Type::getInt8PtrTy(F->getContext()));
  // argGP
  retParamTypes.push_back(Type::getInt8PtrTy(F->getContext()));

  ArrayRef<Type *> retParamTypesAR(retParamTypes);
  return FunctionType::get(funcType->getReturnType(), retParamTypesAR, funcType->isVarArg());
}

bool SetMipsCallConv::runOnModule(Module &M) {
  if (M.begin()) {
    Function *F = M.begin();
    ValueToValueMapTy VMap;

    FunctionType *newFTy = getFunctionType(F);
    Function *newF = Function::Create(newFTy, F->getLinkage(), F->getName());

    M.getFunctionList().push_back(newF);

    // Set parameter names for newF
    assert(newF->getFunctionType()->getNumParams() == 6);

    Argument *arg[4];
    Argument *argSP = NULL;
    Argument *argGP = NULL;

    unsigned int argID = 0;
    for (Function::arg_iterator I = newF->arg_begin(), E = newF->arg_end();
        I != E; ++I) {
      switch(argID) {
        case 0:
          I->setName("arg0");
          arg[0] = I;
          break;
        case 1:
          I->setName("arg1");
          arg[1] = I;
          break;
        case 2:
          I->setName("arg2");
          arg[2] = I;
          break;
        case 3:
          I->setName("arg3");
          arg[3] = I;
          break;
        case 4:
          I->setName("argSP");
          argSP = I;
          break;
        case 5:
          I->setName("argGP");
          argGP = I;
          break;
      }
      ++argID;
    }
    assert(argSP != NULL);
    assert(argSP != NULL);

    // Create Basic Block 0 to do insts related with calling conv
    BasicBlock *block0 = BasicBlock::Create(newF->getContext(), "call_conv", newF);

    // Map args in initial function to values in new function
    unsigned argRegID = 0;
    unsigned stackOffset = 0;
    // Map params in old function to params or load insts in new one
    for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
        /* ++I */) {
      Argument *param = I;
      Type *paramType = I->getType();
      if (argRegID < 4) {
        if (paramType->isIntegerTy(64)) {
          if (argRegID & 1 == 1) {
            // Padding
            argRegID += 1;
            stackOffset += 4;
            continue;
          }
          // Cast arg[argRegID], arg[argRegID + 1] to i64
          // new_arg = arg[argID + 1] << 32 | argRegID
          ZExtInst* zExtInstLo = new ZExtInst(arg[argRegID], Type::getInt64Ty(newF->getContext()), "", block0);
          ZExtInst* zExtInstHi = new ZExtInst(arg[argRegID + 1], Type::getInt64Ty(newF->getContext()), "", block0);
          Value *offset = ConstantInt::get(Type::getInt64Ty(newF->getContext()), 32);
          BinaryOperator *shlInst = BinaryOperator::Create(Instruction::Shl, zExtInstHi, offset, "", block0);
          BinaryOperator *orInst = BinaryOperator::Create(Instruction::Or, zExtInstLo, shlInst, "", block0);
          VMap[I] = orInst;
          stackOffset += 8;
          argRegID += 2;
          ++I;
        }
        else if (paramType->isIntegerTy(8) || paramType->isIntegerTy(16) 
            || paramType->isIntegerTy(32) || paramType->isFloatTy()) {
          VMap[I] = arg[argRegID];
          argRegID += 1;
          stackOffset += 4;
          ++I;
        }
        else if (paramType->isPointerTy()) {
          PointerType *ptrType = dyn_cast<PointerType>(paramType);
          Type *elementType = ptrType->getElementType();
          if (elementType->isStructTy() && param->hasByValAttr()) {
            unsigned align = param->getParamAlignment();
            if (align == 8) {
              // padding is needed in this case
              if (argRegID & 1 != 0) {
                // padding
                argRegID += 1;
                stackOffset += 4;
                continue;
              }
            }
            StructType *structType = dyn_cast<StructType>(elementType);
            std::vector<Type *> structElementTypeV;
            for (unsigned i = 0; i < structType->getNumElements(); ++i) {
              structElementTypeV.push_back(structType->getElementType(i));
            }
            const StructLayout *SLO = DataLayout.getStructLayout(structType);
            // Ensure enough arg regs are used for struct
            unsigned structByteSize = (SLO->getSizeInBits() + 7) >> 3;
            Value *structBaseOffset = ConstantInt::get(
                Type::getInt32Ty(newF->getContext()), stackOffset);
            GetElementPtrInst *structBaseAddr = GetElementPtrInst::Create(
                argSP, structBaseOffset, "", block0);
            BitCastInst *castStructBaseAddr = new BitCastInst(structBaseAddr, 
                ptrType, "", block0);
            VMap[I] = castStructBaseAddr;
            // Store the registers in the stack
            for (unsigned i = 0; i < structByteSize && argRegID < 4; ++i) {
              Value *idx = ConstantInt::get(Type::getInt32Ty(newF->getContext()), stackOffset + i << 2);
              GetElementPtrInst *gepInst = GetElementPtrInst::Create(
                  argSP, idx, "", block0);
              Type *destType = PointerType::getUnqual(Type::getInt32Ty(newF->getContext()));
              BitCastInst *bitCastInst = new BitCastInst(gepInst, destType, "",
                  block0);
              StoreInst *storeInst = new StoreInst(arg[argRegID], bitCastInst, false, 4, block0);
              ++argRegID;
            }
            stackOffset += structByteSize;
            stackOffset += ((stackOffset & 3) == 0 ? 0 : (4 - (stackOffset & 3)));
            ++I;
          }
          else {
            // Args of type "pointer" take 1 arg
            VMap[I] = arg[argRegID];
            argRegID += 1;
            stackOffset += 4;
            ++I;
          }
        }
        else {
          assert(false && "Only arguments with the type of Int8, Int16, Int32, \
              Int 64, Float, Pointer are supported");
        }
      }
      else {
        // Create getelementptr and load inst
        if (paramType->isIntegerTy(64) || paramType->isIntegerTy(8) || paramType->isIntegerTy(16) 
            || paramType->isIntegerTy(32) || paramType->isFloatTy()) {
          if (paramType->isIntegerTy(64)) {
            // padding may be needed
            if ((stackOffset & 7) != 0) {
              stackOffset += 8 - (stackOffset & 7);
            }
          }
          Value *idx = ConstantInt::get(Type::getInt32Ty(newF->getContext()), stackOffset);
          GetElementPtrInst *gepInst = GetElementPtrInst::Create(
              argSP, idx, "", block0);
          Type *destType = PointerType::getUnqual(I->getType());
          BitCastInst *bitCastInst = new BitCastInst(gepInst, destType, "",
              block0);
          LoadInst *loadInst = new LoadInst(bitCastInst, I->getName(), block0);
          VMap[I] = loadInst;
          if (paramType->isIntegerTy(64)) {
            stackOffset += 8;
          }
          else {
            stackOffset += 4;
          }
          ++I;
        }
        else if (paramType->isIntegerTy(8) || paramType->isIntegerTy(16) 
            || paramType->isIntegerTy(32) || paramType->isFloatTy()) {
          Value *idx = ConstantInt::get(Type::getInt32Ty(newF->getContext()), stackOffset);
          GetElementPtrInst *gepInst = GetElementPtrInst::Create(
              argSP, idx, "", block0);
          Type *destType = PointerType::getUnqual(I->getType());
          BitCastInst *bitCastInst = new BitCastInst(gepInst, destType, "",
              block0);
          LoadInst *loadInst = new LoadInst(bitCastInst, I->getName(), block0);
          VMap[I] = loadInst;
          stackOffset += 4;
          ++I;
        }
        else if (paramType->isPointerTy()) {
          PointerType *ptrType = dyn_cast<PointerType>(paramType);
          Type *elementType = ptrType->getElementType();
          if (elementType->isStructTy() && param->hasByValAttr()) {
            unsigned align = param->getParamAlignment();
            if (align == 8) {
              // padding is needed in this case
              if ((stackOffset & 7) != 0) {
                stackOffset += 8 - (stackOffset & 7);
              }
            }
            StructType *structType = dyn_cast<StructType>(elementType);
            std::vector<Type *> structElementTypeV;
            for (unsigned i = 0; i < structType->getNumElements(); ++i) {
              structElementTypeV.push_back(structType->getElementType(i));
            }
            // FIXME - structByteSize is not correct!!!!
            const StructLayout *SLO = DataLayout.getStructLayout(structType);
            // Ensure enough arg regs are used for struct
            unsigned structByteSize = (SLO->getSizeInBits() + 7) >> 3;
            Value *structBaseOffset = ConstantInt::get(
                Type::getInt32Ty(newF->getContext()), stackOffset);
            GetElementPtrInst *structBaseAddr = GetElementPtrInst::Create(
                argSP, structBaseOffset, "", block0);
            BitCastInst *castStructBaseAddr = new BitCastInst(structBaseAddr, 
                ptrType, "", block0);
            VMap[I] = castStructBaseAddr;
            stackOffset += structByteSize;
            // Padding
            stackOffset += ((stackOffset & 3) == 0 ? 0 : (4 - (stackOffset & 3)));
            ++I;
          }
          else {
            // Args of type "pointer" take 1 arg
            Value *idx = ConstantInt::get(Type::getInt32Ty(newF->getContext()), stackOffset);
            GetElementPtrInst *gepInst = GetElementPtrInst::Create(
                argSP, idx, "", block0);
            Type *destType = PointerType::getUnqual(I->getType());
            BitCastInst *bitCastInst = new BitCastInst(gepInst, destType, "",
                block0);
            LoadInst *loadInst = new LoadInst(bitCastInst, I->getName(), block0);
            VMap[I] = loadInst;
            stackOffset += 4;
            ++I;
          }
        }
        else {
          assert(false && "Only arguments with the type of Int8, Int16, Int32, \
              Int 64, Float, Pointer are supported");
        }
      }
    }

    errs() << "Finish adding block 0\n";

    
    bool moduleLevelChanges=false; // fix used-uninitialized warning
    SmallVector<ReturnInst*, 8> Returns;
    CloneFunctionInto(newF, F, VMap, moduleLevelChanges, Returns, "", NULL);
  
    // Add unconditional BranchInst to block 0
    BranchInst *brInst = BranchInst::Create(dyn_cast<BasicBlock>(VMap[&(F->getEntryBlock())]), block0);

    // Remove all the uses of the old function

    //M.getFunctionList().erase(F);
    errs() << "Finish cloning and erasing original function\n";
    std::string nameTmp=F->getName();
    F->setName("UNUSEDOLDFUNCTIONNEVERUSEMEWHYAMIYELLINGICANNOTSTOP");
    newF->setName(nameTmp);
  }
  return true; // Module has been modified
}

char SetMipsCallConv::ID = 0;

static RegisterPass<SetMipsCallConv> X("mips_call_conv", "Modify function signatures and add load instructions to satisfy mips calling convention", false, false);
