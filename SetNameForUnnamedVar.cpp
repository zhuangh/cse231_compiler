#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Type.h"
#include <sstream>
#include <string>

using namespace llvm;

namespace {
class SetNameForUnnamedVar : public FunctionPass {
public:
  static char ID;
  SetNameForUnnamedVar() : FunctionPass(ID) { }

  void replaceAll(std::string &str, const std::string &from, const std::string &to) {
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
      str.replace(startPos, from.length(), to);
      startPos += to.length();
    }
  }

  virtual bool runOnFunction (Function &F) {
    int varID = 0;
    std::stringstream SS;
 //   int v = 0; 
    for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {
      for (BasicBlock::iterator BI = I->begin(), BE = I->end(); BI != BE; ++BI) {
        Instruction *inst = BI;
        if (!inst->hasName()) {
          if (inst->getType()->isVoidTy()) {
            continue;
          }
          SS.clear(); SS.str("");
          SS << "local_" << varID++;
          inst->setName(SS.str());
        }
        else {
          std::string name = inst->getName().str();
          errs() << "Convert " << name << " To ";
          replaceAll(name, "_", "__");
          replaceAll(name, ".", "_d");
          inst->setName(name);
          errs() << name << '\n';
        }	
//	SS.clear(); SS.str("");
//	SS<<v<<"_CCIR_";
         inst->setName("CCIR_" + inst->getName().str());
        //inst->setName(SS.str() + inst->getName().str());
//	cout<<"The instruction #"<<v<<endl;
      }

    }

    return false;
  }
};
}

char SetNameForUnnamedVar::ID = 0;
static RegisterPass<SetNameForUnnamedVar> X("name_var", "Set names for unnamed temporary variables", false, true);
