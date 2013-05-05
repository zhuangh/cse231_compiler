// MemAccessInfo.cpp
// Mem access (store and load) for each basic block
#include "MemAccessInfo.h"

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include <map>
#include <cassert>
#include <cstring>
#include <set>

using namespace llvm;
using namespace arsenal;

char MemAccessInfo::ID = 0;
static RegisterPass<MemAccessInfo> X("memaccess", "Mem access info for each basic block",
    true /* Only looks at CFG */,
    true /* Analysis Pass */);

/// MemAccessInfoBase implementation
/// 

/// print - Print for debugging.
void MemAccessInfoBase::print(raw_ostream &OS) const {
  OS << "store: " << '\n';
  for (const_iterator I = BBStoreSetMap.begin(), E = BBStoreSetMap.end(); I != E; ++I) {
    BasicBlock *block = I->first;
    OS << block->getName() << '\n';
    for (const_inst_iterator II = I->second->begin(), IE = I->second->end(); II != IE; ++II) {
      Instruction *inst = *II;
      OS << inst->getOpcodeName() << '\n';
    }
  }
  OS << "load: " << '\n';
  for (const_iterator I = BBLoadSetMap.begin(), E = BBLoadSetMap.end(); I != E; ++I) {
    BasicBlock *block = I->first;
    OS << block->getName() << '\n';
    for (const_inst_iterator II = I->second->begin(), IE = I->second->end(); II != IE; ++II) {
      Instruction *inst = *II;
      OS << inst->getOpcodeName() << '\n';
    }
  }
}

bool MemAccessInfoBase::addLoad(BasicBlock *block, Instruction *inst) {
  if (BBLoadSetMap.count(block) == 0) {
    // Initialize set for the block
    BBLoadSetMap[block] = new std::set<Instruction *>();
  }
  BBLoadSetMap[block]->insert(inst);
  assert(InstTagMap.count(inst) == 0);
  InstTagMap[inst] = InstTagMap.size();
  return true;
}

bool MemAccessInfoBase::addStore(BasicBlock *block, Instruction *inst) {
  if (BBStoreSetMap.count(block) == 0) {
    // Initialize set for the block
    BBStoreSetMap[block] = new std::set<Instruction *>();
  }
  BBStoreSetMap[block]->insert(inst);
  assert(InstTagMap.count(inst) == 0);
  InstTagMap[inst] = InstTagMap.size();
  return true;
}

bool MemAccessInfoBase::getLoad(BasicBlock *block, std::set<Instruction *> * &loadSet) {
  if (BBLoadSetMap.count(block) == 0) {
    return false;
  }
  loadSet = BBLoadSetMap[block];
  return true;
}

bool MemAccessInfoBase::getStore(BasicBlock *block, std::set<Instruction *> * &storeSet) {
  if (BBStoreSetMap.count(block) == 0) {
    return false;
  }
  storeSet = BBStoreSetMap[block];
  return true;
}

unsigned int MemAccessInfoBase::getTag(Instruction *inst) {
  assert(InstTagMap.count(inst) != 0);
  return InstTagMap[inst];
}
/// MemAccessInfo implementation
///
bool MemAccessInfo::runOnFunction(Function &F) {
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    for (BasicBlock::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {
      Instruction *inst = BI;
      if (inst->mayWriteToMemory()) {
        // store
        InfoBase.addStore(FI, inst);
      }
      else if (inst->mayReadFromMemory()) {
        // load
        InfoBase.addLoad(FI, inst);
      }
    }
  }
  return false;
}

void MemAccessInfo::print(raw_ostream &OS, const Module* M) const {
  InfoBase.print(OS);
}

void MemAccessInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

