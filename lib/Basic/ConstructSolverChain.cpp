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

namespace {

using namespace klee;

std::string getSolverPath(Solver *solver, std::string& base,
                          const ConstraintLogConfig *clc,
                          const char *extensionPrefix = NULL) {
  // FIXME: Kind of gross. Should probably have method on solver
  // that just gives us the file extension.
  // Create dummy query to determine file extension
  const char* fileExtension = NULL;
  ConstraintManager cm;
  Query q(cm, ConstantExpr::alloc(0, Expr::Bool));
  char *dummy = solver->getConstraintLog(q, &fileExtension, /*clc=*/clc);
  free(dummy);
  std::string filePath = base;
  if (extensionPrefix) {
    filePath += extensionPrefix;
  }
  filePath += fileExtension;
  return filePath;
}
}

namespace klee {

Solver *constructSolverChain(Solver *coreSolver, std::string querySMT2LogPath,
                             std::string baseSolverQuerySMT2LogPath,
                             std::string queryPCLogPath,
                             std::string baseSolverQueryPCLogPath,
                             std::string queryCoreSolverLangLogPath,
                             std::string baseCoreSolverLangLogPath) {
  Solver *solver = coreSolver;

  if (optionIsSet(queryLoggingOptions, SOLVER_PC)) {
    solver = createPCLoggingSolver(solver, baseSolverQueryPCLogPath,
                                   MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .pc format to %s\n",
                 baseSolverQueryPCLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_SMTLIB)) {
    solver = createSMTLIBLoggingSolver(solver, baseSolverQuerySMT2LogPath,
                                       MinQueryTimeToLog);
    klee_message("Logging queries that reach solver in .smt2 format to %s\n",
                 baseSolverQuerySMT2LogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_CORE_SOLVER_LANG)) {
    ConstraintLogConfig *clc = NULL;
    Z3ConstraintLogConfig z3clc;
    z3clc.ackermannizeArrays = false;
    z3clc.useToIEEEBVFunction = Z3GetConstraintLogUseToIEEEBV;
    if (CoreSolverToUse == Z3_SOLVER) {
      clc = &z3clc;
    }
    std::string filePath = getSolverPath(solver, baseCoreSolverLangLogPath, clc, NULL);
    solver = createCoreSolverLangLoggingSolver(solver, filePath,
                                               MinQueryTimeToLog, /*clc=*/clc);
    klee_message(
        "Logging queries that reach solver in core solver's language to %s\n",
        filePath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, SOLVER_CORE_SOLVER_LANG_AA)) {
    ConstraintLogConfig *clc = NULL;
    Z3ConstraintLogConfig z3clc;
    z3clc.ackermannizeArrays = true;
    z3clc.useToIEEEBVFunction = Z3GetConstraintLogUseToIEEEBV;
    if (CoreSolverToUse == Z3_SOLVER) {
      clc = &z3clc;
    } else {
      llvm::errs() << "Core solver is not Z3, cannot ackermannize arrays\n";
    }

    std::string filePath = getSolverPath(solver, baseCoreSolverLangLogPath, clc, "aa.");
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

  if (optionIsSet(queryLoggingOptions, ALL_PC)) {
    solver = createPCLoggingSolver(solver, queryPCLogPath, MinQueryTimeToLog);
    klee_message("Logging all queries in .pc format to %s\n",
                 queryPCLogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, ALL_SMTLIB)) {
    solver =
        createSMTLIBLoggingSolver(solver, querySMT2LogPath, MinQueryTimeToLog);
    klee_message("Logging all queries in .smt2 format to %s\n",
                 querySMT2LogPath.c_str());
  }

  if (optionIsSet(queryLoggingOptions, ALL_CORE_SOLVER_LANG)) {
    ConstraintLogConfig *clc = NULL;
    Z3ConstraintLogConfig z3clc;
    z3clc.useToIEEEBVFunction = Z3GetConstraintLogUseToIEEEBV;
    if (CoreSolverToUse == Z3_SOLVER) {
      clc = &z3clc;
    }
    std::string filePath = getSolverPath(solver, queryCoreSolverLangLogPath, clc, NULL);
    solver = createCoreSolverLangLoggingSolver(solver, filePath,
                                               MinQueryTimeToLog, /*clc=*/clc);
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
