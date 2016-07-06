//===-- ExprBuilder.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRBUILDER_H
#define KLEE_EXPRBUILDER_H

#include "Expr.h"

namespace klee {
  /// ExprBuilder - Base expression builder class.
  class ExprBuilder {
  protected:
    ExprBuilder();

  public:
    virtual ~ExprBuilder();

    // Expressions

    virtual ref<Expr> Constant(const llvm::APInt &Value) = 0;
    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) = 0;
    virtual ref<Expr> Read(const UpdateList &Updates, 
                           const ref<Expr> &Index) = 0;
    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Extract(const ref<Expr> &LHS, 
                              unsigned Offset, Expr::Width W) = 0;
    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) = 0;
    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) = 0;
    virtual ref<Expr> FExt(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FToU(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FToS(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> UToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> SToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Not(const ref<Expr> &LHS) = 0;
    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FAdd(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FSub(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FMul(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FDiv(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> FRem(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) = 0;
    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOrd(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUno(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUeq(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOeq(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUgt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOgt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUge(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOge(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUlt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOlt(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUle(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOle(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FUne(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;
    virtual ref<Expr> FOne(const ref<Expr> &LHS, const ref<Expr> &RHS) = 0;

    // Utility functions

    ref<Expr> False() { return ConstantExpr::alloc(0, Expr::Bool); }

    ref<Expr> True() { return ConstantExpr::alloc(1, Expr::Bool); }

    ref<Expr> Constant(uint64_t Value, Expr::Width W) {
      return Constant(llvm::APInt(W, Value));
    }
  };

  /// createDefaultExprBuilder - Create an expression builder which does no
  /// folding.
  ExprBuilder *createDefaultExprBuilder();

  /// createConstantFoldingExprBuilder - Create an expression builder which
  /// folds constant expressions.
  ///
  /// Base - The base builder to use when constructing expressions.
  ExprBuilder *createConstantFoldingExprBuilder(ExprBuilder *Base);

  /// createSimplifyingExprBuilder - Create an expression builder which attemps
  /// to fold redundant expressions and normalize expressions for improved
  /// caching.
  ///
  /// Base - The base builder to use when constructing expressions.
  ExprBuilder *createSimplifyingExprBuilder(ExprBuilder *Base);
}

#endif
