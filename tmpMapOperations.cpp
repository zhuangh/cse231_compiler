// MapOperations.cpp
// Map LLVM IR operation to CCIR operation

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Type.h"
using namespace llvm;

namespace {
  // Label all the basic blocks in the source code 
  struct MapOperations: public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    MapOperations() : FunctionPass(ID) {
      
    }

    virtual bool runOnFunction(Function &F) {
      for (Function::iterator fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
        for (BasicBlock::iterator bj = fi->begin(), be = fi->end(); bj != be; ++bj) {
          outs() << bj->getOpcodeName() << '\t' << bj->getNumOperands() << '\t' << bj->getOperand(0)->getName() 
            << '\t' << bj->getOperand(0)->getType()->getTypeID() << '\n';
        }
      }
      return false;
    }
  };
}

char MapOperations::ID = 0;
static RegisterPass<MapOperations> X("mapop", "Map LLVM IR Operation to CCIR Operation Pass");
