//===-- MergeHandler.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/MergeHandler.h"

#include "CoreStats.h"
#include "Executor.h"
#include "klee/ExecutionState.h"

namespace klee {
llvm::cl::opt<bool>
    UseMerge("use-merge",
        llvm::cl::init(false),
        llvm::cl::desc("Enable support for klee_open_merge() and klee_close_merge() (experimental)"));

llvm::cl::opt<bool>
    DebugLogMerge("debug-log-merge",
        llvm::cl::init(false),
        llvm::cl::desc("Enhanced verbosity for region based merge operations"));

void MergeHandler::addClosedState(ExecutionState *es,
                                         llvm::Instruction *mp) {
  std::map<llvm::Instruction *, std::vector<ExecutionState *> >::iterator closePoint = reachedMergeClose.find(mp);

  // If no other state has yet encountered this klee_close_merge instruction,
  // add a new element to the map
  if (closePoint == reachedMergeClose.end()) {
    reachedMergeClose[mp].push_back(es);
    executor->pauseState(*es);
  } else {
    // Otherwise try to merge with any state in the map element for this
    // instruction
    std::vector<ExecutionState *> &cpv = closePoint->second;
    bool mergedSuccessful = false;

    for (std::vector<ExecutionState *>::iterator it = cpv.begin(),
           ie = cpv.end(); it != ie; ++it ) {
      if ((*it)->merge(*es)) {
        executor->terminateState(*es);
        mergedSuccessful = true;
        break;
      }
    }
    if (!mergedSuccessful) {
      cpv.push_back(es);
      executor->pauseState(*es);
    }
  }
}

void MergeHandler::releaseStates() {
  for (std::map<llvm::Instruction *, std::vector<ExecutionState *> >::iterator it = reachedMergeClose.begin(),
         ie = reachedMergeClose.end(); it != ie; ++it ) {
    for (std::vector<ExecutionState *>::iterator sit = it->second.begin(),
           sie = it->second.end(); sit != sie; ++sit ) {
      executor->continueState(*(*sit));
    }
  }
  reachedMergeClose.clear();
}

bool MergeHandler::hasMergedStates() {
  return (!reachedMergeClose.empty());
}

MergeHandler::MergeHandler(Executor *_executor)
    : executor(_executor), refCount(0) {
}

MergeHandler::~MergeHandler() {
  releaseStates();
}
}
