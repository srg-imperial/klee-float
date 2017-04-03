//===-- ConstructSolverChain.cpp ------------------------------------++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*
 * This file groups declarations that are common to both KLEE and Kleaver.
 */
#include "klee/CommandLine.h"
#include "klee/Common.h"
#include "klee/ConstraintLogConfig.h"
#include "klee/Constraints.h"
#include "klee/Internal/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {
Solver *constructSolverChain(Solver *coreSolver,
                             std::string querySMT2LogPath,
                             std::string baseSolverQuerySMT2LogPath,
                             std::string queryKQueryLogPath,
                             std::string baseSolverQueryKQueryLogPath,
                             std::string queryCoreSolverLangLogPath,
                             std::string baseCoreSolverLangLogPath) {
  Solver *solver = coreSolver;

  if (optionIsSet(queryLoggingOptions, SOLVER_KQUERY)) {
    solver = createKQueryLoggingSolver(solver, baseSolverQueryKQueryLogPath,
                                   MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .kquery format to %s\n",
                 baseSolverQueryKQueryLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_SMTLIB)) {
    solver = createSMTLIBLoggingSolver(solver, baseSolverQuerySMT2LogPath,
                                       MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .smt2 format to %s\n",
                 baseSolverQuerySMT2LogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_CORE_SOLVER_LANG)) {
    const char* fileExtension = NULL;

    // FIXME: Kind of gross. Should probably have method on solver
    // that just gives us the file extension.
    // Create dummy query to determine file extension
    ConstraintManager cm;
    Query q(cm, ConstantExpr::alloc(0, Expr::Bool));
    char *dummy = solver->getConstraintLog(q, &fileExtension, /*clc=*/NULL);
    free(dummy);
    std::string filePath = baseCoreSolverLangLogPath;
    filePath += fileExtension;

    solver = createCoreSolverLangLoggingSolver(solver, filePath,
                                               MinQueryTimeToLog, /*clc=*/NULL);
    klee_message(
        "Logging queries that reach solver in core solver's language to %s\n",
        filePath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_CORE_SOLVER_LANG_AA)) {
    const char *fileExtension = NULL;

    // FIXME: Kind of gross. Should probably have method on solver
    // that just gives us the file extension.
    // Create dummy query to determine file extension
    ConstraintManager cm;
    Query q(cm, ConstantExpr::alloc(0, Expr::Bool));
    char *dummy = solver->getConstraintLog(q, &fileExtension, /*clc=*/NULL);
    free(dummy);
    std::string filePath = baseCoreSolverLangLogPath;
    filePath += "aa."; // give different name
    filePath += fileExtension;

    ConstraintLogConfig *clc = NULL;
    Z3ConstraintLogConfig z3clc;
    z3clc.ackermannizeArrays = true;
    if (CoreSolverToUse == Z3_SOLVER) {
      clc = &z3clc;
    } else {
      llvm::errs() << "Core solver is not Z3, cannot ackermannize arrays\n";
    }

    solver = createCoreSolverLangLoggingSolver(solver, filePath,
                                               MinQueryTimeToLog, /*clc=*/clc);
    klee_message("Logging all (might be ackermannized) queries in core "
                 "solver's language to %s\n",
                 filePath.c_str());
  }

  if (UseAssignmentValidatingSolver)
    solver = createAssignmentValidatingSolver(solver);

  if (UseFastCexSolver)
    solver = createFastCexSolver(solver);

  if (UseCexCache)
    solver = createCexCachingSolver(solver);

  if (UseCache)
    solver = createCachingSolver(solver);

  if (UseIndependentSolver)
    solver = createIndependentSolver(solver);

  if (DebugValidateSolver)
    solver = createValidatingSolver(solver, coreSolver);

  if (optionIsSet(queryLoggingOptions, ALL_KQUERY)) {
    solver = createKQueryLoggingSolver(solver, queryKQueryLogPath,
                                       MinQueryTimeToLog);
    klee_message("Logging all queries in .kquery format to %s\n",
                 queryKQueryLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, ALL_SMTLIB)) {
    solver =
        createSMTLIBLoggingSolver(solver, querySMT2LogPath, MinQueryTimeToLog);
    klee_message("Logging all queries in .smt2 format to %s\n",
                 querySMT2LogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, ALL_CORE_SOLVER_LANG)) {
    const char* fileExtension = NULL;

    // FIXME: Kind of gross. Should probably have method on solver
    // that just gives us the file extension.
    // Create dummy query to determine file extension
    ConstraintManager cm;
    Query q(cm, ConstantExpr::alloc(0, Expr::Bool));
    char *dummy = solver->getConstraintLog(q, &fileExtension, /*clc=*/NULL);
    free(dummy);
    std::string filePath = queryCoreSolverLangLogPath;
    filePath += fileExtension;

    solver = createCoreSolverLangLoggingSolver(solver, filePath,
                                               MinQueryTimeToLog, /*clc=*/NULL);
    klee_message(
        "Logging all queries in core solver's language to %s\n",
        filePath.c_str());
  }

  if (DebugCrossCheckCoreSolverWith != NO_SOLVER) {
    Solver *oracleSolver = createCoreSolver(DebugCrossCheckCoreSolverWith);
    solver = createValidatingSolver(/*s=*/solver, /*oracle=*/oracleSolver);
  }

  return solver;
}
}
