//===-- ExecutorUtil.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Executor.h"

#include "Context.h"

#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Solver.h"

#include "klee/Config/Version.h"
#include "klee/Internal/Module/KModule.h"

#include "klee/util/GetElementPtrTypeIterator.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#else
#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#else
#include "llvm/DataLayout.h"
#endif
#endif

#include <cassert>

using namespace klee;
using namespace llvm;

namespace klee {

ref<ConstantExpr> Executor::evalConstantExpr(const llvm::ConstantExpr *ce,
                                             llvm::APFloat::roundingMode rm) {
  LLVM_TYPE_Q llvm::Type *type = ce->getType();

  ref<ConstantExpr> op1(0), op2(0), op3(0);
  int numOperands = ce->getNumOperands();

  if (numOperands > 0)
    op1 = evalConstant(ce->getOperand(0), rm);
  if (numOperands > 1)
    op2 = evalConstant(ce->getOperand(1), rm);
  if (numOperands > 2)
    op3 = evalConstant(ce->getOperand(2), rm);

  switch (ce->getOpcode()) {
  default:
    ce->dump();
    llvm::errs() << "error: unknown ConstantExpr type\n"
                 << "opcode: " << ce->getOpcode() << "\n";
    abort();

  case Instruction::Trunc:
    return op1->Extract(0, getWidthForLLVMType(type));
  case Instruction::ZExt:
    return op1->ZExt(getWidthForLLVMType(type));
  case Instruction::SExt:
    return op1->SExt(getWidthForLLVMType(type));
  case Instruction::Add:
    return op1->Add(op2);
  case Instruction::Sub:
    return op1->Sub(op2);
  case Instruction::Mul:
    return op1->Mul(op2);
  case Instruction::SDiv:
    return op1->SDiv(op2);
  case Instruction::UDiv:
    return op1->UDiv(op2);
  case Instruction::SRem:
    return op1->SRem(op2);
  case Instruction::URem:
    return op1->URem(op2);
  case Instruction::And:
    return op1->And(op2);
  case Instruction::Or:
    return op1->Or(op2);
  case Instruction::Xor:
    return op1->Xor(op2);
  case Instruction::Shl:
    return op1->Shl(op2);
  case Instruction::LShr:
    return op1->LShr(op2);
  case Instruction::AShr:
    return op1->AShr(op2);
  case Instruction::BitCast:
    return op1;

  case Instruction::IntToPtr:
    return op1->ZExt(getWidthForLLVMType(type));

  case Instruction::PtrToInt:
    return op1->ZExt(getWidthForLLVMType(type));

  case Instruction::GetElementPtr: {
    ref<ConstantExpr> base = op1->ZExt(Context::get().getPointerWidth());

    for (gep_type_iterator ii = gep_type_begin(ce), ie = gep_type_end(ce);
         ii != ie; ++ii) {
      ref<ConstantExpr> addend =
          ConstantExpr::alloc(0, Context::get().getPointerWidth());

      if (LLVM_TYPE_Q StructType *st = dyn_cast<StructType>(*ii)) {
        const StructLayout *sl = kmodule->targetData->getStructLayout(st);
        const ConstantInt *ci = cast<ConstantInt>(ii.getOperand());

        addend = ConstantExpr::alloc(
            sl->getElementOffset((unsigned)ci->getZExtValue()),
            Context::get().getPointerWidth());
      } else {
        const SequentialType *set = cast<SequentialType>(*ii);
        ref<ConstantExpr> index = evalConstant(cast<Constant>(ii.getOperand()), rm);
        unsigned elementSize =
            kmodule->targetData->getTypeStoreSize(set->getElementType());

        index = index->ZExt(Context::get().getPointerWidth());
        addend = index->Mul(
            ConstantExpr::alloc(elementSize, Context::get().getPointerWidth()));
      }

      base = base->Add(addend);
    }

    return base;
  }

  case Instruction::ICmp: {
    switch (ce->getPredicate()) {
    default:
      assert(0 && "unhandled ICmp predicate");
    case ICmpInst::ICMP_EQ:
      return op1->Eq(op2);
    case ICmpInst::ICMP_NE:
      return op1->Ne(op2);
    case ICmpInst::ICMP_UGT:
      return op1->Ugt(op2);
    case ICmpInst::ICMP_UGE:
      return op1->Uge(op2);
    case ICmpInst::ICMP_ULT:
      return op1->Ult(op2);
    case ICmpInst::ICMP_ULE:
      return op1->Ule(op2);
    case ICmpInst::ICMP_SGT:
      return op1->Sgt(op2);
    case ICmpInst::ICMP_SGE:
      return op1->Sge(op2);
    case ICmpInst::ICMP_SLT:
      return op1->Slt(op2);
    case ICmpInst::ICMP_SLE:
      return op1->Sle(op2);
    }
  }

  case Instruction::Select:
    return op1->isTrue() ? op2 : op3;

  // Floating point
  case Instruction::FAdd:
    return op1->FAdd(op2, rm);
  case Instruction::FSub:
    return op1->FSub(op2, rm);
  case Instruction::FMul:
    return op1->FMul(op2, rm);
  case Instruction::FDiv:
    return op1->FDiv(op2, rm);

  case Instruction::FRem: {
    // FIXME:
    llvm_unreachable("Not supported");
  }

  case Instruction::FPTrunc: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->FPTrunc(width, rm);
  }
  case Instruction::FPExt: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->FPExt(width);
  }
  case Instruction::UIToFP: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->UIToFP(width, rm);
  }
  case Instruction::SIToFP: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->SIToFP(width, rm);
  }
  case Instruction::FPToUI: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->FPToUI(width, rm);
  }
  case Instruction::FPToSI: {
    Expr::Width width = getWidthForLLVMType(ce->getType());
    return op1->FPToSI(width, rm);
  }
  case Instruction::FCmp: {
    ref<Expr> result = evaluateFCmp(ce->getPredicate(), op1, op2);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(result)) {
      return ref<ConstantExpr>(CE);
    }
  }
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 1)
    llvm_unreachable("Unsupported expression in evalConstantExpr");
#else
    assert(0 && "Unsupported expression in evalConstantExpr");
#endif
    return op1;
  }
}

  ref<Expr> Executor::evaluateFCmp(unsigned int predicate,
                                   ref<Expr> left, ref<Expr> right) const {
    ref<Expr> result = 0;
    switch (predicate) {
    case FCmpInst::FCMP_FALSE: {
      result = ConstantExpr::alloc(0, Expr::Bool);
      break;
    }
    case FCmpInst::FCMP_OEQ: {
      result = FOEqExpr::create(left, right);
      break;
    }
    case FCmpInst::FCMP_OGT: {
      result = FOGtExpr::create(left, right);
      break;
    }
    case FCmpInst::FCMP_OGE: {
      result = FOGeExpr::create(left, right);
      break;
    }
    case FCmpInst::FCMP_OLT: {
      result = FOLtExpr::create(left, right);
      break;
    }
    case FCmpInst::FCMP_OLE: {
      result = FOLeExpr::create(left, right);
      break;
    }
    case FCmpInst::FCMP_ONE: {
      // This isn't NotExpr(FOEqExpr(arg))
      // because it is an ordered comparision and
      // should return false if either operand is NaN.
      //
      // ¬(isnan(l) ∨ isnan(r)) ∧ ¬(foeq(l, r))
      //
      //  ===
      //
      // ¬( (isnan(l) ∨ isnan(r)) ∨ foeq(l,r))
      result = NotExpr::create(OrExpr::create(IsNaNExpr::either(left, right),
                                              FOEqExpr::create(left, right)));
      break;
    }
    case FCmpInst::FCMP_ORD: {
      result = NotExpr::create(IsNaNExpr::either(left, right));
      break;
    }
    case FCmpInst::FCMP_UNO: {
      result = IsNaNExpr::either(left, right);
      break;
    }
    case FCmpInst::FCMP_UEQ: {
      result = OrExpr::create(IsNaNExpr::either(left, right),
                              FOEqExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_UGT: {
      result = OrExpr::create(IsNaNExpr::either(left, right),
                              FOGtExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_UGE: {
      result = OrExpr::create(IsNaNExpr::either(left, right),
                              FOGeExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_ULT: {
      result = OrExpr::create(IsNaNExpr::either(left, right),
                              FOLtExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_ULE: {
      result = OrExpr::create(IsNaNExpr::either(left, right),
                              FOLeExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_UNE: {
      // Unordered comparision so should
      // return true if either arg is NaN.
      // If either arg to ``FOEqExpr::create()``
      // is a NaN then the result is false which gets
      // negated giving us true when either arg to the instruction
      // is a NaN.
      result = NotExpr::create(FOEqExpr::create(left, right));
      break;
    }
    case FCmpInst::FCMP_TRUE: {
      result = ConstantExpr::alloc(1, Expr::Bool);
      break;
    }
    default:
      llvm_unreachable("Unhandled FCmp predicate");
    }
    return result;
  }
}
