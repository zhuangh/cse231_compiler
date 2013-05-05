#define DEBUG_TYPE "indvarbits-reduce"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

class IndVarBitsReduction : public LoopPass {
  public:
    static char ID;
    IndVarBitsReduction() : LoopPass(ID) {}

    bool runOnLoop(Loop* L, LPPassManager& LPM);

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<LoopInfo>();
      AU.addRequired<ScalarEvolution>();
      AU.addRequiredID(LoopSimplifyID);
      AU.addRequiredID(LCSSAID);

      AU.addPreserved<ScalarEvolution>();
      AU.addPreserved<DominatorTree>();
      AU.addPreserved<LoopInfo>();
      AU.addPreservedID(LoopSimplifyID);
      AU.addPreservedID(LCSSAID);
    }
};

bool IndVarBitsReduction::runOnLoop(Loop* L, LPPassManager& LPM) {

  bool boolChanged = false;

  PHINode *phiCanonicalIV = L->getCanonicalInductionVariable();
  errs()<<"phiCanonical: ---> "<<*phiCanonicalIV<<endl;
  unsigned int uintPhiNumInVals;

  if(!phiCanonicalIV) {
    errs() << "\n>> >> PASS EXITS: Current loop has no canonical induction variable is found << <<\n";
    return false;
  }
  else {
    uintPhiNumInVals = phiCanonicalIV->getNumIncomingValues();
    if(uintPhiNumInVals != 2) {
      errs() << "\n>> >> PASS EXITS: Current loop PHI node has more than 2 incoming edges << <<\n";
      return false;
    } 
  }

  errs() << ">> >> phiCanonicalIV->getName(): " << phiCanonicalIV->getName().str() << "\n";
  errs() << ">> >> phiCanonicalIV->getOperand(0): " << *(phiCanonicalIV->getOperand(0)) << "\n";
  errs() << ">> >> phiCanonicalIV->getOperand(1): " << *(phiCanonicalIV->getOperand(1)) << "\n";
  errs() << ">> >> phiCanonicalIV->getIncomingValue(0): " << *(phiCanonicalIV->getIncomingValue(0)) << "\n";
  errs() << ">> >> phiCanonicalIV->getIncomingValue(1): " << *(phiCanonicalIV->getIncomingValue(1)) << "\n";
  //errs() << ">> >> phiCanonicalIV->getIncomingBlock(0): " << *(phiCanonicalIV->getIncomingBlock(0)) << "\n";
  //errs() << ">> >> phiCanonicalIV->getIncomingBlock(1): " << *(phiCanonicalIV->getIncomingBlock(1)) << "\n";

  BranchInst *insBr = cast<BranchInst>(L->getExitingBlock()->getTerminator()); // Get the br instruction
  Value *valBrCond = insBr->getCondition(); // Get the br condition
  errs() << ">> >> insBr: " << *L->getExitingBlock() <<"\n";
  errs() << ">> >> valBrCond: " << *valBrCond <<"\n";

  std::vector<Instruction*> *lstTruncInsts = new std::vector<Instruction*>;
  lstTruncInsts->clear();

  ICmpInst *insCmp = dyn_cast<ICmpInst>(valBrCond);
  if(insCmp) { //Proceed if not NULL
    Value *valCmpLHS = insCmp->getOperand(0);
    Value *valCmpRHS = insCmp->getOperand(1);
    Value *valCmpVar = NULL;
    Value *valCmpConst = NULL;
    Instruction *insCmpVar = NULL;
    ConstantInt *cniCmpConst = NULL;
    ConstantInt *cniPHIConst = NULL;
    unsigned int uintPHIConstId;

    //errs() << ">> >> >> insCmp Operands: " << insCmp->getNumOperands() <<"\n";
    //errs() << ">> >> >> insCmp Lhs: " << valCmpLHS->getName() <<"\n";

    if(dyn_cast<ConstantInt>(valCmpLHS)) {
      valCmpConst = valCmpLHS;
      if(!dyn_cast<ConstantInt>(valCmpRHS)) valCmpVar = valCmpRHS;
    }
    else if(dyn_cast<ConstantInt>(valCmpRHS)) {
      valCmpConst = valCmpRHS;
      if(!dyn_cast<ConstantInt>(valCmpLHS)) valCmpVar = valCmpLHS;
    }
    cniCmpConst = dyn_cast<ConstantInt>(valCmpConst);
    insCmpVar = dyn_cast<Instruction>(valCmpVar);
    // use dyn_cast<Instruction> to get the previous usage !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    
    while(insCmpVar->getOpcode() == Instruction::Trunc) { // Find relevant trunc OPs, which will be removed later.
      lstTruncInsts->push_back(insCmpVar);
      insCmpVar = dyn_cast<Instruction>(insCmpVar->getOperand(0));
    }

    // insCmpVar is supposed to be a binary operation now
    if(!insCmpVar) {
      errs() << "\n>> >> PASS EXITS: Branching conditional variable is produced in an unknown way! << <<\n";
      return false;
    }
    else {
      if( insCmpVar->getName().equals(phiCanonicalIV->getOperand(0)->getName()) ) {
        if( dyn_cast<ConstantInt>(phiCanonicalIV->getIncomingValue(1)) ) {
          cniPHIConst = dyn_cast<ConstantInt>(phiCanonicalIV->getIncomingValue(1));
          uintPHIConstId = 1;
        }
        else {
          errs() << "\n>> >> PASS EXITS: Current PHI node has no constant incoming value! << <<\n";
          return false;
        }
      }
      else if( insCmpVar->getName().equals(phiCanonicalIV->getOperand(1)->getName()) ) {
        if( dyn_cast<ConstantInt>(phiCanonicalIV->getIncomingValue(0)) ) {
          cniPHIConst = dyn_cast<ConstantInt>(phiCanonicalIV->getIncomingValue(0));
          uintPHIConstId = 0;
        }
        else {
          errs() << "\n>> >> PASS EXITS: Current PHI node has no constant incoming value! << <<\n";
          return false;
        }
      }
      else {
        errs() << "\n>> >> PASS EXITS: Compare variable does not match any PHI node incoming value! << <<\n";
        return false;
      }
    }

    if(cniCmpConst && valCmpVar) { // Proceed ony if there is exactly one variabe and one constant in the compare instruction.
      APInt aitCmpConst = cniCmpConst->getValue();
      unsigned int uintMinCmpConstBits = aitCmpConst.getMinSignedBits() + 1; // Calc the minimum required bits for this comparison.
      //errs() << ">> >> >> valCmpVar " << valCmpVar->getName() <<"\n";
      //errs() << ">> >> >> cniCmpConst " << cniCmpConst->getValue() <<"\n";
      //errs() << ">> >> >> cniCmpConst->getBitWidth() = " << cniCmpConst->getBitWidth() <<"\n";
      //errs() << ">> >> >> uintMinCmpConstBits = " << uintMinCmpConstBits <<"\n";
      //errs() << ">> >> >> insCmpVar =  " << *insCmpVar <<"\n";
      //errs() << ">> >> >> insCmpVar->getOpcode() =  " << insCmpVar->getOpcode() <<"\n";
      //errs() << ">> >> >> Instruction::Trunc = " << Instruction::Trunc <<"\n";
      //errs() << ">> >> >> valCmpVar NumOperands = " << insCmpVar->getNumOperands() <<"\n";
      //errs() << ">> >> >> valCmpVar Bits = " << valCmpVarBits <<"\n";

      if(uintMinCmpConstBits < cniCmpConst->getBitWidth()) {
        if(insCmpVar->getOpcode() == Instruction::Add) { // Only handle Add operations now
          // Truncate add op0
          CastInst *insTrAddOp0 = new TruncInst(insCmpVar->getOperand(0), 
				        IntegerType::get(insCmpVar->getContext(), uintMinCmpConstBits), "CCIR_trunc_add_op0", insCmpVar);
          // Truncate add op1
          CastInst *insTrAddOp1 = new TruncInst(insCmpVar->getOperand(1), 
					IntegerType::get(insCmpVar->getContext(), uintMinCmpConstBits), "CCIR_trunc_add_op1", insCmpVar);
          // Create new truncated add instruction
          BinaryOperator *bopNewAdd = BinaryOperator::Create(Instruction::Add, 
					insTrAddOp0, insTrAddOp1, "CCIR_trunc_add", insCmpVar);
          // Sign extend new add result
          unsigned int uintOrigAddBits = insCmpVar->getOperand(0)->getType()->getPrimitiveSizeInBits();
          CastInst *insSExtCmpVar = new SExtInst(bopNewAdd, IntegerType::get(insCmpVar->getContext(),
					    uintOrigAddBits), "CCIR_sext_cmpvar", insCmpVar);
          // Make use of the sign extended result, which should not change the behavior of the original program
          insCmpVar->replaceAllUsesWith(insSExtCmpVar); //==>
          //insCmpVar->replaceAllUsesWith(bopNewAdd);

          // Truncate compare instruction constant
          ConstantInt *cniTrCmpConst = ConstantInt::get(IntegerType::get(valCmpConst->getContext(), uintMinCmpConstBits), cniCmpConst->getSExtValue());
          assert(cniTrCmpConst->getSExtValue() == cniCmpConst->getSExtValue() && "Truncated compare constant integer value does not equal to the original!");

          // Creat new compare instruction with reduced bits
          CmpInst *insNewCmp = CmpInst::Create(insCmp->getOpcode(), insCmp->getPredicate(), bopNewAdd, cniTrCmpConst, "CCIR_trunc_cond", insCmp);

          // Make use of the new compare instruction, which should not change the behavior of the original program
          insCmp->replaceAllUsesWith(insNewCmp);

          // Drop/Erase dead instructions
          insCmp->dropAllReferences();
          insCmp->eraseFromParent();
          for(std::vector<Instruction*>::iterator itr = lstTruncInsts->begin(); itr != lstTruncInsts->end(); ++itr) {
            Instruction *insTrunc = dyn_cast<Instruction>(*itr);
            insTrunc->dropAllReferences();
            insTrunc->eraseFromParent();
          }
          insCmpVar->dropAllReferences();
	  // dropAll !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          insCmpVar->eraseFromParent();

          ConstantInt *cniTrPHIConst= ConstantInt::get(IntegerType::get(cniPHIConst->getContext(), uintMinCmpConstBits), cniPHIConst->getSExtValue());
          assert(cniTrCmpConst->getSExtValue() == cniCmpConst->getSExtValue() && "Truncated PHI constant integer value does not equal to the original!");
          //phiCanonicalIV->setIncomingValue(uintPHIConstId, cniTrPHIConst);

          boolChanged = true;
        } // if(insCmpVar->getOpcode() == Instruction::Add)
      } // if(uintMinCmpConstBits < cniCmpConst->getBitWidth())
    } // if(cniCmpConst && valCmpVar)
  } // if(insCmp)

  if(boolChanged) errs() << "\n>> >> The bit width of canonical induction variable is reduced! << <<\n";

  return boolChanged;
}

char IndVarBitsReduction::ID = 0;
RegisterPass<IndVarBitsReduction> PX("indvarbits-reduce", "Reduce canonical loop induction variable bit width", false, false);

