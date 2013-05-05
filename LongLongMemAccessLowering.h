#ifndef LONG_LONG_MEM_ACCESS_LOWERING_H
#define LONG_LONG_MEM_ACCESS_LOWERING_H

#include "llvm/Function.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace arsenal {
class LongLongMemAccessLowering : public FunctionPass {
  public:
    static char ID;
    LongLongMemAccessLowering() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function &F);
  protected:
  private:
}; // end of class LongLongMemAccessLowering
} // end of namespace arsenal





#endif
