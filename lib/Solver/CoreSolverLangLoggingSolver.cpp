//===-- CoreSolverLangLoggingSolver.cpp -----------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "QueryLoggingSolver.h"
#include "klee/ConstraintLogConfig.h"
#include "klee/Solver.h"

namespace klee {

class CoreSolverLangLoggingSolver : public QueryLoggingSolver {
private:
  ConstraintLogConfig *clc;
  virtual void printQuery(const Query &query, const Query *falseQuery = 0,
                          const std::vector<const Array *> *objects = 0) {
    char *queryText = NULL;
    if (falseQuery) {
      queryText = solver->getConstraintLog(*falseQuery, /*fileExtension=*/NULL,
                                           /*clc=*/clc);
    } else {
      queryText =
          solver->getConstraintLog(query, /*fileExtension=*/NULL, /*clc=*/clc);
    }
    logBuffer << queryText;
    free(queryText);
    // FIXME: Need to handle requesting object values.
  }

public:
  CoreSolverLangLoggingSolver(Solver *_solver, std::string path,
                              int queryTimeToLog,
                              const ConstraintLogConfig *_clc)
      // FIXME: commentSign is wrong. It depends on core solver language
      : QueryLoggingSolver(_solver, path, /*commentSign=*/";", queryTimeToLog),
        clc(NULL) {
    // We need to maintain our own copy of the config.
    if (_clc) {
      clc = _clc->alloc();
    }
  }
  ~CoreSolverLangLoggingSolver() {
    if (clc) {
      delete clc;
    }
  }
};

Solver *createCoreSolverLangLoggingSolver(Solver *_solver, std::string path,
                                          int minQueryTimeToLog,
                                          const ConstraintLogConfig *clc) {
  return new Solver(
      new CoreSolverLangLoggingSolver(_solver, path, minQueryTimeToLog, clc));
}
}
