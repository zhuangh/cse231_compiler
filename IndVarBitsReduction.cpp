#define DEBUG_TYPE "indvarbits-reduce"
#include "llvm/Transforms/Utils/Local.h"
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

  PHINode *phiCanonIndVar = L->getCanonicalInductionVariable();
  unsigned int uintPhiNumInVals;

  if(!phiCanonIndVar) {
    errs() << "  >> >> PASS EXITS: Current loop has no canonical induction variable << <<\n";
    return false;
  }
  else {
    uintPhiNumInVals = phiCanonIndVar->getNumIncomingValues();
    if(uintPhiNumInVals != 2) {
      errs() << "  >> >> PASS EXITS: Current loop PHI node has more than 2 incoming edges << <<\n";
      return false;
    } 
  }

  //errs() << ">> >> phiCanonIndVar->getName(): " << phiCanonIndVar->getName().str() << "\n";
  //errs() << ">> >> phiCanonIndVar->getOperand(0): " << *(phiCanonIndVar->getOperand(0)) << "\n";
  //errs() << ">> >> phiCanonIndVar->getOperand(1): " << *(phiCanonIndVar->getOperand(1)) << "\n";
  //errs() << ">> >> phiCanonIndVar->getIncomingValue(0): " << *(phiCanonIndVar->getIncomingValue(0)) << "\n";
  //errs() << ">> >> phiCanonIndVar->getIncomingValue(1): " << *(phiCanonIndVar->getIncomingValue(1)) << "\n";
  //errs() << ">> >> phiCanonIndVar->getIncomingBlock(0): " << *(phiCanonIndVar->getIncomingBlock(0)) << "\n";
  //errs() << ">> >> phiCanonIndVar->getIncomingBlock(1): " << *(phiCanonIndVar->getIncomingBlock(1)) << "\n";

  BranchInst *insBr = cast<BranchInst>(L->getExitingBlock()->getTerminator()); // Get the br instruction
  Value *valBrCond = insBr->getCondition(); // Get the br condition
  //errs() << ">> >> insBr: " << *insBr <<"\n";
  //errs() << ">> >> valBrCond: " << *valBrCond <<"\n";

  std::vector<Instruction*> *lstTruncInsts = new std::vector<Instruction*>;
  lstTruncInsts->clear();

  ICmpInst *insCmp = dyn_cast<ICmpInst>(valBrCond);
  if(insCmp) { //Proceed if not NULL
    Value *valCmpLHS = insCmp->getOperand(0);
    Value *valCmpRHS = insCmp->getOperand(1);
    Value *valCmpVar = NULL;
    Value *valCmpConst = NULL;
    Instruction *insCmpVar = NULL;
    ConstantInt *cniCmpConst = NULL; // Compare instruction constant int
    ConstantInt *cniPHIConst = NULL; // PHINode constant int
    ConstantInt *cniIncConst = NULL; // Indunction variable increment constant int

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
    
    while(insCmpVar->getOpcode() == Instruction::Trunc) { // Find relevant trunc OPs, which will be removed later.
      lstTruncInsts->push_back(insCmpVar);
      insCmpVar = dyn_cast<Instruction>(insCmpVar->getOperand(0));
    }

    // insCmpVar is supposed to be a binary operation now
    if(!insCmpVar) {
      errs() << "  >> >> PASS EXITS: Branching conditional variable is produced in an unknown way! << <<\n";
      return false;
    }
    else {
      if( insCmpVar->getName().equals(phiCanonIndVar->getOperand(0)->getName()) ) {
        if( dyn_cast<ConstantInt>(phiCanonIndVar->getIncomingValue(1)) ) {
          cniPHIConst = dyn_cast<ConstantInt>(phiCanonIndVar->getIncomingValue(1));
        }
        else {
          errs() << "  >> >> PASS EXITS: Current PHI node has no constant incoming value! << <<\n";
          return false;
        }
      }
      else if( insCmpVar->getName().equals(phiCanonIndVar->getOperand(1)->getName()) ) {
        if( dyn_cast<ConstantInt>(phiCanonIndVar->getIncomingValue(0)) ) {
          cniPHIConst = dyn_cast<ConstantInt>(phiCanonIndVar->getIncomingValue(0));
        }
        else {
          errs() << "  >> >> PASS EXITS: Current PHI node has no constant incoming value! << <<\n";
          return false;
        }
      }
      else {
        errs() << "  >> >> PASS EXITS: Compare variable does not match any PHI node incoming value! << <<\n";
        return false;
      }
    }

    if(cniCmpConst && valCmpVar) { // Proceed ony if there is exactly one variabe and one constant in the compare instruction.
      //errs() << ">> >> >> valCmpVar " << valCmpVar->getName() <<"\n";
      //errs() << ">> >> >> cniCmpConst " << cniCmpConst->getValue() <<"\n";
      //errs() << ">> >> >> cniCmpConst->getBitWidth() = " << cniCmpConst->getBitWidth() <<"\n";
      //errs() << ">> >> >> uintMinIndVarBits = " << uintMinIndVarBits <<"\n";
      //errs() << ">> >> >> insCmpVar =  " << *insCmpVar <<"\n";
      //errs() << ">> >> >> insCmpVar->getOpcode() =  " << insCmpVar->getOpcode() <<"\n";
      //errs() << ">> >> >> Instruction::Trunc = " << Instruction::Trunc <<"\n";
      //errs() << ">> >> >> valCmpVar NumOperands = " << insCmpVar->getNumOperands() <<"\n";
      //errs() << ">> >> >> valCmpVar Bits = " << valCmpVarBits <<"\n";

      if(insCmpVar->getOpcode() == Instruction::Add) { // Only handle Add operations now
        if( phiCanonIndVar->getName().equals(insCmpVar->getOperand(0)->getName()) ) {
          if( dyn_cast<ConstantInt>(insCmpVar->getOperand(1)) ) {
            cniIncConst = dyn_cast<ConstantInt>(insCmpVar->getOperand(1));
          }
          else {
            errs() << "  >> >> PASS EXITS: Induction variable is not increased by constant integer value! << <<\n";
            return false;
          }
        }
        else if( phiCanonIndVar->getName().equals(insCmpVar->getOperand(1)->getName()) ) {
          if( dyn_cast<ConstantInt>(insCmpVar->getOperand(0)) ) {
            cniIncConst = dyn_cast<ConstantInt>(insCmpVar->getOperand(0));
          }
          else {
            errs() << "  >> >> PASS EXITS: Induction variable is not increased by constant integer value! << <<\n";
            return false;
          }
        }
        else {
          errs() << "  >> >> PASS EXITS: Induction variable is not used as loop terminator! << <<\n";
          return false;
        }
      } // if(insCmpVar->getOpcode() == Instruction::Add)
      unsigned int uintMinIndVarBits = std::max(cniCmpConst->getValue().getMinSignedBits(), cniIncConst->getValue().getMinSignedBits()) + 1;

      if( (uintMinIndVarBits < cniCmpConst->getBitWidth()) && (uintMinIndVarBits < cniIncConst->getBitWidth()) ) {
        ConstantInt *cniTrPHIConst= ConstantInt::get(IntegerType::get(cniPHIConst->getContext(), uintMinIndVarBits), cniPHIConst->getSExtValue());
        assert(cniTrPHIConst->getSExtValue() == cniPHIConst->getSExtValue() && "Truncated PHI constant integer value does not equal to the original!");

        IntegerType *IntMinBitsTy = Type::getIntNTy(phiCanonIndVar->getContext(), uintMinIndVarBits);
        PHINode *phiNewNode = PHINode::Create(IntMinBitsTy, 2, "CCIR_trunc_indvar", phiCanonIndVar);

        // Truncate increment instruction constant
        ConstantInt *cniTrIncConst = ConstantInt::get(IntegerType::get(insCmpVar->getContext(), uintMinIndVarBits), cniIncConst->getSExtValue());
        assert(cniTrIncConst->getSExtValue() == cniIncConst->getSExtValue() && "Truncated increment constant integer value does not equal to the original!");
        // Creat new increment instruction with reduced bits
        BinaryOperator *bopIndVarNext = BinaryOperator::Create(Instruction::Add, phiNewNode, cniTrIncConst, "CCIR_trunc_indvar_next", insCmpVar);
        // Relate the new induction variable with the new PHI node, which should not change the behavior of the original program
        phiNewNode->addIncoming(cniTrPHIConst, phiCanonIndVar->getIncomingBlock(0));
        phiNewNode->addIncoming(bopIndVarNext, phiCanonIndVar->getIncomingBlock(1));

        // Truncate compare instruction constant
        ConstantInt *cniTrCmpConst = ConstantInt::get(IntegerType::get(valCmpConst->getContext(), uintMinIndVarBits), cniCmpConst->getSExtValue());
        assert(cniTrCmpConst->getSExtValue() == cniCmpConst->getSExtValue() && "Truncated compare constant integer value does not equal to the original!");
        // Creat new compare instruction with reduced bits
        CmpInst *insNewCmp = CmpInst::Create(insCmp->getOpcode(), insCmp->getPredicate(), bopIndVarNext, cniTrCmpConst, "CCIR_trunc_exitcond", insCmp);

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

        // Preserve other references to phiCanonIndVar
        unsigned int uintCanonIndVarBits = phiCanonIndVar->getType()->getPrimitiveSizeInBits();
        for (Instruction::use_iterator use_itr = phiCanonIndVar->use_begin(); use_itr!= phiCanonIndVar->use_end(); ++use_itr) {
          Instruction *insUseCanonIndVar = dyn_cast<Instruction>(*use_itr);
          //errs() << "insUseCanonIndVar: " << *insUseCanonIndVar << "\n";
          if(insUseCanonIndVar != insCmpVar) {
            CastInst *insSExtIndVar = new SExtInst(phiNewNode, IntegerType::get(insUseCanonIndVar->getContext(), uintCanonIndVarBits), "CCIR_sext_indvar", insUseCanonIndVar);
            insUseCanonIndVar->replaceUsesOfWith(phiCanonIndVar, insSExtIndVar);
          }
        }

        // Drop/Erase dead instructions
        insCmpVar->replaceAllUsesWith(phiCanonIndVar); // A trick to break the mutual reference between insCmpVar and phiCanonIndVar
        insCmpVar->dropAllReferences();
        insCmpVar->eraseFromParent();
        phiCanonIndVar->dropAllReferences();
        phiCanonIndVar->eraseFromParent();

        errs() << "  >> >> PASS APPLIED: The bit width of old induction variable is reduced from "  << uintCanonIndVarBits << " to " << uintMinIndVarBits << "! << <<\n";
        return true;
      } // if( (uintMinIndVarBits < cniCmpConst->getBitWidth()) && (uintMinIndVarBits < cniIncConst->getBitWidth()) )
      else {
        errs() << "  >> >> PASS EXITS: Induction variable width should not be reduced! << <<\n";
        return false;
      }
    } // if(cniCmpConst && valCmpVar)
    else {
      errs() << "  >> >> PASS EXITS: Induction variable width should not be reduced! << <<\n";
      return false;
    }
  } // if(insCmp)
  else {
    errs() << "  >> >> PASS EXITS: Loop does not terminate on conditional compare! << <<\n";
    return false;
  }
}

char IndVarBitsReduction::ID = 0;
RegisterPass<IndVarBitsReduction> PX("indvarbits-reduce", "Reduce canonical loop induction variable bit width", false, false);

