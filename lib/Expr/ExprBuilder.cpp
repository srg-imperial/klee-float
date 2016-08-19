//===-- ExprBuilder.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/ExprBuilder.h"

using namespace klee;

ExprBuilder::ExprBuilder() {
}

ExprBuilder::~ExprBuilder() {
}

namespace {
  class DefaultExprBuilder : public ExprBuilder {
    virtual ref<Expr> Constant(const llvm::APInt &Value) {
      return ConstantExpr::alloc(Value);
    }

    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return NotOptimizedExpr::alloc(Index);
    }

    virtual ref<Expr> Read(const UpdateList &Updates,
                           const ref<Expr> &Index) {
      return ReadExpr::alloc(Updates, Index);
    }

    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SelectExpr::alloc(Cond, LHS, RHS);
    }

    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return ConcatExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Extract(const ref<Expr> &LHS,
                              unsigned Offset, Expr::Width W) {
      return ExtractExpr::alloc(LHS, Offset, W);
    }

    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      return ZExtExpr::alloc(LHS, W);
    }

    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      return SExtExpr::alloc(LHS, W);
    }

    virtual ref<Expr> FToU(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return FToUExpr::alloc(LHS, W, RM);
    }

    virtual ref<Expr> FToS(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return FToSExpr::alloc(LHS, W, RM);
    }

    virtual ref<Expr> Not(const ref<Expr> &LHS) {
      return NotExpr::alloc(LHS);
    }

    virtual ref<Expr> FpClassify(const ref<Expr> &LHS) {
      return FpClassifyExpr::alloc(LHS);
    }

    virtual ref<Expr> FIsFinite(const ref<Expr> &LHS) {
      return FIsFiniteExpr::alloc(LHS);
    }

    virtual ref<Expr> FIsNan(const ref<Expr> &LHS) {
      return FIsNanExpr::alloc(LHS);
    }

    virtual ref<Expr> FIsInf(const ref<Expr> &LHS) {
      return FIsInfExpr::alloc(LHS);
    }

    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AddExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SubExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return MulExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UDivExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SDivExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return URemExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SRemExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AndExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return OrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return XorExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return ShlExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return LShrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return AShrExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return EqExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return NeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return UgeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return SgeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOrd(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOrdExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUno(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUnoExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUeqExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOeqExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOgtExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUgeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOgeExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOltExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOleExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FUne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FUneExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FOne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FOneExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> ExplicitFloat(const ref<Expr> &LHS, Expr::Width W) {
      return ExplicitFloatExpr::alloc(LHS, W);
    }

    virtual ref<Expr> ExplicitInt(const ref<Expr> &LHS, Expr::Width W) {
      return ExplicitIntExpr::alloc(LHS, W);
    }

    virtual ref<Expr> FSelect(const ref<Expr> &Cond, const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FSelectExpr::alloc(Cond, LHS, RHS);
    }

    virtual ref<Expr> FExt(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return FExtExpr::alloc(LHS, W, RM);
    }

    virtual ref<Expr> UToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return UToFExpr::alloc(LHS, W, RM);
    }

    virtual ref<Expr> SToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return SToFExpr::alloc(LHS, W, RM);
    }

    virtual ref<Expr> FAbs(const ref<Expr> &LHS) {
      return FAbsExpr::alloc(LHS);
    }

    virtual ref<Expr> FSqrt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      return FSqrtExpr::alloc(LHS, RM);
    }

    virtual ref<Expr> FNearbyInt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      return FNearbyIntExpr::alloc(LHS, RM);
    }

    virtual ref<Expr> FAdd(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return FAddExpr::alloc(LHS, RHS, RM);
    }

    virtual ref<Expr> FSub(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return FSubExpr::alloc(LHS, RHS, RM);
    }

    virtual ref<Expr> FMul(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return FMulExpr::alloc(LHS, RHS, RM);
    }

    virtual ref<Expr> FDiv(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return FDivExpr::alloc(LHS, RHS, RM);
    }

    virtual ref<Expr> FRem(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return FRemExpr::alloc(LHS, RHS, RM);
    }

    virtual ref<Expr> FMin(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FMinExpr::alloc(LHS, RHS);
    }

    virtual ref<Expr> FMax(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return FMaxExpr::alloc(LHS, RHS);
    }
  };

  /// ChainedBuilder - Helper class for construct specialized expression
  /// builders, which implements (non-virtual) methods which forward to a base
  /// expression builder, for all expressions.
  class ChainedBuilder {
  protected:
    /// Builder - The builder that this specialized builder is contained
    /// within. Provided for convenience to clients.
    ExprBuilder *Builder;

    /// Base - The base builder class for constructing expressions.
    ExprBuilder *Base;

  public:
    ChainedBuilder(ExprBuilder *_Builder, ExprBuilder *_Base) 
      : Builder(_Builder), Base(_Base) {}
    ~ChainedBuilder() { delete Base; }

    ref<Expr> Constant(const llvm::APInt &Value) {
      return Base->Constant(Value);
    }

    ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return Base->NotOptimized(Index);
    }

    ref<Expr> Read(const UpdateList &Updates,
                   const ref<Expr> &Index) {
      return Base->Read(Updates, Index);
    }

    ref<Expr> Select(const ref<Expr> &Cond,
                     const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Select(Cond, LHS, RHS);
    }

    ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Concat(LHS, RHS);
    }

    ref<Expr> Extract(const ref<Expr> &LHS,
                      unsigned Offset, Expr::Width W) {
      return Base->Extract(LHS, Offset, W);
    }

    ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      return Base->ZExt(LHS, W);
    }

    ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      return Base->SExt(LHS, W);
    }

    ref<Expr> FToU(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return Base->FToU(LHS, W, RM);
    }

    ref<Expr> FToS(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return Base->FToS(LHS, W, RM);
    }

    ref<Expr> Not(const ref<Expr> &LHS) {
      return Base->Not(LHS);
    }

    virtual ref<Expr> FpClassify(const ref<Expr> &LHS) {
      return Base->FpClassify(LHS);
    }

    virtual ref<Expr> FIsFinite(const ref<Expr> &LHS) {
      return Base->FIsFinite(LHS);
    }

    virtual ref<Expr> FIsNan(const ref<Expr> &LHS) {
      return Base->FIsNan(LHS);
    }

    virtual ref<Expr> FIsInf(const ref<Expr> &LHS) {
      return Base->FIsInf(LHS);
    }

    ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Add(LHS, RHS);
    }

    ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sub(LHS, RHS);
    }

    ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Mul(LHS, RHS);
    }

    ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->UDiv(LHS, RHS);
    }

    ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->SDiv(LHS, RHS);
    }

    ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->URem(LHS, RHS);
    }

    ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->SRem(LHS, RHS);
    }

    ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->And(LHS, RHS);
    }

    ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Or(LHS, RHS);
    }

    ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Xor(LHS, RHS);
    }

    ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Shl(LHS, RHS);
    }

    ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->LShr(LHS, RHS);
    }

    ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->AShr(LHS, RHS);
    }

    ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Eq(LHS, RHS);
    }

    ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ne(LHS, RHS);
    }

    ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ult(LHS, RHS);
    }

    ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ule(LHS, RHS);
    }

    ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Ugt(LHS, RHS);
    }

    ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Uge(LHS, RHS);
    }

    ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Slt(LHS, RHS);
    }

    ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sle(LHS, RHS);
    }

    ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sgt(LHS, RHS);
    }

    ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->Sge(LHS, RHS);
    }

    ref<Expr> FOrd(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOrd(LHS, RHS);
    }

    ref<Expr> FUno(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUno(LHS, RHS);
    }

    ref<Expr> FUeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUeq(LHS, RHS);
    }

    ref<Expr> FOeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOeq(LHS, RHS);
    }

    ref<Expr> FUgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUgt(LHS, RHS);
    }

    ref<Expr> FOgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOgt(LHS, RHS);
    }

    ref<Expr> FUge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUge(LHS, RHS);
    }

    ref<Expr> FOge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOge(LHS, RHS);
    }

    ref<Expr> FUlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUlt(LHS, RHS);
    }

    ref<Expr> FOlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOlt(LHS, RHS);
    }

    ref<Expr> FUle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUle(LHS, RHS);
    }

    ref<Expr> FOle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOle(LHS, RHS);
    }

    ref<Expr> FUne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FUne(LHS, RHS);
    }

    ref<Expr> FOne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FOne(LHS, RHS);
    }

    ref<Expr> ExplicitFloat(const ref<Expr> &LHS, Expr::Width W) {
      return Base->ExplicitFloat(LHS, W);
    }

    ref<Expr> ExplicitInt(const ref<Expr> &LHS, Expr::Width W) {
      return Base->ExplicitInt(LHS, W);
    }

    ref<Expr> FSelect(const ref<Expr> &Cond, const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FSelect(Cond, LHS, RHS);
    }

    ref<Expr> FExt(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return Base->FExt(LHS, W, RM);
    }

    ref<Expr> UToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return Base->UToF(LHS, W, RM);
    }

    ref<Expr> SToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      return Base->SToF(LHS, W, RM);
    }

    virtual ref<Expr> FAbs(const ref<Expr> &LHS) {
      return Base->FAbs(LHS);
    }

    virtual ref<Expr> FSqrt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      return Base->FSqrt(LHS, RM);
    }

    virtual ref<Expr> FNearbyInt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      return Base->FNearbyInt(LHS, RM);
    }

    ref<Expr> FAdd(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return Base->FAdd(LHS, RHS, RM);
    }

    ref<Expr> FSub(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return Base->FSub(LHS, RHS, RM);
    }

    ref<Expr> FMul(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return Base->FMul(LHS, RHS, RM);
    }

    ref<Expr> FDiv(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return Base->FDiv(LHS, RHS, RM);
    }

    ref<Expr> FRem(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      return Base->FRem(LHS, RHS, RM);
    }

    virtual ref<Expr> FMin(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FMin(LHS, RHS);
    }

    virtual ref<Expr> FMax(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      return Base->FMax(LHS, RHS);
    }
  };

  /// ConstantSpecializedExprBuilder - A base expression builder class which
  /// handles dispatching to a helper class, based on whether the arguments are
  /// constant or not.
  ///
  /// The SpecializedBuilder template argument should be a helper class which
  /// implements methods for all the expression construction functions. These
  /// methods can be specialized to take [Non]ConstantExpr when desired.
  template<typename SpecializedBuilder>
  class ConstantSpecializedExprBuilder : public ExprBuilder {
    SpecializedBuilder Builder;

  public:
    ConstantSpecializedExprBuilder(ExprBuilder *Base) : Builder(this, Base) {}
    ~ConstantSpecializedExprBuilder() {}

    virtual ref<Expr> Constant(const llvm::APInt &Value) {
      return Builder.Constant(Value);
    }

    virtual ref<Expr> NotOptimized(const ref<Expr> &Index) {
      return Builder.NotOptimized(Index);
    }

    virtual ref<Expr> Read(const UpdateList &Updates,
                           const ref<Expr> &Index) {
      // Roll back through writes when possible.
      const UpdateNode *UN = Updates.head;
      while (UN && Eq(Index, UN->index)->isFalse())
        UN = UN->next;

      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Index))
        return Builder.Read(UpdateList(Updates.root, UN), CE);

      return Builder.Read(UpdateList(Updates.root, UN), Index);
    }

    virtual ref<Expr> Select(const ref<Expr> &Cond,
                             const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Cond))
        return CE->isTrue() ? LHS : RHS;

      return Builder.Select(cast<NonConstantExpr>(Cond), LHS, RHS);
    }

    virtual ref<Expr> Concat(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Concat(RCE);
        return Builder.Concat(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Concat(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Concat(cast<NonConstantExpr>(LHS),
                            cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Extract(const ref<Expr> &LHS,
                              unsigned Offset, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->Extract(Offset, W);

      return Builder.Extract(cast<NonConstantExpr>(LHS), Offset, W);
    }

    virtual ref<Expr> ZExt(const ref<Expr> &LHS, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->ZExt(W);

      return Builder.ZExt(cast<NonConstantExpr>(LHS), W);
    }

    virtual ref<Expr> SExt(const ref<Expr> &LHS, Expr::Width W) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->SExt(W);

      return Builder.SExt(cast<NonConstantExpr>(LHS), W);
    }

    virtual ref<Expr> FToU(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FToU(W, RM);

      return Builder.FToU(cast<NonConstantExpr>(LHS), W, RM);
    }

    virtual ref<Expr> FToS(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FToS(W, RM);

      return Builder.FToS(cast<NonConstantExpr>(LHS), W, RM);
    }

    virtual ref<Expr> Not(const ref<Expr> &LHS) {
      // !!X ==> X
      if (NotExpr *DblNot = dyn_cast<NotExpr>(LHS))
        return DblNot->getKid(0);

      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->Not();

      return Builder.Not(cast<NonConstantExpr>(LHS));
    }

    virtual ref<Expr> FpClassify(const ref<Expr> &LHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FpClassify();

      return Builder.FpClassify(cast<NonConstantExpr>(LHS));
    }

    virtual ref<Expr> FIsFinite(const ref<Expr> &LHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FIsFinite();

      return Builder.FIsFinite(cast<NonConstantExpr>(LHS));
    }

    virtual ref<Expr> FIsNan(const ref<Expr> &LHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FIsNan();

      return Builder.FIsNan(cast<NonConstantExpr>(LHS));
    }

    virtual ref<Expr> FIsInf(const ref<Expr> &LHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FIsInf();

      return Builder.FIsInf(cast<NonConstantExpr>(LHS));
    }

    virtual ref<Expr> Add(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Add(RCE);
        return Builder.Add(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Add(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Add(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Sub(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sub(RCE);
        return Builder.Sub(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Sub(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Sub(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Mul(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Mul(RCE);
        return Builder.Mul(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Mul(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Mul(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> UDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->UDiv(RCE);
        return Builder.UDiv(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.UDiv(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.UDiv(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> SDiv(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->SDiv(RCE);
        return Builder.SDiv(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.SDiv(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.SDiv(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> URem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->URem(RCE);
        return Builder.URem(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.URem(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.URem(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> SRem(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->SRem(RCE);
        return Builder.SRem(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.SRem(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.SRem(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> And(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->And(RCE);
        return Builder.And(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.And(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.And(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Or(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Or(RCE);
        return Builder.Or(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Or(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Or(cast<NonConstantExpr>(LHS),
                        cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Xor(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Xor(RCE);
        return Builder.Xor(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Xor(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Xor(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Shl(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Shl(RCE);
        return Builder.Shl(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Shl(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Shl(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> LShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->LShr(RCE);
        return Builder.LShr(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.LShr(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.LShr(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> AShr(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->AShr(RCE);
        return Builder.AShr(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.AShr(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.AShr(cast<NonConstantExpr>(LHS),
                          cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Eq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Eq(RCE);
        return Builder.Eq(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Eq(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Eq(cast<NonConstantExpr>(LHS),
                        cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ne(RCE);
        return Builder.Ne(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Ne(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Ne(cast<NonConstantExpr>(LHS),
                        cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Ult(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ult(RCE);
        return Builder.Ult(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Ult(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Ult(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Ule(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ule(RCE);
        return Builder.Ule(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Ule(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Ule(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Ugt(RCE);
        return Builder.Ugt(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Ugt(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Ugt(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Uge(RCE);
        return Builder.Uge(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Uge(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Uge(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Slt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Slt(RCE);
        return Builder.Slt(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Slt(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Slt(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Sle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sle(RCE);
        return Builder.Sle(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Sle(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Sle(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sgt(RCE);
        return Builder.Sgt(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Sgt(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Sgt(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (ConstantExpr *LCE = dyn_cast<ConstantExpr>(LHS)) {
        if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS))
          return LCE->Sge(RCE);
        return Builder.Sge(LCE, cast<NonConstantExpr>(RHS));
      } else if (ConstantExpr *RCE = dyn_cast<ConstantExpr>(RHS)) {
        return Builder.Sge(cast<NonConstantExpr>(LHS), RCE);
      }

      return Builder.Sge(cast<NonConstantExpr>(LHS),
                         cast<NonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOrd(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOrd(RCE);
        return Builder.FOrd(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOrd(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOrd(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUno(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUno(RCE);
        return Builder.FUno(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUno(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUno(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUeq(RCE);
        return Builder.FUeq(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUeq(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUeq(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOeq(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOeq(RCE);
        return Builder.FOeq(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOeq(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOeq(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUgt(RCE);
        return Builder.FUgt(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUgt(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUgt(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOgt(RCE);
        return Builder.FOgt(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOgt(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOgt(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUge(RCE);
        return Builder.FUge(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUge(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUge(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOge(RCE);
        return Builder.FOge(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOge(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOge(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUlt(RCE);
        return Builder.FUlt(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUlt(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUlt(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOlt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOlt(RCE);
        return Builder.FOlt(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOlt(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOlt(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUle(RCE);
        return Builder.FUle(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUle(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUle(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOle(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOle(RCE);
        return Builder.FOle(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOle(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOle(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FUne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FUne(RCE);
        return Builder.FUne(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FUne(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FUne(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FOne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FOne(RCE);
        return Builder.FOne(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FOne(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FOne(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> ExplicitFloat(const ref<Expr> &LHS, Expr::Width W) {
      if (ConstantExpr *ce = dyn_cast<ConstantExpr>(LHS))
      {
        return ce->ExplicitFloat(W);
      }
      if (SelectExpr *se = dyn_cast<SelectExpr>(LHS))
      {
        ref<Expr> t = ExplicitFloatExpr::create(se->trueExpr, W);
        ref<Expr> f = ExplicitFloatExpr::create(se->falseExpr, W);

        return FSelectExpr::create(se->cond, t, f);
      }
      return Builder.ExplicitFloat(cast<NonConstantExpr>(LHS), W);
    }

    virtual ref<Expr> ExplicitInt(const ref<Expr> &LHS, Expr::Width W) {
      if (FConstantExpr *ce = dyn_cast<FConstantExpr>(LHS))
      {
        return ce->ExplicitInt(W);
      }
      if (SelectExpr *se = dyn_cast<SelectExpr>(LHS))
      {
        ref<Expr> t = ExplicitIntExpr::create(se->trueExpr, W);
        ref<Expr> f = ExplicitIntExpr::create(se->falseExpr, W);

        return FSelectExpr::create(se->cond, t, f);
      }
      return Builder.ExplicitInt(cast<FNonConstantExpr>(LHS), W);
    }

    virtual ref<Expr> FSelect(const ref<Expr> &Cond, const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(Cond))
        return CE->isTrue() ? LHS : RHS;

      return Builder.Select(cast<FNonConstantExpr>(Cond), LHS, RHS);
    }

    virtual ref<Expr> FExt(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FExt(W, RM);

      return Builder.FExt(cast<FNonConstantExpr>(LHS), W, RM);
    }

    virtual ref<Expr> UToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->UToF(W, RM);

      return Builder.UToF(cast<NonConstantExpr>(LHS), W, RM);
    }

    virtual ref<Expr> SToF(const ref<Expr> &LHS, Expr::Width W, llvm::APFloat::roundingMode RM) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(LHS))
        return CE->SToF(W, RM);

      return Builder.SToF(cast<NonConstantExpr>(LHS), W, RM);
    }

    virtual ref<Expr> FAbs(const ref<Expr> &LHS) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FAbs();

      return Builder.FAbs(cast<FNonConstantExpr>(LHS));
    }

    virtual ref<Expr> FSqrt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FSqrt(RM);

      return Builder.FSqrt(cast<FNonConstantExpr>(LHS), RM);
    }

    virtual ref<Expr> FNearbyInt(const ref<Expr> &LHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *CE = dyn_cast<FConstantExpr>(LHS))
        return CE->FNearbyInt(RM);

      return Builder.FNearbyInt(cast<FNonConstantExpr>(LHS), RM);
    }

    virtual ref<Expr> FAdd(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FAdd(RCE, RM);
        return Builder.FAdd(LCE, cast<FNonConstantExpr>(RHS), RM);
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FAdd(cast<FNonConstantExpr>(LHS), RCE, RM);
      }

      return Builder.FAdd(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS), RM);
    }

    virtual ref<Expr> FSub(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FSub(RCE, RM);
        return Builder.FSub(LCE, cast<FNonConstantExpr>(RHS), RM);
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FSub(cast<FNonConstantExpr>(LHS), RCE, RM);
      }

      return Builder.FSub(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS), RM);
    }

    virtual ref<Expr> FMul(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FMul(RCE, RM);
        return Builder.FMul(LCE, cast<FNonConstantExpr>(RHS), RM);
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FMul(cast<FNonConstantExpr>(LHS), RCE, RM);
      }

      return Builder.FMul(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS), RM);
    }

    virtual ref<Expr> FDiv(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FDiv(RCE, RM);
        return Builder.FDiv(LCE, cast<FNonConstantExpr>(RHS), RM);
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FDiv(cast<FNonConstantExpr>(LHS), RCE, RM);
      }

      return Builder.FDiv(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS), RM);
    }

    virtual ref<Expr> FRem(const ref<Expr> &LHS, const ref<Expr> &RHS, llvm::APFloat::roundingMode RM) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FRem(RCE, RM);
        return Builder.FRem(LCE, cast<FNonConstantExpr>(RHS), RM);
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FRem(cast<FNonConstantExpr>(LHS), RCE, RM);
      }

      return Builder.FRem(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS), RM);
    }

    virtual ref<Expr> FMin(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FMin(RCE);
        return Builder.FMin(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FMin(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FMin(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }

    virtual ref<Expr> FMax(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      if (FConstantExpr *LCE = dyn_cast<FConstantExpr>(LHS)) {
        if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS))
          return LCE->FMax(RCE);
        return Builder.FMax(LCE, cast<FNonConstantExpr>(RHS));
      }
      else if (FConstantExpr *RCE = dyn_cast<FConstantExpr>(RHS)) {
        return Builder.FMax(cast<FNonConstantExpr>(LHS), RCE);
      }

      return Builder.FMax(cast<FNonConstantExpr>(LHS),
                          cast<FNonConstantExpr>(RHS));
    }
  };

  class ConstantFoldingBuilder :
    public ChainedBuilder {
  public:
    ConstantFoldingBuilder(ExprBuilder *Builder, ExprBuilder *Base)
      : ChainedBuilder(Builder, Base) {}

    ref<Expr> Add(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      // 0 + X ==> X
      if (LHS->isZero())
        return RHS;

      switch (RHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // C_0 + (C_1 + X) ==> (C_0 + C1) + X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(LHS->Add(CE), BE->right);
        // C_0 + (X + C_1) ==> (C_0 + C1) + X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(LHS->Add(CE), BE->left);
        break;
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // C_0 + (C_1 - X) ==> (C_0 + C1) - X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Sub(LHS->Add(CE), BE->right);
        // C_0 + (X - C_1) ==> (C_0 - C1) + X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(LHS->Sub(CE), BE->left);
        break;
      }
      }

      return Base->Add(LHS, RHS);
    }

    ref<Expr> Add(const ref<NonConstantExpr> &LHS,
                  const ref<ConstantExpr> &RHS) {
      return Add(RHS, LHS);
    }

    ref<Expr> Add(const ref<NonConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      switch (LHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(LHS);
        // (X + Y) + Z ==> X + (Y + Z)
        return Builder->Add(BE->left,
                            Builder->Add(BE->right, RHS));
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(LHS);
        // (X - Y) + Z ==> X + (Z - Y)
        return Builder->Add(BE->left,
                            Builder->Sub(RHS, BE->right));
      }
      }

      switch (RHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // X + (C_0 + Y) ==> C_0 + (X + Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(CE, Builder->Add(LHS, BE->right));
        // X + (Y + C_0) ==> C_0 + (X + Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(CE, Builder->Add(LHS, BE->left));
        break;
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // X + (C_0 - Y) ==> C_0 + (X - Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(CE, Builder->Sub(LHS, BE->right));
        // X + (Y - C_0) ==> -C_0 + (X + Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(CE->Neg(), Builder->Add(LHS, BE->left));
        break;
      }
      }

      return Base->Add(LHS, RHS);
    }

    ref<Expr> Sub(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      switch (RHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // C_0 - (C_1 + X) ==> (C_0 - C1) - X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Sub(LHS->Sub(CE), BE->right);
        // C_0 - (X + C_1) ==> (C_0 + C1) + X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Sub(LHS->Sub(CE), BE->left);
        break;
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // C_0 - (C_1 - X) ==> (C_0 - C1) + X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(LHS->Sub(CE), BE->right);
        // C_0 - (X - C_1) ==> (C_0 + C1) - X
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Sub(LHS->Add(CE), BE->left);
        break;
      }
      }

      return Base->Sub(LHS, RHS);
    }

    ref<Expr> Sub(const ref<NonConstantExpr> &LHS,
                  const ref<ConstantExpr> &RHS) {
        // X - C_0 ==> -C_0 + X
      return Add(RHS->Neg(), LHS);
    }

    ref<Expr> Sub(const ref<NonConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      switch (LHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(LHS);
        // (X + Y) - Z ==> X + (Y - Z)
        return Builder->Add(BE->left, Builder->Sub(BE->right, RHS));
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(LHS);
        // (X - Y) - Z ==> X - (Y + Z)
        return Builder->Sub(BE->left, Builder->Add(BE->right, RHS));
      }
      }

      switch (RHS->getKind()) {
      default: break;

      case Expr::Add: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // X - (C + Y) ==> -C + (X - Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(CE->Neg(), Builder->Sub(LHS, BE->right));
        // X - (Y + C) ==> -C + (X + Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(CE->Neg(), Builder->Sub(LHS, BE->left));
        break;
      }

      case Expr::Sub: {
        BinaryExpr *BE = cast<BinaryExpr>(RHS);
        // X - (C - Y) ==> -C + (X + Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->left))
          return Builder->Add(CE->Neg(), Builder->Add(LHS, BE->right));
        // X - (Y - C) ==> C + (X - Y)
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(BE->right))
          return Builder->Add(CE, Builder->Sub(LHS, BE->left));
        break;
      }
      }

      return Base->Sub(LHS, RHS);
    }

    ref<Expr> Mul(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      if (LHS->isZero())
        return LHS;
      if (LHS->isOne())
        return RHS;
      // FIXME: Unbalance nested muls, fold constants through
      // {sub,add}-with-constant, etc.
      return Base->Mul(LHS, RHS);
    }

    ref<Expr> Mul(const ref<NonConstantExpr> &LHS,
                  const ref<ConstantExpr> &RHS) {
      return Mul(RHS, LHS);
    }

    ref<Expr> Mul(const ref<NonConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      return Base->Mul(LHS, RHS);
    }

    ref<Expr> And(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      if (LHS->isZero())
        return LHS;
      if (LHS->isAllOnes())
        return RHS;
      // FIXME: Unbalance nested ands, fold constants through
      // {and,or}-with-constant, etc.
      return Base->And(LHS, RHS);
    }

    ref<Expr> And(const ref<NonConstantExpr> &LHS,
                  const ref<ConstantExpr> &RHS) {
      return And(RHS, LHS);
    }

    ref<Expr> And(const ref<NonConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      return Base->And(LHS, RHS);
    }

    ref<Expr> Or(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      if (LHS->isZero())
        return RHS;
      if (LHS->isAllOnes())
        return LHS;
      // FIXME: Unbalance nested ors, fold constants through
      // {and,or}-with-constant, etc.
      return Base->Or(LHS, RHS);
    }

    ref<Expr> Or(const ref<NonConstantExpr> &LHS,
                 const ref<ConstantExpr> &RHS) {
      return Or(RHS, LHS);
    }

    ref<Expr> Or(const ref<NonConstantExpr> &LHS,
                 const ref<NonConstantExpr> &RHS) {
      return Base->Or(LHS, RHS);
    }

    ref<Expr> Xor(const ref<ConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      if (LHS->isZero())
        return RHS;
      // FIXME: Unbalance nested ors, fold constants through
      // {and,or}-with-constant, etc.
      return Base->Xor(LHS, RHS);
    }

    ref<Expr> Xor(const ref<NonConstantExpr> &LHS,
                  const ref<ConstantExpr> &RHS) {
      return Xor(RHS, LHS);
    }

    ref<Expr> Xor(const ref<NonConstantExpr> &LHS,
                  const ref<NonConstantExpr> &RHS) {
      return Base->Xor(LHS, RHS);
    }

    ref<Expr> Eq(const ref<ConstantExpr> &LHS, 
                 const ref<NonConstantExpr> &RHS) {
      Expr::Width Width = LHS->getWidth();
      
      if (Width == Expr::Bool) {
        // true == X ==> X
        if (LHS->isTrue())
          return RHS;

        // false == ... (not)
	return Base->Not(RHS);
      }

      return Base->Eq(LHS, RHS);
    }

    ref<Expr> Eq(const ref<NonConstantExpr> &LHS, 
                 const ref<ConstantExpr> &RHS) {
      return Eq(RHS, LHS);
    }

    ref<Expr> Eq(const ref<NonConstantExpr> &LHS, 
                 const ref<NonConstantExpr> &RHS) {
      return Base->Eq(LHS, RHS);
    }
  };

  typedef ConstantSpecializedExprBuilder<ConstantFoldingBuilder>
    ConstantFoldingExprBuilder;

  class SimplifyingBuilder : public ChainedBuilder {
  public:
    SimplifyingBuilder(ExprBuilder *Builder, ExprBuilder *Base)
      : ChainedBuilder(Builder, Base) {}

    ref<Expr> Eq(const ref<ConstantExpr> &LHS, 
                 const ref<NonConstantExpr> &RHS) {
      Expr::Width Width = LHS->getWidth();
      
      if (Width == Expr::Bool) {
        // true == X ==> X
        if (LHS->isTrue())
          return RHS;

        // false == X (not)
	return Base->Not(RHS);
      }

      return Base->Eq(LHS, RHS);
    }

    ref<Expr> Eq(const ref<NonConstantExpr> &LHS, 
                 const ref<ConstantExpr> &RHS) {
      return Eq(RHS, LHS);
    }

    ref<Expr> Eq(const ref<NonConstantExpr> &LHS, 
                 const ref<NonConstantExpr> &RHS) {
      // X == X ==> true
      if (LHS == RHS)
          return Builder->True();

      return Base->Eq(LHS, RHS);
    }

    ref<Expr> Not(const ref<NonConstantExpr> &LHS) {
      // Transform !(a or b) ==> !a and !b.
      if (const OrExpr *OE = dyn_cast<OrExpr>(LHS))
	return Builder->And(Builder->Not(OE->left),
			    Builder->Not(OE->right));
      return Base->Not(LHS);
    }

    ref<Expr> Ne(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      // X != Y ==> !(X == Y)
      return Builder->Not(Builder->Eq(LHS, RHS));
    }

    ref<Expr> Ugt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      // X u> Y ==> Y u< X
      return Builder->Ult(RHS, LHS);
    }

    ref<Expr> Uge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      // X u>= Y ==> Y u<= X
      return Builder->Ule(RHS, LHS);
    }

    ref<Expr> Sgt(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      // X s> Y ==> Y s< X
      return Builder->Slt(RHS, LHS);
    }

    ref<Expr> Sge(const ref<Expr> &LHS, const ref<Expr> &RHS) {
      // X s>= Y ==> Y s<= X
      return Builder->Sle(RHS, LHS);
    }
  };

  typedef ConstantSpecializedExprBuilder<SimplifyingBuilder>
    SimplifyingExprBuilder;
}

ExprBuilder *klee::createDefaultExprBuilder() {
  return new DefaultExprBuilder();
}

ExprBuilder *klee::createConstantFoldingExprBuilder(ExprBuilder *Base) {
  return new ConstantFoldingExprBuilder(Base);
}

ExprBuilder *klee::createSimplifyingExprBuilder(ExprBuilder *Base) {
  return new SimplifyingExprBuilder(Base);
}
