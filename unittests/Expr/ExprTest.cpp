//===-- ExprTest.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include "gtest/gtest.h"

#include "klee/Expr.h"
#include "klee/util/ArrayCache.h"
#include <fenv.h>
#include <inttypes.h>
#include <math.h>

using namespace klee;

namespace {

ref<Expr> getConstant(int value, Expr::Width width) {
  int64_t ext = value;
  uint64_t trunc = ext & (((uint64_t) -1LL) >> (64 - width));
  return ConstantExpr::create(trunc, width);
}

TEST(ExprTest, BasicConstruction) {
  EXPECT_EQ(ref<Expr>(ConstantExpr::alloc(0, 32)),
            SubExpr::create(ConstantExpr::alloc(10, 32),
                            ConstantExpr::alloc(10, 32)));
}

TEST(ExprTest, ConcatExtract) {
  ArrayCache ac;
  const Array *array = ac.CreateArray("arr0", 256);
  ref<Expr> read8 = Expr::createTempRead(array, 8);
  const Array *array2 = ac.CreateArray("arr1", 256);
  ref<Expr> read8_2 = Expr::createTempRead(array2, 8);
  ref<Expr> c100 = getConstant(100, 8);

  ref<Expr> concat1 = ConcatExpr::create4(read8, read8, c100, read8_2);
  EXPECT_EQ(2U, concat1->getNumKids());
  EXPECT_EQ(2U, concat1->getKid(1)->getNumKids());
  EXPECT_EQ(2U, concat1->getKid(1)->getKid(1)->getNumKids());

  ref<Expr> extract1 = ExtractExpr::create(concat1, 8, 16);
  EXPECT_EQ(Expr::Concat, extract1->getKind());
  EXPECT_EQ(read8, extract1->getKid(0));
  EXPECT_EQ(c100, extract1->getKid(1));

  ref<Expr> extract2 = ExtractExpr::create(concat1, 6, 26);
  EXPECT_EQ( Expr::Concat, extract2->getKind());
  EXPECT_EQ( read8, extract2->getKid(0));
  EXPECT_EQ( Expr::Concat, extract2->getKid(1)->getKind());
  EXPECT_EQ( read8, extract2->getKid(1)->getKid(0));
  EXPECT_EQ( Expr::Concat, extract2->getKid(1)->getKid(1)->getKind());
  EXPECT_EQ( c100, extract2->getKid(1)->getKid(1)->getKid(0));
  EXPECT_EQ( Expr::Extract, extract2->getKid(1)->getKid(1)->getKid(1)->getKind());
  
  ref<Expr> extract3 = ExtractExpr::create(concat1, 24, 1);
  EXPECT_EQ(Expr::Extract, extract3->getKind());

  ref<Expr> extract4 = ExtractExpr::create(concat1, 27, 2);
  EXPECT_EQ(Expr::Extract, extract4->getKind());
  const ExtractExpr* tmp = cast<ExtractExpr>(extract4);
  EXPECT_EQ(3U, tmp->offset);
  EXPECT_EQ(2U, tmp->getWidth());

  ref<Expr> extract5 = ExtractExpr::create(concat1, 17, 5);
  EXPECT_EQ(Expr::Extract, extract5->getKind());

  ref<Expr> extract6 = ExtractExpr::create(concat1, 3, 26);
  EXPECT_EQ(Expr::Concat, extract6->getKind());
  EXPECT_EQ(Expr::Extract, extract6->getKid(0)->getKind());
  EXPECT_EQ(Expr::Concat, extract6->getKid(1)->getKind());
  EXPECT_EQ(read8, extract6->getKid(1)->getKid(0));
  EXPECT_EQ(Expr::Concat, extract6->getKid(1)->getKid(1)->getKind());
  EXPECT_EQ(c100, extract6->getKid(1)->getKid(1)->getKid(0));
  EXPECT_EQ(Expr::Extract, extract6->getKid(1)->getKid(1)->getKid(1)->getKind());

  ref<Expr> concat10 = ConcatExpr::create4(read8, c100, c100, read8);    
  ref<Expr> extract10 = ExtractExpr::create(concat10, 8, 16);
  EXPECT_EQ(Expr::Constant, extract10->getKind());
}

TEST(ExprTest, ExtractConcat) {
  ArrayCache ac;
  const Array *array = ac.CreateArray("arr2", 256);
  ref<Expr> read64 = Expr::createTempRead(array, 64);

  const Array *array2 = ac.CreateArray("arr3", 256);
  ref<Expr> read8_2 = Expr::createTempRead(array2, 8);
  
  ref<Expr> extract1 = ExtractExpr::create(read64, 36, 4);
  ref<Expr> extract2 = ExtractExpr::create(read64, 32, 4);
  
  ref<Expr> extract3 = ExtractExpr::create(read64, 12, 3);
  ref<Expr> extract4 = ExtractExpr::create(read64, 10, 2);
  ref<Expr> extract5 = ExtractExpr::create(read64, 2, 8);
   
  ref<Expr> kids1[6] = { extract1, extract2,
			 read8_2,
			 extract3, extract4, extract5 };
  ref<Expr> concat1 = ConcatExpr::createN(6, kids1);
  EXPECT_EQ(29U, concat1->getWidth());
  
  ref<Expr> extract6 = ExtractExpr::create(read8_2, 2, 5);
  ref<Expr> extract7 = ExtractExpr::create(read8_2, 1, 1);
  
  ref<Expr> kids2[3] = { extract1, extract6, extract7 };
  ref<Expr> concat2 = ConcatExpr::createN(3, kids2);
  EXPECT_EQ(10U, concat2->getWidth());
  EXPECT_EQ(Expr::Extract, concat2->getKid(0)->getKind());
  EXPECT_EQ(Expr::Extract, concat2->getKid(1)->getKind());
}

/*

This test case is motivated by an inconsistency in a model
produced by Z3 which it claims is satisfiable but when
fed back into KLEE's expression language we get unsatisfiable.

Query Expression evaluated to true when using assignment
Expression:
(Eq false
     (Slt 0
          (ZExt w32 (Slt 0
                         (SExt w32 (Extract w8 0 (FAdd w32 (ReadLSB w32 0 f)
                                                           1.0E+0)))))))
Assignment:
f
[0,8,128,127,]
Query with assignment as constraints:
; Emited by klee::Z3SolverImpl::getConstraintLog()
(set-info :status unknown)
(declare-fun f0 () (Array (_ BitVec 32) (_ BitVec 8)))
(assert
 (let ((?x184 (select f0 (_ bv0 32))))
 (= (_ bv0 8) ?x184)))
(assert
 (let ((?x407 (select f0 (_ bv1 32))))
 (= (_ bv8 8) ?x407)))
(assert
 (let ((?x1733 (select f0 (_ bv2 32))))
 (= (_ bv128 8) ?x1733)))
(assert
 (let ((?x2122 (select f0 (_ bv3 32))))
 (= (_ bv127 8) ?x2122)))
(assert
 (let ((?x1733 (select f0 (_ bv2 32))))
(let ((?x2122 (select f0 (_ bv3 32))))
(let ((?x1001 (concat ?x2122 (concat ?x1733 (concat (select f0 (_ bv1 32)) (select f0 (_ bv0 32)))))))
(let ((?x1908 ((_ extract 7 0) (to_ieee_bv (fp.add roundNearestTiesToEven ((_ to_fp 8 24) ?x1001) ((_ to_fp 8 24) (_ bv1065353216 32)))))))
(not (not (bvslt (_ bv0 32) (ite (bvslt (_ bv0 32) ((_ sign_extend 24) ?x1908)) (_ bv1 32) (_ bv0 32))))))))))
(check-sat)
*/
TEST(ExprTest, FAddNaN) {
  // 32-bit float with bit value 0x7f800800 which is a NaN.
  uint32_t lhsBits = 0x7f800800;
  float lhsAsNativeFloat = 0.0f;
  EXPECT_EQ(sizeof(lhsBits), sizeof(lhsAsNativeFloat));
  memcpy(&lhsAsNativeFloat, &lhsBits, sizeof(lhsAsNativeFloat));

  int x = issignaling(lhsAsNativeFloat);
  EXPECT_EQ(1, issignaling(lhsAsNativeFloat)); // Not signaling
  EXPECT_EQ(1, x);

  EXPECT_TRUE(isnan(lhsAsNativeFloat));
  printf("lhs as native float: %f\n", lhsAsNativeFloat);
  printf("lhs as native bits: 0x%" PRIx32 "\n" , lhsBits);

  ref<ConstantExpr> lhs = ConstantExpr::create((uint64_t) lhsBits, Expr::Int32);
  llvm::APFloat lhsAsLLVMFloat = lhs->getAPFloatValue();
  // If we follow IEEE-754 2008 the most significant bit of the significand
  // in the binary representation is the "is_quiet" bit. As that is zero here
  // that implies this is a signaling NaN.
  EXPECT_TRUE(lhsAsLLVMFloat.isSignaling());
  {
    llvm::SmallVector<char, 16> sv;
    lhsAsLLVMFloat.toString(sv);
    std::string lhsAsString(sv.begin(), sv.end());
    llvm::errs() << "LHS as APFloat: " << lhsAsString << "\n";
  }

  // 32-bit float with bit value 0x3f800000 which is 1.0f
  uint32_t rhsBits = 0x3f800000;
  float rhsAsNativeFloat = 0.0f;
  EXPECT_EQ(sizeof(rhsBits), sizeof(rhsAsNativeFloat));
  memcpy(&rhsAsNativeFloat, &rhsBits, sizeof(rhsAsNativeFloat));
  EXPECT_EQ(1.0f, rhsAsNativeFloat);
  printf("rhs as native float: %f\n", rhsAsNativeFloat);
  printf("rhs as native bits: 0x%" PRIx32 "\n" , rhsBits);
  ref<ConstantExpr> rhs = ConstantExpr::create((uint64_t) rhsBits, Expr::Int32);
  llvm::APFloat rhsAsLLVMFloat = rhs->getAPFloatValue();
  {
    llvm::SmallVector<char, 16> sv;
    rhsAsLLVMFloat.toString(sv);
    std::string rhsAsString(sv.begin(), sv.end());
    llvm::errs() << "RHS as APFloat: " << rhsAsString << "\n";
  }

	// Switch to round-to-nearest ties to even if we aren't already
  // using it
	int result = fesetround(FE_TONEAREST);
  EXPECT_EQ(0, result);

  // Do addition natively
  int clearExceptResult = feclearexcept(FE_INVALID);
	EXPECT_EQ(0, clearExceptResult);
  EXPECT_EQ(0, fetestexcept(FE_INVALID)); // No exception raised yet
  float resultAsNativeFloat = lhsAsNativeFloat + rhsAsNativeFloat;
  // IEEE-754 2008 7.2 says that the result should be a qNaN
  EXPECT_EQ(0, issignaling(resultAsNativeFloat));

  // Should be raised because lhsAsNativeFloat is a signalling NaN
  EXPECT_EQ(FE_INVALID, fetestexcept(FE_INVALID));

  uint32_t resultBits = 0;
  memcpy(&resultBits, &resultAsNativeFloat, sizeof(resultAsNativeFloat));
  printf("result as native float: %f\n", resultAsNativeFloat);
  printf("result as native bits: 0x%" PRIx32 "\n" , resultBits);

  // Do addition using LLVM's APFloat
  llvm::APFloat resultAsLLVMFloat(lhsAsLLVMFloat);
  llvm::APFloat::opStatus status =
      resultAsLLVMFloat.add(rhsAsLLVMFloat, llvm::APFloat::rmNearestTiesToEven);
  // FIXME: This is wrong. It looks like a bug in LLVM. If we have a signalling
  // NaN then doing an operation with it should raise an Invalid Operation
  // exception.
  EXPECT_EQ(llvm::APFloat::opOK, status);
  EXPECT_TRUE(resultAsLLVMFloat.isSignaling());
  {
    llvm::SmallVector<char, 16> sv;
    resultAsLLVMFloat.toString(sv);
    std::string resultAsString(sv.begin(), sv.end());
    llvm::errs() << "Result as APFloat: " << resultAsString << "\n";
  }

  // Show as raw bits
  llvm::APInt resultAsAPInt = resultAsLLVMFloat.bitcastToAPInt();
  EXPECT_EQ(sizeof(float)*8, resultAsAPInt.getBitWidth());
  uint64_t rawBits = resultAsAPInt.getZExtValue();
  EXPECT_EQ((uint64_t) 0, ((uint64_t) 0xffffffff00000000) & rawBits);
  uint32_t rawBits32 = (uint32_t) rawBits;
  printf("result as LLVM APFloat native bits: 0x%" PRIx32 "\n" , rawBits32);

  /*
Evaluate constant in Z3

(declare-fun nanBits () (_ BitVec 32))
(declare-fun resultBits () (_ BitVec 32))
(assert (= nanBits #x7f800800))
(assert
(let ((?x1908 (to_ieee_bv (fp.add roundNearestTiesToEven ((_ to_fp 8 24) nanBits) ((_ to_fp 8 24) #x3f800000)))))
(= ?x1908 resultBits)
))
(check-sat)
(get-model)
  */
// This the value in the model that Z3 gives back.
uint32_t rawBitsZ3result = 0x7f800001;
printf("result from a Z3 model as bits: 0x%" PRIx32 "\n" , rawBitsZ3result);
// Here is another model that Z3 gave back at some point (see constraints above).
uint32_t otherZ3result = 0x7f800800;
printf("result from another Z3 model as bits: 0x%" PRIx32 "\n" , otherZ3result);

}

TEST(ExprTest, ReadExprFoldingBasic) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Basic constant folding rule
  UpdateList ul(array, 0);
  ref<Expr> read;

  for (unsigned i = 0; i < size; ++i) {
    // Constant index (i)
    read = ReadExpr::create(ul, ConstantExpr::create(i, Expr::Int32));
    EXPECT_EQ(Expr::Constant, read.get()->getKind());
    // Read - should be constant folded to Contents[i]
    // Check that constant folding worked
    ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
    EXPECT_EQ(Contents[i]->getZExtValue(), c->getZExtValue());
  }
}

TEST(ExprTest, ReadExprFoldingIndexOutOfBound) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with index-out-of-bound
  // Constant index (128)
  ref<Expr> index = ConstantExpr::create(128, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should not be constant folded
  // Check that constant folding was not applied
  EXPECT_EQ(Expr::Read, read.get()->getKind());
}

TEST(ExprTest, ReadExprFoldingConstantUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with constant update
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> updateValue = ConstantExpr::create(32, Expr::Int8);
  ul.extend(index, updateValue);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to 32
  // Check that constant folding worked
  EXPECT_EQ(Expr::Constant, read.get()->getKind());
  ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
  EXPECT_EQ(UINT64_C(32), c->getZExtValue());
}

TEST(ExprTest, ReadExprFoldingConstantMultipleUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with constant update
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  ref<Expr> updateValue = ConstantExpr::create(32, Expr::Int8);
  ref<Expr> updateValue2 = ConstantExpr::create(64, Expr::Int8);
  ul.extend(index, updateValue);
  ul.extend(index, updateValue2);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to 64
  // Check that constant folding worked
  EXPECT_EQ(Expr::Constant, read.get()->getKind());
  ConstantExpr *c = static_cast<ConstantExpr *>(read.get());
  EXPECT_EQ(UINT64_C(64), c->getZExtValue());
}

TEST(ExprTest, ReadExprFoldingSymbolicValueUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with symbolic update (value)
  // Constant index (0)
  ref<Expr> index = ConstantExpr::create(0, Expr::Int32);
  UpdateList ul(array, 0);
  const Array *array2 = ac.CreateArray("arr2", 256);
  ref<Expr> updateValue = ReadExpr::createTempRead(array2, Expr::Int8);
  ul.extend(index, updateValue);
  ref<Expr> read = ReadExpr::create(ul, index);
  // Read - should be constant folded to the symbolic value
  // Check that constant folding worked
  EXPECT_EQ(Expr::Read, read.get()->getKind());
  EXPECT_EQ(updateValue, read);
}

TEST(ExprTest, ReadExprFoldingSymbolicIndexUpdate) {
  unsigned size = 5;

  // Constant array
  std::vector<ref<ConstantExpr> > Contents(size);
  for (unsigned i = 0; i < size; ++i)
    Contents[i] = ConstantExpr::create(i + 1, Expr::Int8);
  ArrayCache ac;
  const Array *array =
      ac.CreateArray("arr", size, &Contents[0], &Contents[0] + size);

  // Constant folding rule with symbolic update (index)
  UpdateList ul(array, 0);
  const Array *array2 = ac.CreateArray("arr2", 256);
  ref<Expr> updateIndex = ReadExpr::createTempRead(array2, Expr::Int32);
  ref<Expr> updateValue = ConstantExpr::create(12, Expr::Int8);
  ul.extend(updateIndex, updateValue);
  ref<Expr> read;

  for (unsigned i = 0; i < size; ++i) {
    // Constant index (i)
    read = ReadExpr::create(ul, ConstantExpr::create(i, Expr::Int32));
    // Read - should not be constant folded
    // Check that constant folding was not applied
    EXPECT_EQ(Expr::Read, read.get()->getKind());
  }
}
}
