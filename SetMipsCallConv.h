#ifndef MIPS_CALL_CONV_H
#define MIPS_CALL_CONV_H

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
#include "llvm/Target/TargetData.h"
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;

namespace arsenal {
class SetMipsCallConv : public ModulePass {
public:
  static char ID;
  SetMipsCallConv(): ModulePass(ID),
  // ATTENTION: Needs to be consistent with the data layout in backend
  DataLayout("e-p:32:32:32-i8:8:32-i16:16:32-i64:64:64-n32")
  { }

  FunctionType *getFunctionType(Function *F);

  virtual bool runOnModule (Module &M);
private:
  // DataLayout is used to calculate the size of struct-typed arguments
  const TargetData DataLayout;
}; // end of class SetMipsCallConv

} // end of namespace arsenal
#endif
