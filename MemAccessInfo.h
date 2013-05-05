#ifndef MEM_ACCESS_INFO_H
#define MEM_ACCESS_INFO_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <map>

using namespace llvm;

namespace arsenal {

class MemAccessInfoBase;
class MemAccessInfo;

class MemAccessInfoBase {
  public:
    typedef std::map<BasicBlock *, std::set<Instruction *> *>::iterator iterator;
    typedef std::set<Instruction *>::iterator inst_iterator;
    typedef std::map<BasicBlock *, std::set<Instruction *> *>::const_iterator const_iterator;
    typedef std::set<Instruction *>::const_iterator const_inst_iterator;
    MemAccessInfoBase() { }
    ~MemAccessInfoBase() { releaseMemory(); }
    
    void releaseMemory() {

    }
    void print(raw_ostream &OS) const;
    bool addLoad(BasicBlock *block, Instruction *inst);
    bool addStore(BasicBlock *block, Instruction *inst);
    bool getLoad(BasicBlock *block, std::set<Instruction *> * &loadSet);
    bool getStore(BasicBlock *block, std::set<Instruction *> * &storeSet);
    unsigned int getTag(Instruction *inst);

  private:
    std::map<BasicBlock *, std::set<Instruction *> *> BBStoreSetMap;
    std::map<BasicBlock *, std::set<Instruction *> *> BBLoadSetMap;
    std::map<Instruction *, unsigned int> InstTagMap;

    void operator=(const MemAccessInfoBase &); // do not implement
    MemAccessInfoBase(const MemAccessInfoBase &); // do not implement

}; // end of class MemAccessInfoBase

class MemAccessInfo: public FunctionPass {
public:
  static char ID;
  MemAccessInfo(): FunctionPass(ID) {}

  MemAccessInfoBase& getBase() { return InfoBase; }

  virtual bool runOnFunction(Function &F);

  virtual void releaseMemory() { InfoBase.releaseMemory(); }

  virtual void print(raw_ostream &OS, const Module* M = 0) const;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  MemAccessInfoBase InfoBase;
}; // end of class MemAccessInfo

} // end of arsenal namespace
#endif
