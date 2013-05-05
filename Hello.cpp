//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//
//
// The "Hello" pass is designed to simply print out the name of non-external
// functions that exist in the program being compiled. It does not modify the
// program at all, it just inspects it. 
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"

// We are writing a Pass, we are operating on Function's, and we will be doing
// some printing.
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
using namespace llvm;

STATISTIC(HelloCounter, "Counts number of functions greeted");

// Anonymous namespaces are to C++ what the "static" keyword is to C (at global
// scope). It makes the things declared inside of the anonymous namespace
// visible only to the current file.
namespace {
  // Hello - The first implementation, without getAnalysisUsage.

  // This declares a "Hello" class that is a subclass of FunctionPass.
  // FunctionPass's operate on a function at a time.
  struct Hello : public FunctionPass {
    // This declares pass identifier used by LLVM to identify pass. This allows
    // LLVM to avoid using expensive C++ runtime information.
    static char ID; // Pass identification, replacement for typeid
    Hello() : FunctionPass(ID) {}

    // We declare a "runOnFunction" method, which overloads an abstract virtual
    // method inherited from FunctionPass. This is where we are supposed to do
    // our thing, so we just print out our message with the name of each
    // function.
    virtual bool runOnFunction(Function &F) {
      ++HelloCounter;
      errs() << "Hello: ";
      errs().write_escaped(F.getName()) << '\n';
      return false;
    }
  };
}

// We initialize pass ID here. LLVM uses ID's address to identify a pass, so
// initialization value is not important.
char Hello::ID = 0;

// Lastly, we register our class Hello, giving it a command line argument
// "hello", and a name "Hello World Pass". The last two arguments describe its
// behavior: if a pass walks CFG without modifying it then the third argument
// is set to true; if a pass is an analysis pass, for example dominator tree
// pass, then true is supplied as the fourth argument.
static RegisterPass<Hello> X("air_opt_hello", "@AIR Hello World Pass", false, false);

// Note that everything in this file is contained in an anonymous namespace —
// this reflects the fact that passes are self contained units that do not need
// external interfaces (although they can have them) to be useful.

//===----------------------------------------------------------------------===//

// Yet another example
namespace {
  // Hello2 - The second implementation with getAnalysisUsage implemented.
  struct Hello2 : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Hello2() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      ++HelloCounter;
      errs() << "Hello: ";
      errs().write_escaped(F.getName()) << '\n';
      return false;
    }

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }
  };
}

char Hello2::ID = 0;
static RegisterPass<Hello2> Y("air_opt_hello2", "@AIR Hello World Pass (with getAnalysisUsage implemented)");

