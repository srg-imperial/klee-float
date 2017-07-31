//===-- Z3Solver.cpp -------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#include "klee/Internal/Support/ErrorHandling.h"
#ifdef ENABLE_Z3
#include "Z3Builder.h"
#include "klee/Constraints.h"
#include "klee/Solver.h"
#include "klee/SolverImpl.h"
#include "klee/util/Assignment.h"
#include "klee/util/ExprUtil.h"
#include "../Expr/FindArrayAckermannizationVisitor.h" // FIXME: No relative includes!
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ErrorHandling.h"

namespace {
llvm::cl::opt<std::string> Z3QueryDumpFile(
    "z3-query-dump", llvm::cl::init(""),
    llvm::cl::desc("Dump Z3's representation of the query to the specified path"));

llvm::cl::opt<bool> Z3ValidateModels(
    "z3-validate-models", llvm::cl::init(false),
    llvm::cl::desc("When generating Z3 models validate these against the query"));

llvm::cl::opt<bool> Z3AckermannizeArrays(
    "z3-array-ackermannize", llvm::cl::init(true),
    llvm::cl::desc("Try to ackermannize arrays before building Z3 queries "
                   "(experimental) (default false)"));
}


namespace klee {

class Z3SolverImpl : public SolverImpl {
private:
  Z3Builder *builder;
  double timeout;
  SolverRunStatus runStatusCode;
  llvm::raw_fd_ostream* dumpedQueriesFile;
  ::Z3_params solverParameters;
  // Parameter symbols
  ::Z3_symbol timeoutParamStrSymbol;

  bool internalRunSolver(const Query &,
                         const std::vector<const Array *> *objects,
                         std::vector<std::vector<unsigned char> > *values,
                         bool &hasSolution);
bool validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel);
void ackermannizeArrays(Z3Builder *z3Builder, const Query &query,
                        FindArrayAckermannizationVisitor &faav,
                        std::map<const ArrayAckermannizationInfo *, Z3ASTHandle>
                            &arrayReplacements);

public:
Z3SolverImpl();
~Z3SolverImpl();

char *getConstraintLog(const Query &);
void setCoreSolverTimeout(double _timeout) {
  assert(_timeout >= 0.0 && "timeout must be >= 0");
  timeout = _timeout;

  unsigned int timeoutInMilliSeconds = (unsigned int)((timeout * 1000) + 0.5);
  if (timeoutInMilliSeconds == 0)
    timeoutInMilliSeconds = UINT_MAX;
  Z3_params_set_uint(builder->ctx, solverParameters, timeoutParamStrSymbol,
                     timeoutInMilliSeconds);
  }

  bool computeTruth(const Query &, bool &isValid);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus handleSolverResponse(
      ::Z3_solver theSolver, ::Z3_lbool satisfiable,
      const std::vector<const Array *> *objects,
      std::vector<std::vector<unsigned char> > *values, bool &hasSolution,
      FindArrayAckermannizationVisitor &ffv,
      std::map<const ArrayAckermannizationInfo *, Z3ASTHandle> &arrayReplacements);

  SolverRunStatus getOperationStatusCode();
};

Z3SolverImpl::Z3SolverImpl()
    : builder(new Z3Builder(/*autoClearConstructCache=*/false)), timeout(0.0),
      runStatusCode(SOLVER_RUN_STATUS_FAILURE), dumpedQueriesFile(0) {
  assert(builder && "unable to create Z3Builder");
  solverParameters = Z3_mk_params(builder->ctx);
  Z3_params_inc_ref(builder->ctx, solverParameters);
  timeoutParamStrSymbol = Z3_mk_string_symbol(builder->ctx, "timeout");
  setCoreSolverTimeout(timeout);

  // HACK: This changes Z3's handling of the `to_ieee_bv` function so that
  // we get a signal bit pattern interpretation for NaN. At the time of writing
  // without this option Z3 sometimes generates models which don't satisfy the
  // original constraints.
  //
  // See https://github.com/Z3Prover/z3/issues/740 .
  // https://github.com/Z3Prover/z3/issues/507
  Z3_global_param_set("rewriter.hi_fp_unspecified", "true");

  if (!Z3QueryDumpFile.empty()) {
    std::string error;
    // FIXME: This partially comes from KleeHandler::openOutputFile(). That
    // code should be refactored so we can use it here.
    dumpedQueriesFile = new llvm::raw_fd_ostream(Z3QueryDumpFile.c_str(), error,
                                                 llvm::sys::fs::F_None);
    if (!error.empty()) {
      llvm::errs() << "Error creating file for dumping Z3 queries:" << error
                   << "\n";
      abort();
    }
  }
}

Z3SolverImpl::~Z3SolverImpl() {
  Z3_params_dec_ref(builder->ctx, solverParameters);
  delete builder;

  if (dumpedQueriesFile) {
    dumpedQueriesFile->close();
    delete dumpedQueriesFile;
  }
}

Z3Solver::Z3Solver() : Solver(new Z3SolverImpl()) {}

char *Z3Solver::getConstraintLog(const Query &query) {
  return impl->getConstraintLog(query);
}

void Z3Solver::setCoreSolverTimeout(double timeout) {
  impl->setCoreSolverTimeout(timeout);
}

char *Z3SolverImpl::getConstraintLog(const Query &query) {
  std::vector<Z3ASTHandle> assumptions;
  for (std::vector<ref<Expr> >::const_iterator it = query.constraints.begin(),
                                               ie = query.constraints.end();
       it != ie; ++it) {
    assumptions.push_back(builder->construct(*it));
  }
  ::Z3_ast *assumptionsArray = NULL;
  int numAssumptions = query.constraints.size();
  if (numAssumptions) {
    assumptionsArray = (::Z3_ast *)malloc(sizeof(::Z3_ast) * numAssumptions);
    for (int index = 0; index < numAssumptions; ++index) {
      assumptionsArray[index] = (::Z3_ast)assumptions[index];
    }
  }

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // the negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3ASTHandle formula = Z3ASTHandle(
      Z3_mk_not(builder->ctx, builder->construct(query.expr)), builder->ctx);

  ::Z3_string result = Z3_benchmark_to_smtlib_string(
      builder->ctx,
      /*name=*/"Emited by klee::Z3SolverImpl::getConstraintLog()",
      /*logic=*/"",
      /*status=*/"unknown",
      /*attributes=*/"",
      /*num_assumptions=*/numAssumptions,
      /*assumptions=*/assumptionsArray,
      /*formula=*/formula);

  if (numAssumptions)
    free(assumptionsArray);
  // Client is responsible for freeing the returned C-string
  return strdup(result);
}

bool Z3SolverImpl::computeTruth(const Query &query, bool &isValid) {
  bool hasSolution;
  bool status =
      internalRunSolver(query, /*objects=*/NULL, /*values=*/NULL, hasSolution);
  isValid = !hasSolution;
  return status;
}

bool Z3SolverImpl::computeValue(const Query &query, ref<Expr> &result) {
  std::vector<const Array *> objects;
  std::vector<std::vector<unsigned char> > values;
  bool hasSolution;

  // Find the object used in the expression, and compute an assignment
  // for them.
  findSymbolicObjects(query.expr, objects);
  if (!computeInitialValues(query.withFalse(), objects, values, hasSolution))
    return false;
  assert(hasSolution && "state has invalid constraint set");

  // Evaluate the expression with the computed assignment.
  Assignment a(objects, values);
  result = a.evaluate(query.expr);

  return true;
}

bool Z3SolverImpl::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char> > &values, bool &hasSolution) {
  return internalRunSolver(query, &objects, &values, hasSolution);
}

bool Z3SolverImpl::internalRunSolver(
    const Query &query, const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution) {


  TimerStatIncrementer t(stats::queryTime);
  // TODO: Does making a new solver for each query have a performance
  // impact vs making one global solver and using push and pop?
  // TODO: is the "simple_solver" the right solver to use for
  // best performance?
  Z3_solver theSolver = Z3_mk_solver(builder->ctx);
  Z3_solver_inc_ref(builder->ctx, theSolver);
  Z3_solver_set_params(builder->ctx, theSolver, solverParameters);

  runStatusCode = SOLVER_RUN_STATUS_FAILURE;

  // Try ackermannize the arrays
  std::map<const ArrayAckermannizationInfo*,Z3ASTHandle> arrayReplacements;
  FindArrayAckermannizationVisitor faav(/*recursive=*/false);
  if (Z3AckermannizeArrays) {
    ackermannizeArrays(this->builder, query, faav, arrayReplacements);
  }

  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    Z3_solver_assert(builder->ctx, theSolver, builder->construct(*it));
  }

  ++stats::queries;
  if (objects)
    ++stats::queryCounterexamples;

  Z3ASTHandle z3QueryExpr =
      Z3ASTHandle(builder->construct(query.expr), builder->ctx);

  // KLEE Queries are validity queries i.e.
  // ∀ X Constraints(X) → query(X)
  // but Z3 works in terms of satisfiability so instead we ask the
  // negation of the equivalent i.e.
  // ∃ X Constraints(X) ∧ ¬ query(X)
  Z3_solver_assert(
      builder->ctx, theSolver,
      Z3ASTHandle(Z3_mk_not(builder->ctx, z3QueryExpr), builder->ctx));

  // Assert an generated side constraints we have to this last so that all other
  // constraints have been traversed so we have all the side constraints needed.
  for (std::vector<Z3ASTHandle>::iterator it = builder->sideConstraints.begin(),
                                          ie = builder->sideConstraints.end();
       it != ie; ++it) {
    Z3ASTHandle sideConstraint = *it;
    Z3_solver_assert(builder->ctx, theSolver, sideConstraint);
  }

  if (dumpedQueriesFile) {
    *dumpedQueriesFile << "; start Z3 query\n";
    *dumpedQueriesFile << Z3_solver_to_string(builder->ctx, theSolver);
    *dumpedQueriesFile << "(check-sat)\n";
    *dumpedQueriesFile << "(reset)\n";
    *dumpedQueriesFile << "; end Z3 query\n\n";
    dumpedQueriesFile->flush();
  }

  ::Z3_lbool satisfiable = Z3_solver_check(builder->ctx, theSolver);
  runStatusCode = handleSolverResponse(theSolver, satisfiable, objects, values,
                                       hasSolution, faav, arrayReplacements);

  if (Z3AckermannizeArrays) {
    // Remove any replacements we made as accumulating these across
    // solver runs might not be valid.
    builder->clearReplacements();
  }

  Z3_solver_dec_ref(builder->ctx, theSolver);
  // Clear the builder's cache to prevent memory usage exploding.
  // By using ``autoClearConstructCache=false`` and clearning now
  // we allow Z3_ast expressions to be shared from an entire
  // ``Query`` rather than only sharing within a single call to
  // ``builder->construct()``.
  builder->clearConstructCache();

  // Clear any generated side constraints could break subsequent queries
  // if we assert them in the future.
  builder->clearSideConstraints();

  if (runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE ||
      runStatusCode == SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE) {
    if (hasSolution) {
      ++stats::queriesInvalid;
    } else {
      ++stats::queriesValid;
    }
    return true; // success
  }
  return false; // failed
}

SolverImpl::SolverRunStatus Z3SolverImpl::handleSolverResponse(
    ::Z3_solver theSolver, ::Z3_lbool satisfiable,
    const std::vector<const Array *> *objects,
    std::vector<std::vector<unsigned char> > *values, bool &hasSolution,
    FindArrayAckermannizationVisitor &ffv,
    std::map<const ArrayAckermannizationInfo *, Z3ASTHandle> &arrayReplacements) {
  switch (satisfiable) {
  case Z3_L_TRUE: {
    hasSolution = true;
    if (!objects) {
      // No assignment is needed
      assert(values == NULL);
      return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
    }
    assert(values && "values cannot be nullptr");
    ::Z3_model theModel = Z3_solver_get_model(builder->ctx, theSolver);
    assert(theModel && "Failed to retrieve model");
    Z3_model_inc_ref(builder->ctx, theModel);
    values->reserve(objects->size());
    for (std::vector<const Array *>::const_iterator it = objects->begin(),
                                                    ie = objects->end();
         it != ie; ++it) {
      const Array *array = *it;
      std::vector<unsigned char> data;

      // See if there is any ackermannization info for this array
      const std::vector<ArrayAckermannizationInfo>* aais = NULL;
      FindArrayAckermannizationVisitor::ArrayToAckermannizationInfoMapTy::
          const_iterator aiii = ffv.ackermannizationInfo.find(array);
      if (aiii != ffv.ackermannizationInfo.end()) {
        aais = &(aiii->second);
      }

      data.reserve(array->size);
      for (unsigned offset = 0; offset < array->size; offset++) {
        // We can't use Z3ASTHandle here so have to do ref counting manually
        ::Z3_ast arrayElementExpr;
        Z3ASTHandle initial_read = Z3ASTHandle();
        if (aais && aais->size() > 0) {
          // Look through the possible ackermannized regions of the array
          // and find the region that corresponds to this byte.
          for (std::vector<ArrayAckermannizationInfo>::const_iterator
                   i = aais->begin(),
                   ie = aais->end();
               i != ie; ++i) {
            const ArrayAckermannizationInfo* info = &(*i);
            if (!(info->containsByte(offset))) {
              continue;
            }

            // This is the ackermannized region for this offset.
            Z3ASTHandle replacementVariable = arrayReplacements[info];
            assert((offset*8) >= info->contiguousLSBitIndex);
            unsigned bitOffsetToReadWithinVariable = (offset*8) - info->contiguousLSBitIndex;
            assert(bitOffsetToReadWithinVariable < info->getWidth());
            // Extract the byte
            initial_read = Z3ASTHandle(
                Z3_mk_extract(
                    builder->ctx, /*high=*/bitOffsetToReadWithinVariable + 7,
                    /*low=*/bitOffsetToReadWithinVariable, replacementVariable),
                builder->ctx);
            break;
          }
          if (Z3_ast(initial_read) == NULL) {
            // The array was ackermannized but this particular byte
            // wasn't which implies it wasn't used in the query.
            // This implies the value for this byte doesn't matter
            // so just assign zero for this byte.
            data.push_back((unsigned char) 0);
            continue;
          }
        } else {
          // This array wasn't ackermannized.
          initial_read = builder->getInitialRead(array, offset);
        }

        bool successfulEval =
            Z3_model_eval(builder->ctx, theModel, initial_read,
                          /*model_completion=*/Z3_TRUE, &arrayElementExpr);
        assert(successfulEval && "Failed to evaluate model");
        Z3_inc_ref(builder->ctx, arrayElementExpr);
        assert(Z3_get_ast_kind(builder->ctx, arrayElementExpr) ==
                   Z3_NUMERAL_AST &&
               "Evaluated expression has wrong sort");

        int arrayElementValue = 0;
        bool successGet = Z3_get_numeral_int(builder->ctx, arrayElementExpr,
                                             &arrayElementValue);
        assert(successGet && "failed to get value back");
        assert(arrayElementValue >= 0 && arrayElementValue <= 255 &&
               "Integer from model is out of range");
        data.push_back(arrayElementValue);
        Z3_dec_ref(builder->ctx, arrayElementExpr);
      }
      values->push_back(data);
    }

    // Validate the model if requested
    if (Z3ValidateModels) {
      bool success = validateZ3Model(theSolver, theModel);
      if (!success) {
        builder->closeInteractionLog();
        abort();
      }
    }

    Z3_model_dec_ref(builder->ctx, theModel);
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_SOLVABLE;
  }
  case Z3_L_FALSE:
    hasSolution = false;
    return SolverImpl::SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE;
  case Z3_L_UNDEF: {
    ::Z3_string reason =
        ::Z3_solver_get_reason_unknown(builder->ctx, theSolver);
    if (strcmp(reason, "timeout") == 0 || strcmp(reason, "canceled") == 0 ||
        strcmp(reason, "(resource limits reached)") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_TIMEOUT;
    }
    if (strcmp(reason, "unknown") == 0) {
      return SolverImpl::SOLVER_RUN_STATUS_FAILURE;
    }
    klee_warning("Unexpected solver failure. Reason is \"%s,\"\n", reason);
    builder->closeInteractionLog();
    abort();
  }
  default:
    llvm_unreachable("unhandled Z3 result");
  }
}

bool Z3SolverImpl::validateZ3Model(::Z3_solver &theSolver, ::Z3_model &theModel) {
  bool success = true;
  ::Z3_ast_vector constraints =
      Z3_solver_get_assertions(builder->ctx, theSolver);
  Z3_ast_vector_inc_ref(builder->ctx, constraints);

  unsigned size = Z3_ast_vector_size(builder->ctx, constraints);

  for (unsigned index = 0; index < size; ++index) {
    Z3ASTHandle constraint = Z3ASTHandle(
        Z3_ast_vector_get(builder->ctx, constraints, index), builder->ctx);

    ::Z3_ast rawEvaluatedExpr;
    bool successfulEval =
        Z3_model_eval(builder->ctx, theModel, constraint,
                      /*model_completion=*/Z3_TRUE, &rawEvaluatedExpr);
    assert(successfulEval && "Failed to evaluate model");

    // Use handle to do ref-counting.
    Z3ASTHandle evaluatedExpr(rawEvaluatedExpr, builder->ctx);

    Z3SortHandle sort =
        Z3SortHandle(Z3_get_sort(builder->ctx, evaluatedExpr), builder->ctx);
    assert(Z3_get_sort_kind(builder->ctx, sort) == Z3_BOOL_SORT &&
           "Evaluated expression has wrong sort");

    Z3_lbool evaluatedValue =
        Z3_get_bool_value(builder->ctx, evaluatedExpr);
    if (evaluatedValue != Z3_L_TRUE) {
      llvm::errs() << "Validating model failed:\n"
                   << "The expression:\n";
      constraint.dump();
      llvm::errs() << "evaluated to \n";
      evaluatedExpr.dump();
      llvm::errs() << "But should be true\n";
      success = false;
    }
  }

  if (!success) {
    llvm::errs() << "Solver state:\n" << Z3_solver_to_string(builder->ctx, theSolver) << "\n";
    llvm::errs() << "Model:\n" << Z3_model_to_string(builder->ctx, theModel) << "\n";
  }

  Z3_ast_vector_dec_ref(builder->ctx, constraints);
  return success;
}

void Z3SolverImpl::ackermannizeArrays(
    Z3Builder *z3Builder, const Query &query,
    FindArrayAckermannizationVisitor &faav,
    std::map<const ArrayAckermannizationInfo *, Z3ASTHandle>
        &arrayReplacements) {
  for (ConstraintManager::const_iterator it = query.constraints.begin(),
                                         ie = query.constraints.end();
       it != ie; ++it) {
    faav.visit(*it);
  }
  faav.visit(query.expr);
  for (FindArrayAckermannizationVisitor::ArrayToAckermannizationInfoMapTy::
           const_iterator aaii = faav.ackermannizationInfo.begin(),
                          aaie = faav.ackermannizationInfo.end();
       aaii != aaie; ++aaii) {
    const std::vector<ArrayAckermannizationInfo> &replacements = aaii->second;
    for (std::vector<ArrayAckermannizationInfo>::const_iterator
             i = replacements.begin(),
             ie = replacements.end();
         i != ie; ++i) {
      // Taking a pointer like this is dangerous. If the std::vector<> gets
      // resized the data might be invalidated.
      const ArrayAckermannizationInfo *aaInfo = &(*i); // Safe?
      // Replace with variable
      std::string str;
      llvm::raw_string_ostream os(str);
      os << aaInfo->getArray()->name << "_ackermann";
      assert(aaInfo->toReplace.size() > 0);
      Z3ASTHandle replacementVar;
      for (ExprHashSet::const_iterator ei = aaInfo->toReplace.begin(),
                                       ee = aaInfo->toReplace.end();
           ei != ee; ++ei) {
        ref<Expr> toReplace = *ei;
        if (replacementVar.isNull()) {
          replacementVar = z3Builder->getFreshBitVectorVariable(
              toReplace->getWidth(), os.str().c_str());
        }
        bool success = z3Builder->addReplacementExpr(toReplace, replacementVar);
        assert(success && "Failed to add replacement variable");
      }
      arrayReplacements[aaInfo] = replacementVar;
    }
  }
}

SolverImpl::SolverRunStatus Z3SolverImpl::getOperationStatusCode() {
  return runStatusCode;
}
}
#endif // ENABLE_Z3
