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

// zhuangh add
#include "llvm/Constants.h"
#include "llvm/InstrTypes.h"
// end of zhuangh

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
  unsigned BBID;
  unsigned ListID;

  errs() << "Analyze Function: " << F.getName() << "\n";

  BBID = 0;
  for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
    BBID++;
  }
  errs() << "Number of BasicBlocks: " << BBID << "\n";

  // BBID = 100;
  std::vector<Instruction*>* RippleLists[100]; //==> need delete later?
  std::vector<Instruction*>* IncrList[100]; //==> need delete later?

  BBID = 0;
  int incrAdderWidth = -1;

  for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
    BasicBlock *BC = dyn_cast<BasicBlock>(BB); //==> how to get an identifier of a BB?
    errs() << "  Basic Block #:" << BBID++ << "\n";


    ListID = 0;
    RippleLists[ListID] = new std::vector<Instruction*>;
    RippleLists[ListID]->clear();

    IncrList[ListID] = new std::vector<Instruction *>;
    IncrList[ListID]->clear();

    for (BasicBlock::iterator IB = BB->begin(), IE = BB->end(); IB != IE; ++IB) {

      // The dyn_cast<> operator is a "checking cast" operation. It checks to
      // see if the operand is of the specified type, and if so, returns a
      // pointer to it (this operator does not work with references). If the
      // operand is not of the correct type, a null pointer is returned.
	Instruction *IT = dyn_cast<Instruction>(IB);
	Instruction *BackIT;
	// Begin Zhuangh: modified the increment index adder

	errs()<<"The !!!!!!! !!!! hahah:  "<<*IT<<"\n";


	PHINode * p_node = dyn_cast<PHINode>(IT);
	if(p_node !=NULL){
	    errs()<<"!\n\n"<<"In Phi!!"<<"\n\n";
	    if( dyn_cast<BinaryOperator>(p_node) )
	    {
		BinaryOperator * calc = dyn_cast<BinaryOperator>(p_node);
		Value *v0 = calc->getOperand(0);
		CastInst * tv0 = new TruncInst(v0, IntegerType::get(p_node->getContext(), 10 ), "", calc);


	    }
	}

	ICmpInst * cmpinst = dyn_cast<ICmpInst>(IT);
	if(cmpinst!=NULL) { // If not a NULL pointer
	    errs()<< "!!!! In CMP Oh yeah!!!!"<<"\n";
	    switch(cmpinst->getPredicate()){
	    case ICmpInst::ICMP_EQ:
	    case ICmpInst::ICMP_ULE: 
	    case ICmpInst::ICMP_SLE: 
	    case ICmpInst::ICMP_ULT: 
	    case ICmpInst::ICMP_SLT:
		{

		    errs()<<"HHHHHH "<<*IT<<"\n";
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
		    //int bb = b.getLimitedValue();

		    // errs()<<a.getBitWidth()<<"\n";
		    incrAdderWidth = (int)ceil(log(aa)/log(2));
		    errs()<<"The flag adder should be "<< incrAdderWidth <<" bit adder\n";
		    errs()<<"QQQQ SLT\n";
		}
		break;
	    default: 
		break;
	    }

	}

	// End of Zhuangh


      // Get Binary Operator Instruction::Add
      if(dyn_cast<BinaryOperator>(IT)) { // If not a NULL pointer
        BinaryOperator* Inst = dyn_cast<BinaryOperator>(IT);

	if (Inst->getOpcode() == Instruction::Add) { //==> OR Mul
	    Value *Op0 = IT->getOperand(0);
          Value *Op1 = IT->getOperand(1);

          //errs() << "Add Value0 ::" << *Op0 << "\n";
          //errs() << "Add Value1 ::" << *Op1 << "\n";

	  errs()<<"!!!!!!!!!!!!!!!!!!!!!!!\n!!!!!!!!!!!!!!"<<IT->getName()<<"\n";
	  errs() <<  ( (IT->getName()).substr(0,8)).equals( "CCIR_inc")<<"\n"; 
	  if( ( (IT->getName()).substr(0,8)).equals( "CCIR_inc")  == 1){

	      IncrList[0]->push_back(IT);
	      errs()<<IncrList[0]->size()<<"\n";

	  }
	  if(RippleLists[0]->empty()) { // List is not initialized yet
            RippleLists[0]->push_back(IT);
          }
          else {
            unsigned dependency = 0;
            unsigned use_times = 0;
            for(unsigned ListIt = 0; ListIt <= ListID; ListIt++) {
              BackIT = dyn_cast<Instruction>(RippleLists[ListIt]->back());
              //errs() << "BackIT :" << *BackIT << "\n";
              for (Instruction::use_iterator UseIt = BackIT->use_begin(); UseIt!= BackIT->use_end(); ++UseIt) use_times++;
              if(BackIT->getName().equals(Op0->getName()) || BackIT->getName().equals(Op1->getName())) {
                if(use_times == 1) RippleLists[ListIt]->push_back(IT);
                dependency++; // ==> Need to deal with (dependency > 1) instructions specially?
              }
            }
            if((!dependency) && (use_times == 1)){
              ListID++;
              RippleLists[ListID] = new std::vector<Instruction*>;
              RippleLists[ListID]->clear();
              RippleLists[ListID]->push_back(IT);
            }
          }
        }
      }
    } // End Instruction iterator

	std::vector<Instruction*> RmList;
	RmList.clear();
	for(std::vector<Instruction*>::iterator iIt = IncrList[0]->begin(); 
	    iIt != IncrList[0]->end(); ++iIt){
	    Instruction *Inst = dyn_cast<Instruction>(*iIt);
	    errs()<<"Output inc adder: "<<*Inst<<"\n";

	    if( dyn_cast<BinaryOperator>(Inst) )
	    {

		BinaryOperator * calc = dyn_cast<BinaryOperator>(Inst);
		Value *v0 = calc->getOperand(0);
		Value *v1 = calc->getOperand(1);

		CastInst * tv0 = new TruncInst(v0, IntegerType::get(Inst->getContext(), incrAdderWidth), "CCIR_inc_tmp", calc);

		CastInst * tv1 = new TruncInst(v1, IntegerType::get(Inst->getContext(), incrAdderWidth), "CCIR_inc_tmp", calc);
//		CastInst * tv0 = new SExtInst(v0, IntegerType::get(Inst->getContext(), incrAdderWidth), "CCIR_inc_tmp", calc);
//		CastInst * tv1 = new SExtInst(v1, IntegerType::get(Inst->getContext(), incrAdderWidth), "CCIR_inc_tmp", calc);
		
		BinaryOperator * newAnd = BinaryOperator::Create(Instruction::Add, tv0, tv1, "CCIR_inc_tmp_add", calc);

		CastInst * casteAnd = new SExtInst(newAnd, IntegerType::get(Inst->getContext(),32 /*incrAdderWidth*/), "CCIR_inc_d",calc);
		errs()<<"Fin inc adder: "<<"\n";	
		calc->replaceAllUsesWith(casteAnd);
		RmList.push_back(Inst);
	        //Inst->removeFromParent();
		//errs() << ">> >> >> Inst: " << *Inst << "\n";
	    }

	}
    for(std::vector<Instruction*>::iterator iIt = RmList.begin(); 
	   iIt != RmList.end(); ++iIt) {
	Instruction *Inst = dyn_cast<Instruction>(*iIt);
	errs() << ">> >> >> Inst: " << *Inst << "\n";
	errs() << ">> >> >> Inst use: " << Inst->use_empty() << "\n";
	//Inst->dropAllReferences();
        //Inst->removeFromParent();

//	BB->getInstList().erase(Inst);
    }


    for(unsigned ListIt = 0; ListIt <= ListID; ListIt++) {
      errs() << "\n    RippleLists[" << ListIt << "] size: " << RippleLists[ListIt]->size() << "\n";
      for(std::vector<Instruction*>::iterator RplIt = RippleLists[ListIt]->begin(); RplIt != RippleLists[ListIt]->end(); ++RplIt) {
        Instruction *Inst = dyn_cast<Instruction>(*RplIt);
        errs() << "      Output dependant operations:" << *Inst << "\n";
        //errs() << "Name:" << Inst->getName().str() << " ";
        //errs() << "Op0:"  << Inst->getOperand(0)->getName().str() << " ";
        //errs() << "Op1:"  << Inst->getOperand(1)->getName().str() << "\n";
        unsigned use_times = 0;
        for (Instruction::use_iterator UseIt = Inst->use_begin(); UseIt!= Inst->use_end(); ++UseIt) {
          use_times++;
        }
        errs() << "        Used times: " << use_times << "\n";
        if(RippleLists[ListIt]->size() > 3) Inst->moveBefore(RippleLists[ListIt]->back());
      }

      // Optimization
      if (RippleLists[ListIt]->size() >= 3) {
        std::vector<Instruction*> NewList;

        while(RippleLists[ListIt]->size() >= 3) {
          int i;
          (&NewList)->clear();

          errs() << "Before for RippleLists[ListIt]->size() = " << RippleLists[ListIt]->size() << "\n";
          for(i = 0; i + 2 < RippleLists[ListIt]->size(); i += 2) {
            Instruction *Inst0 = RippleLists[ListIt]->at(i);
            Instruction *Inst1 = RippleLists[ListIt]->at(i + 1);
            Instruction *Inst2 = RippleLists[ListIt]->at(i + 2);

            Value *Op1_0 = Inst1->getOperand(0);
            Value *Op1_1 = Inst1->getOperand(1);
            Value *Op2_0 = Inst2->getOperand(0);
            Value *Op2_1 = Inst2->getOperand(1);

            if((Inst0->getName().equals(Op1_0->getName()) || Inst0->getName().equals(Op1_1->getName())) &&
               (Inst1->getName().equals(Op2_0->getName()) || Inst1->getName().equals(Op2_1->getName()))) {

              int target1, target2;
              if(Inst0->getName().equals(Op1_0->getName()))
                target1 = 0;
              else
                target1 = 1;

              if(Inst1->getName().equals(Op2_0->getName()))
                target2 = 1;
              else
                target2 = 0;

              Value *temp = Inst2->getOperand(target2);
              Inst2->setOperand(target2, Inst1->getOperand(target1));
              Inst1->setOperand(target1, temp);
              Inst1->moveBefore(Inst2);
              (&NewList)->push_back(Inst2);
            }
          }
          if(i + 2 == RippleLists[ListIt]->size()) (&NewList)->push_back(RippleLists[ListIt]->back());
          *(RippleLists[ListIt]) = NewList;
          errs() << "i = " << i << "\n";
          errs() << "RippleLists[ListIt]->size() = " << RippleLists[ListIt]->size() << "\n";
          for(std::vector<Instruction*>::iterator RplIt = RippleLists[ListIt]->begin(); RplIt != RippleLists[ListIt]->end(); ++RplIt) {
            Instruction *Inst = dyn_cast<Instruction>(*RplIt);
            errs() << "  Output dependant operations:" << *Inst << "\n";
          }
        }
      }
      //delete RippleLists[ListIt]; //==> Need deleting?
    }
    errs() << "\n";
  } // End basic block iterator

  return true;
}

char RippleElimination::ID = 0;
RegisterPass<RippleElimination> PXX("ripple-elim", "Transform ripple operations to tree operations", false, false);

