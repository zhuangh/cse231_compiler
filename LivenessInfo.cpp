// LivenessInfo.cpp
// Live-in and live-out variables for each basic block
#include "LivenessInfo.h"

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Instructions.h"
#include "llvm/GlobalVariable.h"

#include <map>
#include <cassert>

using namespace llvm;
using namespace arsenal;

char LivenessInfo::ID = 0;
static RegisterPass<LivenessInfo> X("live", "Liveness Info Pass",
    true /* Only looks at CFG */,
    true /* Analysis Pass */);

/// LivenessInfoBase implementation
/// 

/// print - Print for debugging.
void LivenessInfoBase::print(raw_ostream &OS) const {
  OS << "live in: " << '\n';
  for (const_iterator I = BBLiveInSetMap.begin(), E = BBLiveInSetMap.end(); I != E; ++I) {
    const BasicBlock *block = I->first;
    OS << block->getName() << '\n';
    for (const_value_iterator VI = I->second->begin(), VE = I->second->end(); VI != VE; ++VI) {
      Value *value = *VI;
      OS << value->getName() << '\n';
    }
    OS << '\n';
  }
  OS << "live out: " << '\n';
  for (const_iterator I = BBLiveOutSetMap.begin(), E = BBLiveOutSetMap.end(); I != E; ++I) {
    const BasicBlock *block = I->first;
    OS << block->getName() << '\n';
    for (const_value_iterator VI = I->second->begin(), VE = I->second->end(); VI != VE; ++VI) {
      Value *value = *VI;
      OS << value->getName() << '\n';
    }
    OS << '\n';
  }
}
bool LivenessInfoBase::addLiveIn(const BasicBlock *block, Value *value) {
  if (BBLiveInSetMap.count(block) == 0) {
    BBLiveInSetMap[block] = new std::set<Value *>();
  }
  BBLiveInSetMap[block]->insert(value);
  return true;
}

bool LivenessInfoBase::addLiveOut(const BasicBlock *block, Value *value) {
  if (BBLiveOutSetMap.count(block) == 0) {
    BBLiveOutSetMap[block] = new std::set<Value *>();
  }
  BBLiveOutSetMap[block]->insert(value);
  return true;
}

bool LivenessInfoBase::getLiveIn(const BasicBlock *block, std::set<Value *> * &valueSet) {
  if (BBLiveInSetMap.count(block) == 0) {
    return false;
  }
  valueSet = BBLiveInSetMap[block];
  return true;
}

bool LivenessInfoBase::getLiveOut(const BasicBlock *block, std::set<Value *> * &valueSet) {
  if (BBLiveOutSetMap.count(block) == 0) {
    return false;
  }
  valueSet = BBLiveOutSetMap[block];
  return true;
}

/// LivenessInfo implementation
///
bool LivenessInfo::runOnFunction(Function &F) {
  // Live out set
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    for (BasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {
      if (BI->isUsedOutsideOfBlock(FI)) {
        InfoBase.addLiveOut(FI, BI);
      }
//      // Add phi target to the live out set of predecessor basic block
//      if (PHINode *phiInst = dyn_cast<PHINode>(BI)) {
//        for (unsigned int i = 0; i < phiInst->getNumIncomingValues(); ++i) {
//          BasicBlock * block = phiInst->getIncomingBlock(i);
//          InfoBase.addLiveOut(block, phiInst);
//        }
//      }
    }
  }
  // Live in set
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    for (BasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {
      if (isa<PHINode>(BI)) {
      }
      else {
        for (User::op_iterator OI = BI->op_begin(), OE = BI->op_end(); OI != OE; ++OI) {
          Value *opValue = *OI;
          if (Instruction *defInst = dyn_cast<Instruction>(opValue)) {
            if (defInst->getParent() != FI) {
              // opValue is defined in other basic blocks
              InfoBase.addLiveIn(FI, opValue);
            }
          }
          else if (Argument *argument = dyn_cast<Argument>(opValue)) {
            InfoBase.addLiveIn(FI, argument);
          }
          else if (GlobalVariable *gv = dyn_cast<GlobalVariable>(opValue)) {
            InfoBase.addLiveIn(FI, gv);
          }
        }
      }
    }
  }
  return false;
}

void LivenessInfo::print(raw_ostream &OS, const Module* M) const {
  InfoBase.print(OS);
}

void LivenessInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

