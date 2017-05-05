//===-- TimingSolver.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TimingSolver.h"

#include "klee/Config/Version.h"
#include "klee/ExecutionState.h"
#include "klee/Solver.h"
#include "klee/Statistics.h"
#include "klee/Internal/System/Time.h"
#include "klee/Internal/Support/Debug.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "CoreStats.h"
#include "Executor.h"
#include "ExecutorTimerInfo.h"

#include "llvm/Support/TimeValue.h"
#include "llvm/Support/CommandLine.h"

using namespace klee;
using namespace llvm;

namespace {
cl::opt<bool>
DynamicSolverTimeout("dynamic-solver-timeout",
        cl::desc("Use dynamic solver timeout"),
        cl::init(false));

cl::opt<double> DynamicSolverTimeoutTestCaseGenMaxTime(
    "dynamic-solver-timeout-max-test-gen-time",
    cl::desc("Maximum time (seconds) allowed to spend doing all test case "
             "generation"),
    cl::init(0.0));

cl::opt<double> DynamicSolverTimeoutMinQueryTimePerTestCase(
    "dynamic-solver-timeout-min-query-time-per-test-case",
    cl::desc("Minimum time (seconds) allowed for a query during test case "
             "generation"),
    cl::init(0.0f));

cl::opt<double> DynamicSolverTimeoutMinQueryTimeDuringPathExploration(
    "dynamic-solver-timeout-min-query-time-during-path-exploration",
    cl::desc(
        "Minimum time (seconds) allowed for a query during path exploration"),
    cl::init(0.0f));

// HACK: This really belongs in the Executor but the Executor doesn't seem
// completely consistent in the way in the way it sets the timeout so we hack
// setting it here instead.
// Returns true if the underlying solver should be executed.
bool setDynamicTimeout(TimingSolver *s) {
  if (!DynamicSolverTimeout)
    return true;

  bool shouldRunSolver = true;

  // HACK: Try to get the HaltTimer
  static const Executor::TimerInfo *ht = NULL;
  static double htNextFireTime = 0.0;
  if (!ht) {
    ht = s->executor->getHaltTimer();
    // HACK: Only grab the next fire time once
    // because when it fires KLEE will increment the
    // next fire time.
    htNextFireTime = ht->nextFireTime;
  }
  double timeoutToUse = 0.0;
  double executorCoreSolverTimeout = s->executor->getCoreSolverTimeout();
  if (!ht) {
    // No halt timer so just use the core solver timeout
    timeoutToUse = executorCoreSolverTimeout;
    KLEE_DEBUG_WITH_TYPE(
        "dynamic_solver_timeout",
        llvm::errs() << "No halt timer using Executor's core solver timeout\n");
  } else {
    double currentWallTime = util::getWallTime();
    double timeLeftUntilExecutorToHalt = htNextFireTime - currentWallTime;
    assert(isinf(timeLeftUntilExecutorToHalt) == 0);
    assert(!isnan(timeLeftUntilExecutorToHalt));
    if (timeLeftUntilExecutorToHalt < 0) {
      // Executor should have already halted so assume that we are doing test
      // case generation now.

      // Note we query the number of active states only once. By doing this the
      // queryTimePerState will be a fixed quantity during test case
      // generation. If we queried this every time then we would end up giving
      // more time to test cases we handle later (becuase the number of active
      // states will decrease as states are handled).
      static size_t numberOfStatesLeft = s->executor->getNumberOfActiveState();
      KLEE_DEBUG_WITH_TYPE("dynamic_solver_timeout",
                           llvm::errs() << "Looks like Executor already "
                                           "halted.\n");
      if (DynamicSolverTimeoutTestCaseGenMaxTime <= 0.0) {
        // Treat as no timeout
        timeoutToUse = 0.0;
        KLEE_DEBUG_WITH_TYPE(
            "dynamic_solver_timeout",
            llvm::errs() << "Using unlimited time for test case generation\n");
      } else if (numberOfStatesLeft == 0) {
        // FIXME: Ideally this should never happen
        klee_warning_once(
            0, "Doing test case generation with time limit but no states\n");
        // In this case just use regular core solver timeout
        timeoutToUse = executorCoreSolverTimeout;
        KLEE_DEBUG_WITH_TYPE("dynamic_solver_timeout",
                             llvm::errs() << "In test case generation but "
                                             "no states left. Using Executor's "
                                             "core solver timeout\n");
      } else {
        double queryTimePerState = DynamicSolverTimeoutTestCaseGenMaxTime /
                                   ((double)numberOfStatesLeft);
        // NOTE: `DynamicSolverTimeoutMinQueryTimePerTestCase` is 0.0
        // by default so `std::max()` is a no-op.
        timeoutToUse =
            std::max(queryTimePerState,
                     ((double)DynamicSolverTimeoutMinQueryTimePerTestCase));
        KLEE_DEBUG_WITH_TYPE(
            "dynamic_solver_timeout",
            llvm::errs() << "In test generation.\n"
                            "Number of states: "
                         << numberOfStatesLeft
                         << "\nqueryTimePerState: " << queryTimePerState
                         << "\nDynamicSolverTimeoutMinQueryTimePerTestCase: "
                         << DynamicSolverTimeoutMinQueryTimePerTestCase
                         << "\nPicking max of `queryTimePerState` and "
                            "`DynamicSolverTimeoutMinQueryTimePerTestCase`\n");
      }
    } else {
      // Executor should currently be doing path exploration.
      //
      // This tries to prevents terminating really early due to
      // which would happen if there was a static solver timeout
      // and a large amount of path exploration time left.

      // NOTE: `DynamicSolverTimeoutMinQueryTimeDuringPathExploration` is
      // 0.0 by default so `std::max()` is a no-op.
      //
      // FIXME: We could do something more sophisticated here like dividing
      // `timeLeftUntilExecutorToHalt` by the number of active states.
      timeoutToUse = std::max(
          timeLeftUntilExecutorToHalt,
          ((double)DynamicSolverTimeoutMinQueryTimeDuringPathExploration));
      KLEE_DEBUG_WITH_TYPE(
          "dynamic_solver_timeout",
          llvm::errs()
              << "In path exploration.\n"
              << "timeLeftUntilExecutorToHalt: " << timeLeftUntilExecutorToHalt
              << "\nDynamicSolverTimeoutMinQueryTimeDuringPathExploration: "
              << DynamicSolverTimeoutMinQueryTimeDuringPathExploration
              << "\nPicking max of `timeLeftUntilExecutorToHalt` and "
                 "`DynamicSolverTimeoutMinQueryTimeDuringPathExploration`\n");
    }
  }
  assert(isinf(timeoutToUse) == 0);
  assert(!isnan(timeoutToUse));
  if (timeoutToUse > 0.0) {
    s->solver->setCoreSolverTimeout(timeoutToUse);
    KLEE_DEBUG_WITH_TYPE("dynamic_solver_timeout",
                         llvm::errs() << "Using dynamic solver timeout of "
                                      << timeoutToUse << " seconds\n");
  } else {
    KLEE_DEBUG_WITH_TYPE(
        "dynamic_solver_timeout",
        llvm::errs()
            << "0.0 or negative timeout computed. Not invoking solver.\n");
    shouldRunSolver = false;
  }
  return shouldRunSolver;
}
}

/***/
void TimingSolver::setTimeout(double t) {
  if (DynamicSolverTimeout) {
    klee_warning_once(0, "Ignoring set solver timeout request. Using dynamic timeout instead.");
  } else {
    solver->setCoreSolverTimeout(t);
  }
}

bool TimingSolver::evaluate(const ExecutionState& state, ref<Expr> expr,
                            Solver::Validity &result) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? Solver::True : Solver::False;
    return true;
  }

  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  if (!setDynamicTimeout(this)) {
    return false;
  }
  bool success = solver->evaluate(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool TimingSolver::mustBeTrue(const ExecutionState& state, ref<Expr> expr, 
                              bool &result) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE->isTrue() ? true : false;
    return true;
  }

  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  if (!setDynamicTimeout(this)) {
    return false;
  }
  bool success = solver->mustBeTrue(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool TimingSolver::mustBeFalse(const ExecutionState& state, ref<Expr> expr,
                               bool &result) {
  return mustBeTrue(state, Expr::createIsZero(expr), result);
}

bool TimingSolver::mayBeTrue(const ExecutionState& state, ref<Expr> expr, 
                             bool &result) {
  bool res;
  if (!mustBeFalse(state, expr, res))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::mayBeFalse(const ExecutionState& state, ref<Expr> expr, 
                              bool &result) {
  bool res;
  if (!mustBeTrue(state, expr, res))
    return false;
  result = !res;
  return true;
}

bool TimingSolver::getValue(const ExecutionState& state, ref<Expr> expr, 
                            ref<ConstantExpr> &result) {
  // Fast path, to avoid timer and OS overhead.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(expr)) {
    result = CE;
    return true;
  }
  
  sys::TimeValue now = util::getWallTimeVal();

  if (simplifyExprs)
    expr = state.constraints.simplifyExpr(expr);

  if (!setDynamicTimeout(this)) {
    return false;
  }
  bool success = solver->getValue(Query(state.constraints, expr), result);

  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;

  return success;
}

bool 
TimingSolver::getInitialValues(const ExecutionState& state, 
                               const std::vector<const Array*>
                                 &objects,
                               std::vector< std::vector<unsigned char> >
                                 &result) {
  if (objects.empty())
    return true;

  sys::TimeValue now = util::getWallTimeVal();

  if (!setDynamicTimeout(this)) {
    return false;
  }
  bool success = solver->getInitialValues(Query(state.constraints,
                                                ConstantExpr::alloc(0, Expr::Bool)), 
                                          objects, result);
  
  sys::TimeValue delta = util::getWallTimeVal();
  delta -= now;
  stats::solverTime += delta.usec();
  state.queryCost += delta.usec()/1000000.;
  
  return success;
}

std::pair< ref<Expr>, ref<Expr> >
TimingSolver::getRange(const ExecutionState& state, ref<Expr> expr) {
  if (!setDynamicTimeout(this)) {
    // FIXME: Implementation doesn't actually define how to handle the solver
    // not succeeding. Just do this for now. If we do this we will likely
    // trigger a crash.
    return std::make_pair(ref<Expr>(NULL), ref<Expr>(NULL));
  }
  return solver->getRange(Query(state.constraints, expr));
}
