// LabelBasicBlocks.cpp
// Assign a unique block id to all the basic blocks.
#include "BlockIDInfo.h"

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "ErrorMessage.h"

#include <map>
#include <cassert>

using namespace llvm;
using namespace arsenal;

char BlockIDInfo::ID = 0;
static RegisterPass<BlockIDInfo> X("idinfo", "Block ID Info Pass",
    true /* Only looks at CFG */,
    true /* Analysis Pass */);

/// BlockIDInfoBase implementation
/// 

/// print - Print BlockToIDMap for debugging.
void BlockIDInfoBase::print(raw_ostream &OS) const {
  OS << "Block To ID Map" << '\n';
  for (std::map<const BasicBlock *, int>::const_iterator I = BlockToIDMap.begin(),
      E = BlockToIDMap.end(); I != E; ++I) {
    OS << I->first->getName() << '\t' << I->second << '\n';
  }
}

/// getID - blockID = BlockToIDMap[block], return true if successful, false
/// otherwise
int BlockIDInfoBase::getID(const BasicBlock *block) const {
  if (BlockToIDMap.count(block) == 0) {
    assert(false && "Cannot find the basicblock ptr in hash");
  }
  int blockID = BlockToIDMap.find(block)->second;
  return blockID;
}

/// addBlockToIDPair - add name to id mapping to both BlockToIDMap and
/// IDToBlockMap, return true if successful, false otherwise
bool BlockIDInfoBase::addBlockToIDPair(const BasicBlock *block, int blockID) {
  if (BlockToIDMap.count(block) > 0)
    return false;
  BlockToIDMap[block] = blockID;
  if (blockID > MaxID) {
    MaxID = blockID;
  }
  return true;
}

bool BlockIDInfoBase::addInstToIDPair(const Instruction *inst, int inBlockID) {
  if (InstToInBlockIDMap.count(inst) > 0) {
    return false;
  }
  InstToInBlockIDMap[inst] = inBlockID;
  return true;
}
bool BlockIDInfoBase::getInstInBlockID(const Instruction *inst, int &inBlockID) const {
  if (InstToInBlockIDMap.count(inst) == 0) {
    return false;
  }
  inBlockID = InstToInBlockIDMap.find(inst)->second;
  return true;
}

/// BlockIDInfo implementation
///
bool BlockIDInfo::runOnFunction(Function &F) {
  int blockCounter = 0;
  for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
    ++blockCounter;
    assert(I->hasName() && "Every Block has a name");
    if(!IDInfoBase.addBlockToIDPair(I, blockCounter)) {
      PrintError(errs());
    }
    int instCounter = 0;
    for (BasicBlock::iterator BI = I->begin(), BE = I->end(); BI != BE; ++BI) {
      IDInfoBase.addInstToIDPair(BI, instCounter);
      ++instCounter;
    }
  }
  return false;
}

void BlockIDInfo::print(raw_ostream &OS, const Module* M) const {
  IDInfoBase.print(OS);
}

void BlockIDInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

