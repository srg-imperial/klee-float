//===-- ExprVisitor.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr.h"
#include "klee/util/ExprVisitor.h"

#include "llvm/Support/CommandLine.h"

namespace {
  llvm::cl::opt<bool>
  UseVisitorHash("use-visitor-hash", 
                 llvm::cl::desc("Use hash-consing during expr visitation."),
                 llvm::cl::init(true));
}

using namespace klee;

ref<Expr> ExprVisitor::visit(const ref<Expr> &e) {
  if (!UseVisitorHash || isa<ConstantExpr>(e)) {
    return visitActual(e);
  } else {
    visited_ty::iterator it = visited.find(e);

    if (it!=visited.end()) {
      return it->second;
    } else {
      ref<Expr> res = visitActual(e);
      visited.insert(std::make_pair(e, res));
      return res;
    }
  }
}

ref<Expr> ExprVisitor::visitActual(const ref<Expr> &e) {
  if (isa<ConstantExpr>(e)) {    
    return e;
  } else {
    Expr &ep = *e.get();

    Action res = visitExpr(ep);
    switch(res.kind) {
    case Action::DoChildren:
      // continue with normal action
      break;
    case Action::SkipChildren:
      return e;
    case Action::ChangeTo:
      return res.argument;
    }

    switch(ep.getKind()) {
    case Expr::NotOptimized: res = visitNotOptimized(static_cast<NotOptimizedExpr&>(ep)); break;
    case Expr::Read: res = visitRead(static_cast<ReadExpr&>(ep)); break;
    case Expr::Select: res = visitSelect(static_cast<SelectExpr&>(ep)); break;
    case Expr::Concat: res = visitConcat(static_cast<ConcatExpr&>(ep)); break;
    case Expr::Extract: res = visitExtract(static_cast<ExtractExpr&>(ep)); break;
    case Expr::ZExt: res = visitZExt(static_cast<ZExtExpr&>(ep)); break;
    case Expr::SExt: res = visitSExt(static_cast<SExtExpr&>(ep)); break;
    case Expr::FExt: res = visitFExt(static_cast<FExtExpr&>(ep)); break;
    case Expr::FToU: res = visitFToU(static_cast<FToUExpr&>(ep)); break;
    case Expr::FToS: res = visitFToS(static_cast<FToSExpr&>(ep)); break;
    case Expr::UToF: res = visitUToF(static_cast<UToFExpr&>(ep)); break;
    case Expr::SToF: res = visitSToF(static_cast<SToFExpr&>(ep)); break;
    case Expr::Add: res = visitAdd(static_cast<AddExpr&>(ep)); break;
    case Expr::Sub: res = visitSub(static_cast<SubExpr&>(ep)); break;
    case Expr::Mul: res = visitMul(static_cast<MulExpr&>(ep)); break;
    case Expr::UDiv: res = visitUDiv(static_cast<UDivExpr&>(ep)); break;
    case Expr::SDiv: res = visitSDiv(static_cast<SDivExpr&>(ep)); break;
    case Expr::URem: res = visitURem(static_cast<URemExpr&>(ep)); break;
    case Expr::SRem: res = visitSRem(static_cast<SRemExpr&>(ep)); break;
    case Expr::Not: res = visitNot(static_cast<NotExpr&>(ep)); break;
    case Expr::And: res = visitAnd(static_cast<AndExpr&>(ep)); break;
    case Expr::Or: res = visitOr(static_cast<OrExpr&>(ep)); break;
    case Expr::Xor: res = visitXor(static_cast<XorExpr&>(ep)); break;
    case Expr::Shl: res = visitShl(static_cast<ShlExpr&>(ep)); break;
    case Expr::LShr: res = visitLShr(static_cast<LShrExpr&>(ep)); break;
    case Expr::AShr: res = visitAShr(static_cast<AShrExpr&>(ep)); break;
    case Expr::FAdd: res = visitFAdd(static_cast<FAddExpr&>(ep)); break;
    case Expr::FSub: res = visitFSub(static_cast<FSubExpr&>(ep)); break;
    case Expr::FMul: res = visitFMul(static_cast<FMulExpr&>(ep)); break;
    case Expr::FDiv: res = visitFDiv(static_cast<FDivExpr&>(ep)); break;
    case Expr::FRem: res = visitFRem(static_cast<FRemExpr&>(ep)); break;
    case Expr::Eq: res = visitEq(static_cast<EqExpr&>(ep)); break;
    case Expr::Ne: res = visitNe(static_cast<NeExpr&>(ep)); break;
    case Expr::Ult: res = visitUlt(static_cast<UltExpr&>(ep)); break;
    case Expr::Ule: res = visitUle(static_cast<UleExpr&>(ep)); break;
    case Expr::Ugt: res = visitUgt(static_cast<UgtExpr&>(ep)); break;
    case Expr::Uge: res = visitUge(static_cast<UgeExpr&>(ep)); break;
    case Expr::Slt: res = visitSlt(static_cast<SltExpr&>(ep)); break;
    case Expr::Sle: res = visitSle(static_cast<SleExpr&>(ep)); break;
    case Expr::Sgt: res = visitSgt(static_cast<SgtExpr&>(ep)); break;
    case Expr::Sge: res = visitSge(static_cast<SgeExpr&>(ep)); break;
    case Expr::FOrd: res = visitFOrd(static_cast<FOrdExpr&>(ep)); break;
    case Expr::FUno: res = visitFUno(static_cast<FUnoExpr&>(ep)); break;
    case Expr::FUeq: res = visitFUeq(static_cast<FUeqExpr&>(ep)); break;
    case Expr::FOeq: res = visitFOeq(static_cast<FOeqExpr&>(ep)); break;
    case Expr::FUgt: res = visitFUgt(static_cast<FUgtExpr&>(ep)); break;
    case Expr::FOgt: res = visitFOgt(static_cast<FOgtExpr&>(ep)); break;
    case Expr::FUge: res = visitFUge(static_cast<FUgeExpr&>(ep)); break;
    case Expr::FOge: res = visitFOge(static_cast<FOgeExpr&>(ep)); break;
    case Expr::FUlt: res = visitFUlt(static_cast<FUltExpr&>(ep)); break;
    case Expr::FOlt: res = visitFOlt(static_cast<FOltExpr&>(ep)); break;
    case Expr::FUle: res = visitFUle(static_cast<FUleExpr&>(ep)); break;
    case Expr::FOle: res = visitFOle(static_cast<FOleExpr&>(ep)); break;
    case Expr::FUne: res = visitFUne(static_cast<FUneExpr&>(ep)); break;
    case Expr::FOne: res = visitFOne(static_cast<FOneExpr&>(ep)); break;
    case Expr::Constant:
    default:
      assert(0 && "invalid expression kind");
    }

    switch(res.kind) {
    default:
      assert(0 && "invalid kind");
    case Action::DoChildren: {  
      bool rebuild = false;
      ref<Expr> e(&ep), kids[8];
      unsigned count = ep.getNumKids();
      for (unsigned i=0; i<count; i++) {
        ref<Expr> kid = ep.getKid(i);
        kids[i] = visit(kid);
        if (kids[i] != kid)
          rebuild = true;
      }
      if (rebuild) {
        e = ep.rebuild(kids);
        if (recursive)
          e = visit(e);
      }
      if (!isa<ConstantExpr>(e)) {
        res = visitExprPost(*e.get());
        if (res.kind==Action::ChangeTo)
          e = res.argument;
      }
      return e;
    }
    case Action::SkipChildren:
      return e;
    case Action::ChangeTo:
      return res.argument;
    }
  }
}

ExprVisitor::Action ExprVisitor::visitExpr(const Expr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitExprPost(const Expr&) {
  return Action::skipChildren();
}

ExprVisitor::Action ExprVisitor::visitNotOptimized(const NotOptimizedExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitRead(const ReadExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSelect(const SelectExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitConcat(const ConcatExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitExtract(const ExtractExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitZExt(const ZExtExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSExt(const SExtExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitFExt(const FExtExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFToU(const FToUExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFToS(const FToSExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitUToF(const UToFExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitSToF(const SToFExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitAdd(const AddExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSub(const SubExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitMul(const MulExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitUDiv(const UDivExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSDiv(const SDivExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitURem(const URemExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSRem(const SRemExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitNot(const NotExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitAnd(const AndExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitOr(const OrExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitXor(const XorExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitShl(const ShlExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitLShr(const LShrExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitAShr(const AShrExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitFAdd(const FAddExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFSub(const FSubExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFMul(const FMulExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFDiv(const FDivExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFRem(const FRemExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitEq(const EqExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitNe(const NeExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitUlt(const UltExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitUle(const UleExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitUgt(const UgtExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitUge(const UgeExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSlt(const SltExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSle(const SleExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSgt(const SgtExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitSge(const SgeExpr&) {
  return Action::doChildren(); 
}

ExprVisitor::Action ExprVisitor::visitFOrd(const FOrdExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUno(const FUnoExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUeq(const FUeqExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOeq(const FOeqExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUgt(const FUgtExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOgt(const FOgtExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUge(const FUgeExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOge(const FOgeExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUlt(const FUltExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOlt(const FOltExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUle(const FUleExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOle(const FOleExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFUne(const FUneExpr&) {
  return Action::doChildren();
}

ExprVisitor::Action ExprVisitor::visitFOne(const FOneExpr&) {
  return Action::doChildren();
}
