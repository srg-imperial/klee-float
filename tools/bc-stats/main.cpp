//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

using namespace llvm;

namespace {
cl::opt<std::string> InputFile(cl::desc("<input bytecode>"), cl::Positional,
                               cl::init("-"));
}

enum ExitCodes { SUCCESS, FAIL_TO_OPEN = 1, FAIL_TO_PARSE = 2 };

struct BCSTats {
  uint64_t num_function_defns; // Function definitions
  uint64_t num_function_decls; // Function declarations
  uint64_t num_branches;        // Number of branches in module

  BCSTats() : num_function_defns(0), num_function_decls(0), num_branches(0){};

  void dump(llvm::raw_ostream &os) {
    os << "num_branches: " << num_branches
       << "\nnum_function_defns: " << num_function_defns
       << "\nnum_function_decls: " << num_function_decls << "\n";
  }

  void dump() { dump(errs()); }
};

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, " bc-stats");

  LLVMContext ctx;
  Module *m = NULL;
  OwningPtr<MemoryBuffer> BufferPtr;
  std::string errorMsg = "";
  error_code ec = MemoryBuffer::getFileOrSTDIN(InputFile.c_str(), BufferPtr);
  if (ec) {
    errs() << "Failed to open \"" << InputFile << "\"\n";
    return FAIL_TO_OPEN;
  }

  m = ParseBitcodeFile(BufferPtr.get(), ctx, &errorMsg);
  if (!m) {
    errs() << "Failed to parse bitcode file: " << errorMsg << "\n";
    return FAIL_TO_PARSE;
  }

  BCSTats moduleStats;

  // Walk through functions
  for (Module::iterator func = m->begin(), fie = m->end(); func != fie;
       ++func) {
    if (func->isDeclaration()) {
      ++moduleStats.num_function_decls;
      continue;
    }
    ++moduleStats.num_function_defns;
    errs() << "Processing: " << func->getName() << "\n";

    // TODO: Implement algorithm to count number of symbolic bytes.
    uint64_t num_branches_in_function = 0;
    for (Function::iterator BB = func->begin(), BE = func->end(); BB != BE;
         ++BB) {
      for (BasicBlock::iterator ii = BB->begin(), ie = BB->end(); ii != ie;
           ++ii) {
        // This covers BranchInst, SwitchInst, IndirectBrInst, ReturnInst, etc..
        if (TerminatorInst *ti = dyn_cast<TerminatorInst>(ii)) {
          errs() << "Found terminator instruction\n";
          ti->dump();
          uint64_t successors = ti->getNumSuccessors();
          if (successors) {
            num_branches_in_function += (successors - 1);
            errs() << "successors: " << successors << "\n";
            if (successors > 1)
              errs() << "Adding " << (successors - 1) << " branches\n";
          }
        }
      }
    }
    errs() << "Found " << num_branches_in_function << " branches in "
           << func->getName() << "\n";
    moduleStats.num_branches += num_branches_in_function;
  }

  // Dump stats
  moduleStats.dump(outs());
  return SUCCESS;
}
