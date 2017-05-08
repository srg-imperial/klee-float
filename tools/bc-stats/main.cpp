//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Internal/Support/Debug.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO.h"
#include <llvm/IR/Constants.h>
#include <string>
#include <vector>

using namespace llvm;

namespace {
cl::opt<std::string> InputFile(cl::desc("<input bytecode>"), cl::Positional,
                               cl::init("-"));

cl::opt<std::string>
    EntryPoint("entry-point",
               cl::desc("Entry Point to use for DCE. By default don't do DCE"),
               cl::init(""));

cl::opt<bool> OnlyCountSymbolicBytesInEntryPoint(
    "only-symbolic-bytes-in-entry",
    cl::desc("Only count symbolic bytes in the entry point functions (default: "
             "false)"),
    cl::init(false));
}

enum ExitCodes {
  SUCCESS,
  FAIL_TO_OPEN = 1,
  FAIL_TO_PARSE = 2,
  FAIL_TO_FIND_ENTRY_POINT = 3,
  INVALID_CMD_LINE_OPTIONS = 4
};

struct BCSTats {
  uint64_t num_function_defns;           // Function definitions
  uint64_t num_function_decls;           // Function declarations
  uint64_t num_branches;                 // Number of branches in module
  uint64_t estimated_num_symbolic_bytes; // Estimated number of symbolic bytes

  BCSTats()
      : num_function_defns(0), num_function_decls(0), num_branches(0),
        estimated_num_symbolic_bytes(0){};

  void dump(llvm::raw_ostream &os) {
    // Output as YAML
    os << "num_branches: " << num_branches
       << "\nnum_function_defns: " << num_function_defns
       << "\nnum_function_decls: " << num_function_decls
       << "\nestimated_num_symbolic_bytes: " << estimated_num_symbolic_bytes
       << "\n";
  }

  void dump() { dump(errs()); }
};

// FIXME: This is very naive. In the presence of any branching control flow
// this estimate is wrong.
// FIXME: Only count `klee_make_symbolic` calls in BBs that dominate every exit
// BB. That way we only count klee_make_symbolic calls that have to be exited
// in the function.
void countSymbolicBytes(const Function *func, BCSTats &moduleStats) {
  uint64_t symbolic_bytes_requested_in_function = 0;
  if (OnlyCountSymbolicBytesInEntryPoint) {
    if (func->getName() != EntryPoint) {
      KLEE_DEBUG_WITH_TYPE("symbolic_count",
                           errs() << "Skipping non-entry point function \""
                                  << func->getName() << "\"\n");
    }
  }
  for (Function::const_iterator BB = func->begin(), BE = func->end(); BB != BE;
       ++BB) {
    for (BasicBlock::const_iterator ii = BB->begin(), ie = BB->end(); ii != ie;
         ++ii) {
      // FIXME: Should use `ImmutableCallSite`. Can't use it like `CallSite`
      // in LLVM 3.4
      CallSite cs;
      if (const CallInst *ci = dyn_cast<CallInst>(ii)) {
        cs = CallSite(const_cast<CallInst *>(ci));
      } else if (const InvokeInst *invInst = dyn_cast<InvokeInst>(ii)) {
        cs = CallSite(const_cast<InvokeInst *>(invInst));
      }
      if (!cs)
        continue;
      Function *callee = cs.getCalledFunction();
      if (!callee || callee->getName() != "klee_make_symbolic")
        continue;

      assert((cs.arg_size() == 2 || cs.arg_size() == 3) &&
             "wrong number of args");
      Value *bytes = cs.getArgument(1);
      if (const ConstantInt *CE = dyn_cast<ConstantInt>(bytes)) {
        assert(CE->getValue().getBitWidth() <= 64 && "Too wide");
        symbolic_bytes_requested_in_function +=
            CE->getValue().getLimitedValue();
      } else {
        errs() << "WARNING: Could not handle argument to klee_make_symbolic: "
               << *bytes << "\n";
      }
    }
  }
  KLEE_DEBUG_WITH_TYPE(
      "symbolic_count",
      errs() << "Estimated that \"" << func->getName() << "\" requested "
             << symbolic_bytes_requested_in_function << " bytes\n");
  moduleStats.estimated_num_symbolic_bytes +=
      symbolic_bytes_requested_in_function;
}

void countBranches(const Function *func, BCSTats &moduleStats) {
  uint64_t num_branches_in_function = 0;
  for (Function::const_iterator BB = func->begin(), BE = func->end(); BB != BE;
       ++BB) {
    for (BasicBlock::const_iterator ii = BB->begin(), ie = BB->end(); ii != ie;
         ++ii) {
      // This covers BranchInst, SwitchInst, IndirectBrInst, ReturnInst, etc..
      if (const TerminatorInst *ti = dyn_cast<TerminatorInst>(ii)) {
        KLEE_DEBUG_WITH_TYPE("branching",
                             errs() << "Found terminator instruction\n");
        KLEE_DEBUG_WITH_TYPE("branching", ti->dump());
        uint64_t successors = ti->getNumSuccessors();
        if (successors) {
          num_branches_in_function += (successors - 1);
          KLEE_DEBUG_WITH_TYPE("branching",
                               errs() << "successors: " << successors << "\n");
          if (successors > 1)
            KLEE_DEBUG_WITH_TYPE("branching",
                                 errs() << "Adding " << (successors - 1)
                                        << " branches\n");
        }
      }
    }
  }
  KLEE_DEBUG_WITH_TYPE("branching",
                       errs() << "Found " << num_branches_in_function
                              << " branches in " << func->getName() << "\n");
  moduleStats.num_branches += num_branches_in_function;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, " bc-stats");

  if (OnlyCountSymbolicBytesInEntryPoint && EntryPoint.size() == 0) {
    errs() << "Entry point must be specified when using "
              "-only-symbolic-bytes-in-entry\n";
    return INVALID_CMD_LINE_OPTIONS;
  }

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

  PassManager PM;
  std::vector<const char *> exportFuncs;
  if (EntryPoint.size() > 0) {
    if (m->getFunction(EntryPoint) == NULL) {
      errs() << "Cannot find entry point function \"" << EntryPoint << "\"\n";
      return FAIL_TO_FIND_ENTRY_POINT;
    }
    exportFuncs.push_back(EntryPoint.c_str());
    // This will remove dead globals
    PM.add(createInternalizePass(exportFuncs));
    PM.add(createGlobalDCEPass());
  }
  PM.run(*m);

  BCSTats moduleStats;

  // Walk through functions
  for (Module::const_iterator func = m->begin(), fie = m->end(); func != fie;
       ++func) {
    if (func->isDeclaration()) {
      ++moduleStats.num_function_decls;
      continue;
    }
    ++moduleStats.num_function_defns;
    // errs() << "Processing: " << func->getName() << "\n";
    countBranches(func, moduleStats);
    countSymbolicBytes(func, moduleStats);
  }

  // Dump stats
  moduleStats.dump(outs());
  return SUCCESS;
}
