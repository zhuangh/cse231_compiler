#ifndef LIVENESS_INFO_H
#define LIVENESS_INFO_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <map>

using namespace llvm;

namespace arsenal {

class LivenessInfoBase;
class LivenessInfo;

class LivenessInfoBase {
  public:
    typedef std::map<const BasicBlock *, std::set<Value *> *>::iterator iterator;
    typedef std::map<const BasicBlock *, std::set<Value *> *>::const_iterator const_iterator;
    typedef std::set<Value *>::iterator value_iterator;
    typedef std::set<Value *>::const_iterator const_value_iterator;

    LivenessInfoBase() { }
    ~LivenessInfoBase() { releaseMemory(); }
    
    void releaseMemory() {

    }
    void print(raw_ostream &OS) const;
    bool addLiveIn(const BasicBlock *block, Value *value);
    bool addLiveOut(const BasicBlock *block, Value *value);
    bool getLiveIn(const BasicBlock *block, std::set<Value *> * &valueSet);
    bool getLiveOut(const BasicBlock *block, std::set<Value *> * &valueSet);

  private:
    std::map<const BasicBlock *, std::set<Value *> *> BBLiveInSetMap;
    std::map<const BasicBlock *, std::set<Value *> *> BBLiveOutSetMap;

    void operator=(const LivenessInfoBase &); // do not implement
    LivenessInfoBase(const LivenessInfoBase &); // do not implement

}; // end of class LivenessInfoBase

class LivenessInfo: public FunctionPass {
public:
  static char ID;
  LivenessInfo(): FunctionPass(ID) {}

  LivenessInfoBase& getBase() { return InfoBase; }

  virtual bool runOnFunction(Function &F);

  virtual void releaseMemory() { InfoBase.releaseMemory(); }

  virtual void print(raw_ostream &OS, const Module* M = 0) const;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  LivenessInfoBase InfoBase;
}; // end of class LivenessInfo

} // end of arsenal namespace
#endif
