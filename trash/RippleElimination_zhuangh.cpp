#define DEBUG_TYPE "RippleElimination"

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/Dominators.h"

#include "llvm/Constants.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <map>

using namespace llvm;

using std::set;
using std::map;
using std::vector;
using std::pair;
using std::string;

class RippleElimination : public FunctionPass {

public:
  static char ID;
  RippleElimination() : FunctionPass(ID) {}
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesCFG();
  }
  bool runOnFunction(Function &F);
};

bool RippleElimination::runOnFunction(Function &F) {
  std::vector<Instruction*> **RippleLists;
  unsigned ListID = 0;
  unsigned NumofLists = 0;

  RippleLists = new std::vector<Instruction*>*;
  RippleLists[ListID] = new std::vector<Instruction*>;
  RippleLists[ListID]->clear();

  //errs() << "RippleLists[0] size:" << RippleLists[0]->size() << "\n";

  for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
    BasicBlock *BC = dyn_cast<BasicBlock>(BB); //==> how to get an identifier of a BB?
    for (BasicBlock::iterator IB = BB->begin(), IE = BB->end(); IB != IE; ++IB) {

      // The dyn_cast<> operator is a "checking cast" operation. It checks to
      // see if the operand is of the specified type, and if so, returns a
      // pointer to it (this operator does not work with references). If the
      // operand is not of the correct type, a null pointer is returned.
      Instruction *IT = dyn_cast<Instruction>(IB);
      Instruction *BackIT;
      ICmpInst * cmpinst = dyn_cast<ICmpInst>(IT);
      if(cmpinst!=NULL) { // If not a NULL pointer
	  	  errs()<< "!!!! In CMP Oh yeah!!!!"<<"\n";
	  switch(cmpinst->getPredicate()){
	  case ICmpInst::ICMP_SLE: 
	      {
		  errs()<<"QQQQ SLE\n";
	      }
	      break;
	  case ICmpInst::ICMP_SLT:
	      {

		  Value *op0 = IT->getOperand(0);
		  Instruction * op0_inst =  dyn_cast<Instruction> (op0);
		  Value *op0_op1 = op0_inst->getOperand(1);
		  ConstantInt *op0_op1_const = dyn_cast<ConstantInt>(op0_op1);
		  APInt b = op0_op1_const->getValue();

		  ConstantInt *op1 = dyn_cast<ConstantInt>(IT->getOperand(1));

		  APInt  a = op1->getValue(); 
		  errs() << "CMP Value0 ::" << b.getLimitedValue() << "\n";
		  errs() << "CMP Value1 ::" << a.getLimitedValue() << "\n";
		  int aa = a.getLimitedValue();
		  int bb = b.getLimitedValue();

		  // errs()<<a.getBitWidth()<<"\n";

		  errs()<<"The flag adder should be "<<(int)ceil(log(aa)/log(2))<<" bit adder\n";
		  errs()<<"QQQQ SLT\n";
	      }
	      break;
	  default: 
	      break;
	  }

      }
      // Get Binary Operator Instruction::Add
      if(dyn_cast<BinaryOperator>(IT)) { // If not a NULL pointer
        BinaryOperator* Inst = dyn_cast<BinaryOperator>(IT);

//	errs()<<"Zhuangh put: "<<Inst->getOpcode()<<"\n";

        if (Inst->getOpcode() == Instruction::Add) {
          Value *Op0 = IT->getOperand(0);
          Value *Op1 = IT->getOperand(1);

          //errs() << "Add Value0 ::" << *Op0 << "\n";
          //errs() << "Add Value1 ::" << *Op1 << "\n";
          if(RippleLists[0]->empty()) {
            RippleLists[0]->push_back(IT);
          }
          else {
            BackIT = dyn_cast<Instruction>(RippleLists[0]->back());
            //errs() << "BackIT :" << *BackIT << "\n";
            if(BackIT->getName().equals(Op0->getName()) || BackIT->getName().equals(Op1->getName())) {
              RippleLists[0]->push_back(IT);
            }
          }
        }
      }
    } // End Instruction iterator

    if(RippleLists[0]->size() < 2) RippleLists[0]->clear(); // No ripple detected

    for(std::vector<Instruction*>::iterator RplIT = RippleLists[0]->begin(); RplIT != RippleLists[0]->end(); ++RplIT){
      Instruction *Inst = dyn_cast<Instruction>(*RplIT);
      errs() << "Ripple Operations:" << *Inst << "\n";
      //errs() << "Name:" << Inst->getName().str() << " ";
      //errs() << "Op0:" << Inst->getOperand(0)->getName().str() << " ";
      //errs() << "Op1:" << Inst->getOperand(1)->getName().str() << "\n";
      //It->eraseFromParent();
    }
    //errs() << "RippleLists[0] size:" << RippleLists[0]->size() << "\n";
    errs() << "\n";
    RippleLists[0]->clear();
  } // End basic block iterator

  return true;
}

char RippleElimination::ID = 0;
RegisterPass<RippleElimination> PXX("ripple-elim", "Transform ripple operations to tree operations", false, false);

