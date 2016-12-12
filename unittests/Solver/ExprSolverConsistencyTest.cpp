//===-- ExprSolverConsistencyTest.cpp -------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "klee/CommandLine.h"
#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/ArrayCache.h"
#include "klee/util/Assignment.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>

using namespace klee;

namespace {
ArrayCache ac;

Expr::Width widthsToTest[] = {Expr::Int16, Expr::Int32, Expr::Int64, Expr::Fl80,
                              128};
}

std::string getAPIntAsHex(const llvm::APInt& api) {
  llvm::SmallVector<char, 32> data;
  api.toStringUnsigned(data, /*radix=*/16);
  std::string stringValue = std::string(data.begin(), data.end());
  stringValue = "0x" + stringValue;
  return stringValue;
}

void testBinaryFPArithWithNaNResult(Expr::Width width, Expr::Kind kind) {
  llvm::errs() << "Testing kind:" << kind << ", width:" << width << "\n";
  const Array *initialFloatArray = ac.CreateArray("float", width);
  ref<Expr> readOfInitialFloatArray =
      Expr::createTempRead(initialFloatArray, width);
  const Array *resultFloatArray = ac.CreateArray("result_float", width);
  ref<Expr> readOfResultFloatArray =
      Expr::createTempRead(resultFloatArray, width);

  // Create 1.0f constant
  llvm::APFloat oneFloat(1.0f);

  // Upcast to appropriate width
  bool losesInfo = true;
  switch (width) {
    case Expr::Int16:
      oneFloat.convert(llvm::APFloat::IEEEhalf, llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      break;
    case Expr::Int32:
      losesInfo = false;
      break;
    case Expr::Int64:
      oneFloat.convert(llvm::APFloat::IEEEdouble, llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      break;
    case Expr::Fl80:
      oneFloat.convert(llvm::APFloat::x87DoubleExtended, llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      break;
    case Expr::Int128:
      oneFloat.convert(llvm::APFloat::IEEEquad, llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      break;
    default:
      llvm_unreachable("wrong width");
  }
  ASSERT_FALSE(losesInfo);
  ASSERT_EQ(oneFloat.bitcastToAPInt().getBitWidth(), width) << "wrong width";

  // Create constraints
  ref<Expr> oneExpr = ConstantExpr::alloc(oneFloat);

  // Create operation (e.g. FAdd)
  std::vector<Expr::CreateArg> args;
  args.push_back(Expr::CreateArg(readOfInitialFloatArray));
  args.push_back(Expr::CreateArg(oneExpr));
  args.push_back(Expr::CreateArg(llvm::APFloat::rmNearestTiesToEven));
  ref<Expr> op = Expr::createFromKind(kind, args);
  ASSERT_TRUE(op.get() != NULL);
  ref<Expr> isNaN = IsNaNExpr::create(op);
  ref<Expr> resultArrayEqual = EqExpr::create(readOfResultFloatArray, op);

  Solver *solver = klee::createCoreSolver(CoreSolverToUse);
  ConstraintManager constraints;
  constraints.addConstraint(isNaN);
  constraints.addConstraint(resultArrayEqual);

  // Get an assignment
  ref<Expr> falseExpr = ConstantExpr::alloc(0, Expr::Bool);
  Query q(constraints, falseExpr);
  std::vector<const Array *> arrays;
  std::vector<std::vector<unsigned char> > bytes;
  arrays.push_back(initialFloatArray);
  arrays.push_back(resultFloatArray);
  bool success = solver->getInitialValues(q, arrays, bytes);
  EXPECT_EQ(success, true) << "Constraint solving failed";

  Assignment as(arrays, bytes, /*_allowFreeValues=*/true);

  // Check the NaN
  ref<Expr> foldedConstraint = as.evaluate(isNaN);
  const ConstantExpr* asCon = dyn_cast<ConstantExpr>(foldedConstraint);
  ASSERT_TRUE(asCon != NULL) << "Can't evaluate constraint as Constant";
  ASSERT_TRUE(asCon->isTrue()) << "Constraint should be true";

  // Get the value of the initialFloatArray
  ref<Expr> initialFloatArrayValue = as.evaluate(readOfInitialFloatArray);
  const ConstantExpr *initialFloatArrayValueFolded =
      dyn_cast<ConstantExpr>(initialFloatArrayValue);
  ASSERT_TRUE(initialFloatArrayValueFolded != NULL)
      << "Can't evaluate as constant";
  ASSERT_EQ(initialFloatArrayValueFolded->getWidth(), width);

  // Now evaluate the operation using the initial array values
  // provided by Z3 using KLEE's Expr language.
  ref<Expr> operationKleeEval = as.evaluate(op);
  const ConstantExpr *operationKleeEvalFolded =
      dyn_cast<ConstantExpr>(operationKleeEval);
  ASSERT_TRUE(operationKleeEvalFolded != NULL);
  ASSERT_EQ(operationKleeEvalFolded->getWidth(), width);

  // Get the value that Z3 expects the operation to evaluate to
  ref<Expr> resultFloatArrayValue = as.evaluate(readOfResultFloatArray);
  const ConstantExpr *resultFloatArrayValueFolded =
      dyn_cast<ConstantExpr>(resultFloatArrayValue);
  ASSERT_TRUE(resultFloatArrayValueFolded != NULL) << "Can't evaluate as constant";
  ASSERT_EQ(resultFloatArrayValueFolded->getWidth(), width);

  // Now assert that the same thing was computed
  // Dump their bit representations for debugging.
  std::string operationKleeEvalStr = getAPIntAsHex(operationKleeEvalFolded->getAPValue());
  std::string resultFloatArrayStr = getAPIntAsHex(resultFloatArrayValueFolded->getAPValue());
  ASSERT_EQ(operationKleeEvalStr, resultFloatArrayStr);
}

void testUnaryFPArithWithNaNResult(Expr::Width width, Expr::Kind kind,
                                   bool needsRoundingMode,
                                   bool operandMustBeNaN) {
  llvm::errs() << "Testing kind:" << kind << ", width:" << width << "\n";
  const Array *initialFloatArray = ac.CreateArray("float", width);
  ref<Expr> readOfInitialFloatArray =
      Expr::createTempRead(initialFloatArray, width);
  const Array *resultFloatArray = ac.CreateArray("result_float", width);
  ref<Expr> readOfResultFloatArray =
      Expr::createTempRead(resultFloatArray, width);

  // Create 1.0f constant
  llvm::APFloat oneFloat(1.0f);

  // Create constraints
  ref<Expr> oneExpr = ConstantExpr::alloc(oneFloat);

  // Create operation (e.g. FSqrt)
  std::vector<Expr::CreateArg> args;
  args.push_back(Expr::CreateArg(readOfInitialFloatArray));
  if (needsRoundingMode)
    args.push_back(Expr::CreateArg(llvm::APFloat::rmNearestTiesToEven));
  ref<Expr> op = Expr::createFromKind(kind, args);
  ASSERT_TRUE(op.get() != NULL);
  ref<Expr> isNaN = IsNaNExpr::create(op);
  ref<Expr> isNaNOperand = IsNaNExpr::create(readOfInitialFloatArray);
  ref<Expr> resultArrayEqual = EqExpr::create(readOfResultFloatArray, op);

  Solver *solver = klee::createCoreSolver(CoreSolverToUse);
  ConstraintManager constraints;
  constraints.addConstraint(isNaN);
  constraints.addConstraint(resultArrayEqual);
  if (operandMustBeNaN) {
    // This needed for operations like sqrt() where operands
    // other than NaN (e.g. -ve numbers) can give a NaN result
    constraints.addConstraint(isNaNOperand);
  }

  // Get an assignment
  ref<Expr> falseExpr = ConstantExpr::alloc(0, Expr::Bool);
  Query q(constraints, falseExpr);
  std::vector<const Array *> arrays;
  std::vector<std::vector<unsigned char> > bytes;
  arrays.push_back(initialFloatArray);
  arrays.push_back(resultFloatArray);
  bool success = solver->getInitialValues(q, arrays, bytes);
  EXPECT_EQ(success, true) << "Constraint solving failed";

  Assignment as(arrays, bytes, /*_allowFreeValues=*/true);

  // Check the NaN
  ref<Expr> foldedConstraint = as.evaluate(isNaN);
  const ConstantExpr *asCon = dyn_cast<ConstantExpr>(foldedConstraint);
  ASSERT_TRUE(asCon != NULL) << "Can't evaluate constraint as Constant";
  ASSERT_TRUE(asCon->isTrue()) << "Constraint should be true";

  // Get the value of the initialFloatArray
  ref<Expr> initialFloatArrayValue = as.evaluate(readOfInitialFloatArray);
  const ConstantExpr *initialFloatArrayValueFolded =
      dyn_cast<ConstantExpr>(initialFloatArrayValue);
  ASSERT_TRUE(initialFloatArrayValueFolded != NULL)
      << "Can't evaluate as constant";
  ASSERT_EQ(initialFloatArrayValueFolded->getWidth(), width);

  // Now evaluate the operation using the initial array values
  // provided by Z3 using KLEE's Expr language.
  ref<Expr> operationKleeEval = as.evaluate(op);
  const ConstantExpr *operationKleeEvalFolded =
      dyn_cast<ConstantExpr>(operationKleeEval);
  ASSERT_TRUE(operationKleeEvalFolded != NULL);
  ASSERT_EQ(operationKleeEvalFolded->getWidth(), width);

  // Get the value that Z3 expects the operation to evaluate to
  ref<Expr> resultFloatArrayValue = as.evaluate(readOfResultFloatArray);
  const ConstantExpr *resultFloatArrayValueFolded =
      dyn_cast<ConstantExpr>(resultFloatArrayValue);
  ASSERT_TRUE(resultFloatArrayValueFolded != NULL)
      << "Can't evaluate as constant";
  ASSERT_EQ(resultFloatArrayValueFolded->getWidth(), width);

  // Now assert that the same thing was computed
  // Dump their bit representations for debugging.
  std::string operationKleeEvalStr =
      getAPIntAsHex(operationKleeEvalFolded->getAPValue());
  std::string resultFloatArrayStr =
      getAPIntAsHex(resultFloatArrayValueFolded->getAPValue());
  ASSERT_EQ(operationKleeEvalStr, resultFloatArrayStr);
}

// TODO: Test other operations

TEST(ExprSolverConsistencyTest, FAddWithNaNResult) {
  for (unsigned i= 0; i < sizeof(widthsToTest)/sizeof(Expr::Width); ++i) {
    testBinaryFPArithWithNaNResult(widthsToTest[i], Expr::FAdd);
  }
}

TEST(ExprSolverConsistencyTest, FSubWithNaNResult) {
  for (unsigned i= 0; i < sizeof(widthsToTest)/sizeof(Expr::Width); ++i) {
    testBinaryFPArithWithNaNResult(widthsToTest[i], Expr::FSub);
  }
}

TEST(ExprSolverConsistencyTest, FMulWithNaNResult) {
  for (unsigned i= 0; i < sizeof(widthsToTest)/sizeof(Expr::Width); ++i) {
    testBinaryFPArithWithNaNResult(widthsToTest[i], Expr::FMul);
  }
}

TEST(ExprSolverConsistencyTest, FDivWithNaNResult) {
  for (unsigned i= 0; i < sizeof(widthsToTest)/sizeof(Expr::Width); ++i) {
    testBinaryFPArithWithNaNResult(widthsToTest[i], Expr::FDiv);
  }
}

TEST(ExprSolverConsistencyTest, FSqrtWithNaNResult) {
  /*
  for (unsigned i = 0; i < sizeof(widthsToTest) / sizeof(Expr::Width); ++i) {
    if (widthsToTest[i] == 16) {
      // FIXME: Support eval of sqrt with IEEEhalf.
      continue;
    }
    testUnaryFPArithWithNaNResult(Expr::Int32, Expr::FSqrt, true, true);
  }
*/
  // FIXME: Can't test Expr::Int16
  // FIXME: Testing other widths is very expensive so don't do it
  // for now.
  testUnaryFPArithWithNaNResult(Expr::Int32, Expr::FSqrt,
                                /*needsRoundingMode=*/true,
                                /*operandMustBeNaN=*/true);
}
