#ifndef BLOCK_ID_INFO_H
#define BLOCK_ID_INFO_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

using namespace llvm;

namespace arsenal {

class BlockIDInfoBase;
class BlockIDInfo;

class BlockIDInfoBase {
  public:
    BlockIDInfoBase(): MaxID(-1) { }
    ~BlockIDInfoBase() { releaseMemory(); }
    
    void releaseMemory() { }

    void print(raw_ostream &OS) const;

    int getID(const BasicBlock *block) const;
    int getMaxID() {
      assert(MaxID >= 0);
      return MaxID;
    }
    bool addBlockToIDPair(const BasicBlock *block, int blockID);
    bool addInstToIDPair(const Instruction *inst, int inBlockID);
    bool getInstInBlockID(const Instruction *inst, int &inBlockID) const;

  private:
    std::map<const BasicBlock *, int> BlockToIDMap;
    std::map<const Instruction *, int> InstToInBlockIDMap;

    void operator=(const BlockIDInfoBase &); // do not implement
    BlockIDInfoBase(const BlockIDInfoBase &); // do not implement
    int MaxID;

}; // end of class BlockIDInfoBase

class BlockIDInfo: public FunctionPass {
public:
  static char ID;
  BlockIDInfo(): FunctionPass(ID) {}

  BlockIDInfoBase& getBase() { return IDInfoBase; }

  virtual bool runOnFunction(Function &F);

  virtual void releaseMemory() { IDInfoBase.releaseMemory(); }

  virtual void print(raw_ostream &OS, const Module* M = 0) const;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  BlockIDInfoBase IDInfoBase;
}; // end of class BlockIDInfo

} // end of arsenal namespace
#endif
