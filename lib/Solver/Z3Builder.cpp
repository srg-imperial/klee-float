//===-- Z3Builder.cpp ------------------------------------------*- C++ -*-====//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/config.h"
#ifdef ENABLE_Z3
#include "Z3Builder.h"

#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/Bits.h"
#include "ConstantDivision.h"
#include "klee/SolverStats.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"

using namespace klee;

namespace {
llvm::cl::opt<bool> UseConstructHashZ3(
    "use-construct-hash-z3",
    llvm::cl::desc("Use hash-consing during Z3 query construction."),
    llvm::cl::init(true));

llvm::cl::opt<std::string> Z3LogInteractionFile(
    "z3-log-interaction", llvm::cl::init(""),
    llvm::cl::desc("Log interaction with Z3 to the specified path"));
}

namespace klee {

// Declared here rather than `Z3Builder.h` so they can be called in gdb.
template <> void Z3NodeHandle<Z3_sort>::dump() {
  llvm::errs() << "Z3SortHandle:\n" << ::Z3_sort_to_string(context, node)
               << "\n";
}
template <> void Z3NodeHandle<Z3_ast>::dump() {
  llvm::errs() << "Z3ASTHandle:\n" << ::Z3_ast_to_string(context, as_ast())
               << "\n";
}

void custom_z3_error_handler(Z3_context ctx, Z3_error_code ec) {
  ::Z3_string errorMsg =
#ifdef HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT
      // Z3 > 4.4.1
      Z3_get_error_msg(ctx, ec);
#else
      // Z3 4.4.1
      Z3_get_error_msg(ec);
#endif
  // FIXME: This is kind of a hack. The value comes from the enum
  // Z3_CANCELED_MSG but this isn't currently exposed by Z3's C API
  if (strcmp(errorMsg, "canceled") == 0) {
    // Solver timeout is not a fatal error
    return;
  }
  llvm::errs() << "Error: Incorrect use of Z3. [" << ec << "] " << errorMsg
               << "\n";
  if (Z3LogInteractionFile.length() > 0) {
    Z3_close_log();
  }
  abort();
}

Z3ArrayExprHash::~Z3ArrayExprHash() {}

void Z3ArrayExprHash::clear() {
  _array_hash.clear();
  clearUpdates();
}

void Z3ArrayExprHash::clearUpdates() {
  _update_node_hash.clear();
}

Z3Builder::Z3Builder(bool autoClearConstructCache)
    : autoClearConstructCache(autoClearConstructCache) {
  if (Z3LogInteractionFile.length() > 0) {
    llvm::errs() << "Logging Z3 interaction to \"" << Z3LogInteractionFile << "\"\n";
    Z3_open_log(Z3LogInteractionFile.c_str());
  }

  // FIXME: Should probably let the client pass in a Z3_config instead
  Z3_config cfg = Z3_mk_config();
  // It is very important that we ask Z3 to let us manage memory so that
  // we are able to cache expressions and sorts.
  ctx = Z3_mk_context_rc(cfg);
  // Make sure we handle any errors reported by Z3.
  Z3_set_error_handler(ctx, custom_z3_error_handler);
  // When emitting Z3 expressions make them SMT-LIBv2 compliant
  Z3_set_ast_print_mode(ctx, Z3_PRINT_SMTLIB2_COMPLIANT);
  Z3_del_config(cfg);
}

Z3Builder::~Z3Builder() {
  // Clear caches so exprs/sorts gets freed before the destroying context
  // they aren associated with.
  clearConstructCache();
  clearReplacements();
  clearSideConstraints();
  _arr_hash.clear();
  Z3_del_context(ctx);
  closeInteractionLog();
}

void Z3Builder::closeInteractionLog() {
  if (Z3LogInteractionFile.length() > 0) {
    Z3_close_log();
  }
}

Z3SortHandle Z3Builder::getBvSort(unsigned width) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_bv_sort(ctx, width), ctx);
}

Z3SortHandle Z3Builder::getArraySort(Z3SortHandle domainSort,
                                     Z3SortHandle rangeSort) {
  // FIXME: cache these
  return Z3SortHandle(Z3_mk_array_sort(ctx, domainSort, rangeSort), ctx);
}

Z3ASTHandle Z3Builder::buildArray(const char *name, unsigned indexWidth,
                                  unsigned valueWidth) {
  Z3SortHandle domainSort = getBvSort(indexWidth);
  Z3SortHandle rangeSort = getBvSort(valueWidth);
  Z3SortHandle t = getArraySort(domainSort, rangeSort);
  Z3_symbol s = Z3_mk_string_symbol(ctx, const_cast<char *>(name));
  return Z3ASTHandle(Z3_mk_const(ctx, s, t), ctx);
}

Z3ASTHandle Z3Builder::getTrue() { return Z3ASTHandle(Z3_mk_true(ctx), ctx); }

Z3ASTHandle Z3Builder::getFalse() { return Z3ASTHandle(Z3_mk_false(ctx), ctx); }

Z3ASTHandle Z3Builder::bvOne(unsigned width) { return bvZExtConst(width, 1); }

Z3ASTHandle Z3Builder::bvZero(unsigned width) { return bvZExtConst(width, 0); }

Z3ASTHandle Z3Builder::bvMinusOne(unsigned width) {
  return bvSExtConst(width, (int64_t)-1);
}

Z3ASTHandle Z3Builder::bvConst32(unsigned width, uint32_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvConst64(unsigned width, uint64_t value) {
  Z3SortHandle t = getBvSort(width);
  return Z3ASTHandle(Z3_mk_unsigned_int64(ctx, value, t), ctx);
}

Z3ASTHandle Z3Builder::bvZExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3ASTHandle expr = Z3ASTHandle(bvConst64(64, value), ctx);
  Z3ASTHandle zero = Z3ASTHandle(bvConst64(64, 0), ctx);
  for (width -= 64; width > 64; width -= 64)
    expr = Z3ASTHandle(Z3_mk_concat(ctx, zero, expr), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, bvConst64(width, 0), expr), ctx);
}

Z3ASTHandle Z3Builder::bvSExtConst(unsigned width, uint64_t value) {
  if (width <= 64)
    return bvConst64(width, value);

  Z3SortHandle t = getBvSort(width - 64);
  if (value >> 63) {
    Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, -1, t), ctx);
    return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
  }

  Z3ASTHandle r = Z3ASTHandle(Z3_mk_int64(ctx, 0, t), ctx);
  return Z3ASTHandle(Z3_mk_concat(ctx, r, bvConst64(64, value)), ctx);
}

Z3ASTHandle Z3Builder::bvBoolExtract(Z3ASTHandle expr, int bit) {
  return Z3ASTHandle(Z3_mk_eq(ctx, bvExtract(expr, bit, bit), bvOne(1)), ctx);
}

Z3ASTHandle Z3Builder::bvExtract(Z3ASTHandle expr, unsigned top,
                                 unsigned bottom) {
  return Z3ASTHandle(Z3_mk_extract(ctx, top, bottom, castToBitVector(expr)), ctx);
}

Z3ASTHandle Z3Builder::eqExpr(Z3ASTHandle a, Z3ASTHandle b) {
  // Handle implicit bitvector/float coercision
  Z3SortHandle aSort = Z3SortHandle(Z3_get_sort(ctx, a), ctx);
  Z3SortHandle bSort = Z3SortHandle(Z3_get_sort(ctx, b), ctx);
  Z3_sort_kind aKind = Z3_get_sort_kind(ctx, aSort);
  Z3_sort_kind bKind = Z3_get_sort_kind(ctx, bSort);

  if (aKind == Z3_BV_SORT && bKind == Z3_FLOATING_POINT_SORT) {
    // Coerce `b` to be a bitvector
    b = castToBitVector(b);
  }

  if (aKind == Z3_FLOATING_POINT_SORT && bKind == Z3_BV_SORT) {
    // Coerce `a` to be a bitvector
    a = castToBitVector(a);
  }
  return Z3ASTHandle(Z3_mk_eq(ctx, a, b), ctx);
}

// logical right shift
Z3ASTHandle Z3Builder::bvRightShift(Z3ASTHandle expr, unsigned shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(
        Z3_mk_concat(ctx, bvZero(shift), bvExtract(exprAsBv, width - 1, shift)),
        ctx);
  }
}

// logical left shift
Z3ASTHandle Z3Builder::bvLeftShift(Z3ASTHandle expr, unsigned shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return expr;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    return Z3ASTHandle(
        Z3_mk_concat(ctx, bvExtract(exprAsBv, width - shift - 1, 0), bvZero(shift)),
        ctx);
  }
}

// left shift by a variable amount on an expression of the specified width
Z3ASTHandle Z3Builder::bvVarLeftShift(Z3ASTHandle expr, Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res =
        iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)), bvLeftShift(exprAsBv, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// logical right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3Builder::bvVarRightShift(Z3ASTHandle expr, Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);
  Z3ASTHandle res = bvZero(width);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  for (int i = width - 1; i >= 0; i--) {
    res =
        iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)), bvRightShift(exprAsBv, i), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

// arithmetic right shift by a variable amount on an expression of the specified
// width
Z3ASTHandle Z3Builder::bvVarArithRightShift(Z3ASTHandle expr,
                                            Z3ASTHandle shift) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  Z3ASTHandle shiftAsBv = castToBitVector(shift);
  unsigned width = getBVLength(exprAsBv);

  // get the sign bit to fill with
  Z3ASTHandle signedBool = bvBoolExtract(exprAsBv, width - 1);

  // start with the result if shifting by width-1
  Z3ASTHandle res = constructAShrByConstant(exprAsBv, width - 1, signedBool);

  // construct a big if-then-elif-elif-... with one case per possible shift
  // amount
  // XXX more efficient to move the ite on the sign outside all exprs?
  // XXX more efficient to sign extend, right shift, then extract lower bits?
  for (int i = width - 2; i >= 0; i--) {
    res = iteExpr(eqExpr(shiftAsBv, bvConst32(width, i)),
                  constructAShrByConstant(exprAsBv, i, signedBool), res);
  }

  // If overshifting, shift to zero
  Z3ASTHandle ex = bvLtExpr(shiftAsBv, bvConst32(getBVLength(shiftAsBv), width));
  res = iteExpr(ex, res, bvZero(width));
  return res;
}

Z3ASTHandle Z3Builder::notExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_not(ctx, expr), ctx);
}

Z3ASTHandle Z3Builder::bvNotExpr(Z3ASTHandle expr) {
  return Z3ASTHandle(Z3_mk_bvnot(ctx, castToBitVector(expr)), ctx);
}

Z3ASTHandle Z3Builder::andExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_and(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::bvAndExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvand(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::orExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  ::Z3_ast args[2] = {lhs, rhs};
  return Z3ASTHandle(Z3_mk_or(ctx, 2, args), ctx);
}

Z3ASTHandle Z3Builder::bvOrExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvor(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::iffExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  Z3SortHandle lhsSort = Z3SortHandle(Z3_get_sort(ctx, lhs), ctx);
  Z3SortHandle rhsSort = Z3SortHandle(Z3_get_sort(ctx, rhs), ctx);
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_get_sort_kind(ctx, rhsSort) &&
         "lhs and rhs sorts must match");
  assert(Z3_get_sort_kind(ctx, lhsSort) == Z3_BOOL_SORT && "args must have BOOL sort");
  return Z3ASTHandle(Z3_mk_iff(ctx, lhs, rhs), ctx);
}

Z3ASTHandle Z3Builder::bvXorExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvxor(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::bvSignExtend(Z3ASTHandle src, unsigned width) {
  Z3ASTHandle srcAsBv = castToBitVector(src);
  unsigned src_width =
      Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, srcAsBv), ctx));
  assert(src_width <= width && "attempted to extend longer data");

  return Z3ASTHandle(Z3_mk_sign_ext(ctx, width - src_width, srcAsBv), ctx);
}

Z3ASTHandle Z3Builder::writeExpr(Z3ASTHandle array, Z3ASTHandle index,
                                 Z3ASTHandle value) {
  return Z3ASTHandle(Z3_mk_store(ctx, array, index, value), ctx);
}

Z3ASTHandle Z3Builder::readExpr(Z3ASTHandle array, Z3ASTHandle index) {
  return Z3ASTHandle(Z3_mk_select(ctx, array, index), ctx);
}

Z3ASTHandle Z3Builder::iteExpr(Z3ASTHandle condition, Z3ASTHandle whenTrue,
                               Z3ASTHandle whenFalse) {
  // Handle implicit bitvector/float coercision
  Z3SortHandle whenTrueSort = Z3SortHandle(Z3_get_sort(ctx, whenTrue), ctx);
  Z3SortHandle whenFalseSort = Z3SortHandle(Z3_get_sort(ctx, whenFalse), ctx);
  Z3_sort_kind whenTrueKind = Z3_get_sort_kind(ctx, whenTrueSort);
  Z3_sort_kind whenFalseKind = Z3_get_sort_kind(ctx, whenFalseSort);

  if (whenTrueKind == Z3_BV_SORT && whenFalseKind == Z3_FLOATING_POINT_SORT) {
    // Coerce `whenFalse` to be a bitvector
    whenFalse = castToBitVector(whenFalse);
  }

  if (whenTrueKind == Z3_FLOATING_POINT_SORT && whenFalseKind == Z3_BV_SORT) {
    // Coerce `whenTrue` to be a bitvector
    whenTrue = castToBitVector(whenTrue);
  }
  return Z3ASTHandle(Z3_mk_ite(ctx, condition, whenTrue, whenFalse), ctx);
}

unsigned Z3Builder::getBVLength(Z3ASTHandle expr) {
  return Z3_get_bv_sort_size(ctx, Z3SortHandle(Z3_get_sort(ctx, expr), ctx));
}

Z3ASTHandle Z3Builder::bvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvult(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::bvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvule(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::sbvLtExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvslt(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::sbvLeExpr(Z3ASTHandle lhs, Z3ASTHandle rhs) {
  return Z3ASTHandle(Z3_mk_bvsle(ctx, castToBitVector(lhs), castToBitVector(rhs)), ctx);
}

Z3ASTHandle Z3Builder::constructAShrByConstant(Z3ASTHandle expr, unsigned shift,
                                               Z3ASTHandle isSigned) {
  Z3ASTHandle exprAsBv = castToBitVector(expr);
  unsigned width = getBVLength(exprAsBv);

  if (shift == 0) {
    return exprAsBv;
  } else if (shift >= width) {
    return bvZero(width); // Overshift to zero
  } else {
    // FIXME: Is this really the best way to interact with Z3?
    return iteExpr(isSigned,
                   Z3ASTHandle(Z3_mk_concat(ctx, bvMinusOne(shift),
                                            bvExtract(exprAsBv, width - 1, shift)),
                               ctx),
                   bvRightShift(exprAsBv, shift));
  }
}

Z3ASTHandle Z3Builder::getInitialArray(const Array *root) {

  assert(root);
  Z3ASTHandle array_expr;
  bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);

  if (!hashed) {
    // Unique arrays by name, so we make sure the name is unique by
    // using the size of the array hash as a counter.
    std::string unique_id = llvm::itostr(_arr_hash._array_hash.size());
    unsigned const uid_length = unique_id.length();
    unsigned const space = (root->name.length() > 32 - uid_length)
                               ? (32 - uid_length)
                               : root->name.length();
    std::string unique_name = root->name.substr(0, space) + unique_id;

    array_expr = buildArray(unique_name.c_str(), root->getDomain(),
                            root->getRange());

    if (root->isConstantArray()) {
      // FIXME: Flush the concrete values into Z3. Ideally we would do this
      // using assertions, which might be faster, but we need to fix the caching
      // to work correctly in that case.
      for (unsigned i = 0, e = root->size; i != e; ++i) {
        Z3ASTHandle prev = array_expr;
        array_expr = writeExpr(
            prev, construct(ConstantExpr::alloc(i, root->getDomain()), 0),
            construct(root->constantValues[i], 0));
      }
    }

    _arr_hash.hashArrayExpr(root, array_expr);
  }

  return (array_expr);
}

Z3ASTHandle Z3Builder::getInitialRead(const Array *root, unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(32, index));
}

Z3ASTHandle Z3Builder::getArrayForUpdate(const Array *root,
                                         const UpdateNode *un) {
  if (!un) {
    return (getInitialArray(root));
  } else {
    // FIXME: This really needs to be non-recursive.
    Z3ASTHandle un_expr;
    bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);

    if (!hashed) {
      un_expr = writeExpr(getArrayForUpdate(root, un->next),
                          construct(un->index, 0), construct(un->value, 0));

      _arr_hash.hashUpdateNodeExpr(un, un_expr);
    }

    return (un_expr);
  }
}

/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3ASTHandle Z3Builder::construct(ref<Expr> e, int *width_out) {
  // See if a replacement variable should be used instead of constructing
  // the replacement expression.
  ExprHashMap<Z3ASTHandle>::iterator replIt = replaceWithExpr.find(e);
  if (replIt != replaceWithExpr.end()) {
    if (width_out)
      *width_out = e->getWidth();
    return replIt->second;
  }
  // TODO: We could potentially use Z3_simplify() here
  // to store simpler expressions.
  if (!UseConstructHashZ3 || isa<ConstantExpr>(e)) {
    return constructActual(e, width_out);
  } else {
    ExprHashMap<std::pair<Z3ASTHandle, unsigned> >::iterator it =
        constructed.find(e);
    if (it != constructed.end()) {
      if (width_out)
        *width_out = it->second.second;
      return it->second.first;
    } else {
      int width;
      if (!width_out)
        width_out = &width;
      Z3ASTHandle res = constructActual(e, width_out);
      constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
      return res;
    }
  }
}

/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
Z3ASTHandle Z3Builder::constructActual(ref<Expr> e, int *width_out) {
  int width;
  if (!width_out)
    width_out = &width;

  ++stats::queryConstructs;

  switch (e->getKind()) {
  case Expr::Constant: {
    ConstantExpr *CE = cast<ConstantExpr>(e);
    *width_out = CE->getWidth();

    // Coerce to bool if necessary.
    if (*width_out == 1)
      return CE->isTrue() ? getTrue() : getFalse();

    Z3ASTHandle Res;
    if (*width_out <= 32) {
      // Fast path.
      Res = bvConst32(*width_out, CE->getZExtValue(32));
    } else if (*width_out <= 64) {
      // Fast path.
      Res = bvConst64(*width_out, CE->getZExtValue());
    } else {
      ref<ConstantExpr> Tmp = CE;
      Res = bvConst64(64, Tmp->Extract(0, 64)->getZExtValue());
      while (Tmp->getWidth() > 64) {
        Tmp = Tmp->Extract(64, Tmp->getWidth() - 64);
        unsigned Width = std::min(64U, Tmp->getWidth());
        Res = Z3ASTHandle(
            Z3_mk_concat(
                ctx, bvConst64(Width, Tmp->Extract(0, Width)->getZExtValue()),
                Res),
            ctx);
      }
    }

    // Coerce to float if necesary
    if (CE->isFloat()) {
      Res = castToFloat(Res);
    }
    return Res;
  }

  // Special
  case Expr::NotOptimized: {
    NotOptimizedExpr *noe = cast<NotOptimizedExpr>(e);
    return construct(noe->src, width_out);
  }

  case Expr::Read: {
    ReadExpr *re = cast<ReadExpr>(e);
    assert(re && re->updates.root);
    *width_out = re->updates.root->getRange();
    return readExpr(getArrayForUpdate(re->updates.root, re->updates.head),
                    construct(re->index, 0));
  }

  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    Z3ASTHandle cond = construct(se->cond, 0);
    Z3ASTHandle tExpr = construct(se->trueExpr, width_out);
    Z3ASTHandle fExpr = construct(se->falseExpr, width_out);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    Z3ASTHandle res = construct(ce->getKid(numKids - 1), 0);
    for (int i = numKids - 2; i >= 0; i--) {
      res =
          Z3ASTHandle(Z3_mk_concat(ctx, construct(ce->getKid(i), 0), res), ctx);
    }
    *width_out = ce->getWidth();
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    Z3ASTHandle src = construct(ee->expr, width_out);
    *width_out = ee->getWidth();
    if (*width_out == 1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return bvExtract(src, ee->offset + *width_out - 1, ee->offset);
    }
  }

  // Casting

  case Expr::ZExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ASTHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvOne(*width_out), bvZero(*width_out));
    } else {
      assert(*width_out > srcWidth && "Invalid width_out");
      return Z3ASTHandle(Z3_mk_concat(ctx, bvZero(*width_out - srcWidth),
                                      castToBitVector(src)),
                         ctx);
    }
  }

  case Expr::SExt: {
    int srcWidth;
    CastExpr *ce = cast<CastExpr>(e);
    Z3ASTHandle src = construct(ce->src, &srcWidth);
    *width_out = ce->getWidth();
    if (srcWidth == 1) {
      return iteExpr(src, bvMinusOne(*width_out), bvZero(*width_out));
    } else {
      return bvSignExtend(src, *width_out);
    }
  }

  case Expr::FPExt: {
    int srcWidth;
    FPExtExpr *ce = cast<FPExtExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid FPExt width");
    assert(*width_out >= srcWidth && "Invalid FPExt");
    // Just use any arounding mode here as we are extending
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_float(
            ctx, getRoundingModeSort(llvm::APFloat::rmNearestTiesToEven), src,
            getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::FPTrunc: {
    int srcWidth;
    FPTruncExpr *ce = cast<FPTruncExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid FPTrunc width");
    assert(*width_out <= srcWidth && "Invalid FPTrunc");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_float(ctx, getRoundingModeSort(ce->roundingMode), src,
                              getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::FPToUI: {
    int srcWidth;
    FPToUIExpr *ce = cast<FPToUIExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(srcWidth)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid FPToUI width");
    return Z3ASTHandle(Z3_mk_fpa_to_ubv(ctx,
                                        getRoundingModeSort(ce->roundingMode),
                                        src, *width_out),
                       ctx);
  }

  case Expr::FPToSI: {
    int srcWidth;
    FPToSIExpr *ce = cast<FPToSIExpr>(e);
    Z3ASTHandle src = castToFloat(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(srcWidth)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid FPToSI width");
    return Z3ASTHandle(Z3_mk_fpa_to_sbv(ctx,
                                        getRoundingModeSort(ce->roundingMode),
                                        src, *width_out),
                       ctx);
  }

  case Expr::UIToFP: {
    int srcWidth;
    UIToFPExpr *ce = cast<UIToFPExpr>(e);
    Z3ASTHandle src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid UIToFP width");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_unsigned(ctx, getRoundingModeSort(ce->roundingMode),
                                 src, getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  case Expr::SIToFP: {
    int srcWidth;
    SIToFPExpr *ce = cast<SIToFPExpr>(e);
    Z3ASTHandle src = castToBitVector(construct(ce->src, &srcWidth));
    *width_out = ce->getWidth();
    assert(&(ConstantExpr::widthToFloatSemantics(*width_out)) !=
               &(llvm::APFloat::Bogus) &&
           "Invalid SIToFP width");
    return Z3ASTHandle(
        Z3_mk_fpa_to_fp_signed(ctx, getRoundingModeSort(ce->roundingMode), src,
                               getFloatSortFromBitWidth(*width_out)),
        ctx);
  }

  // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(ae->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(ae->right, width_out));
    assert(*width_out != 1 && "uncanonicalized add");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvadd(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(se->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(se->right, width_out));
    assert(*width_out != 1 && "uncanonicalized sub");
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsub(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    Z3ASTHandle right = castToBitVector(construct(me->right, width_out));
    assert(*width_out != 1 && "uncanonicalized mul");
    Z3ASTHandle left = castToBitVector(construct(me->left, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvmul(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized udiv");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();
        if (bits64::isPowerOfTwo(divisor))
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
      }
    }

    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvudiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized sdiv");
    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsdiv(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    assert(*width_out != 1 && "uncanonicalized urem");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          int bits = bits64::indexOfSingleBit(divisor);
          assert(bits >= 0 && "bit index cannot be negative");
          assert(bits64::indexOfSingleBit(divisor) < INT32_MAX);

          // special case for modding by 1 or else we bvExtract -1:0
          if (bits == 0) {
            return bvZero(*width_out);
          } else {
            assert(*width_out > bits && "invalid width_out");
            return Z3ASTHandle(Z3_mk_concat(ctx, bvZero(*width_out - bits),
                                            bvExtract(left, bits - 1, 0)),
                               ctx);
          }
        }
      }
    }

    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvurem(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(de->left, width_out));
    Z3ASTHandle right = castToBitVector(construct(de->right, width_out));
    assert(*width_out != 1 && "uncanonicalized srem");
    // LLVM's srem instruction says that the sign follows the dividend
    // (``left``).
    // Z3's C API says ``Z3_mk_bvsrem()`` does this so these seem to match.
    Z3ASTHandle result = Z3ASTHandle(Z3_mk_bvsrem(ctx, left, right), ctx);
    assert(getBVLength(result) == static_cast<unsigned>(*width_out) &&
           "width mismatch");
    return result;
  }

  // Bitwise
  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    Z3ASTHandle expr = construct(ne->expr, width_out);
    if (*width_out == 1) {
      return notExpr(expr);
    } else {
      return bvNotExpr(expr);
    }
  }

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    Z3ASTHandle left = construct(ae->left, width_out);
    Z3ASTHandle right = construct(ae->right, width_out);
    if (*width_out == 1) {
      return andExpr(left, right);
    } else {
      return bvAndExpr(left, right);
    }
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    Z3ASTHandle left = construct(oe->left, width_out);
    Z3ASTHandle right = construct(oe->right, width_out);
    if (*width_out == 1) {
      return orExpr(left, right);
    } else {
      return bvOrExpr(left, right);
    }
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    Z3ASTHandle left = construct(xe->left, width_out);
    Z3ASTHandle right = construct(xe->right, width_out);

    if (*width_out == 1) {
      // XXX check for most efficient?
      return iteExpr(left, Z3ASTHandle(notExpr(right)), right);
    } else {
      return bvXorExpr(left, right);
    }
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    assert(*width_out != 1 && "uncanonicalized shl");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(se->right, &shiftWidth);
      return bvVarLeftShift(left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    Z3ASTHandle left = construct(lse->left, width_out);
    assert(*width_out != 1 && "uncanonicalized lshr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned)CE->getLimitedValue());
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(lse->right, &shiftWidth);
      return bvVarRightShift(left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    Z3ASTHandle left = castToBitVector(construct(ase->left, width_out));
    assert(*width_out != 1 && "uncanonicalized ashr");

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
      unsigned shift = (unsigned)CE->getLimitedValue();
      Z3ASTHandle signedBool = bvBoolExtract(left, *width_out - 1);
      return constructAShrByConstant(left, shift, signedBool);
    } else {
      int shiftWidth;
      Z3ASTHandle amount = construct(ase->right, &shiftWidth);
      return bvVarArithRightShift(left, amount);
    }
  }

  // Comparison

  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    Z3ASTHandle left = construct(ee->left, width_out);
    Z3ASTHandle right = construct(ee->right, width_out);
    if (*width_out == 1) {
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
        if (CE->isTrue())
          return right;
        return notExpr(right);
      } else {
        return iffExpr(left, right);
      }
    } else {
      *width_out = 1;
      return eqExpr(left, right);
    }
  }

  case Expr::Ult: {
    UltExpr *ue = cast<UltExpr>(e);
    Z3ASTHandle left = construct(ue->left, width_out);
    Z3ASTHandle right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ult");
    *width_out = 1;
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    Z3ASTHandle left = construct(ue->left, width_out);
    Z3ASTHandle right = construct(ue->right, width_out);
    assert(*width_out != 1 && "uncanonicalized ule");
    *width_out = 1;
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    Z3ASTHandle right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized slt");
    *width_out = 1;
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    Z3ASTHandle left = construct(se->left, width_out);
    Z3ASTHandle right = construct(se->right, width_out);
    assert(*width_out != 1 && "uncanonicalized sle");
    *width_out = 1;
    return sbvLeExpr(left, right);
  }

  case Expr::FOEq: {
    FOEqExpr *fcmp = cast<FOEqExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_eq(ctx, left, right), ctx);
  }

  case Expr::FOLt: {
    FOLtExpr *fcmp = cast<FOLtExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_lt(ctx, left, right), ctx);
  }

  case Expr::FOLe: {
    FOLeExpr *fcmp = cast<FOLeExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_leq(ctx, left, right), ctx);
  }

  case Expr::FOGt: {
    FOGtExpr *fcmp = cast<FOGtExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_gt(ctx, left, right), ctx);
  }

  case Expr::FOGe: {
    FOGeExpr *fcmp = cast<FOGeExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fcmp->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fcmp->right, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_geq(ctx, left, right), ctx);
  }

  case Expr::IsNaN: {
    IsNaNExpr *ine = cast<IsNaNExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_nan(ctx, arg), ctx);
  }

  case Expr::IsInfinite: {
    IsInfiniteExpr *iie = cast<IsInfiniteExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(iie->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_infinite(ctx, arg), ctx);
  }

  case Expr::IsNormal: {
    IsNormalExpr *ine = cast<IsNormalExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ine->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_normal(ctx, arg), ctx);
  }

  case Expr::IsSubnormal: {
    IsSubnormalExpr *ise = cast<IsSubnormalExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(ise->expr, width_out));
    *width_out = 1;
    return Z3ASTHandle(Z3_mk_fpa_is_subnormal(ctx, arg), ctx);
  }

  case Expr::FAdd: {
    FAddExpr *fadd = cast<FAddExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fadd->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fadd->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FAdd");
    return Z3ASTHandle(Z3_mk_fpa_add(ctx,
                                     getRoundingModeSort(fadd->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FSub: {
    FSubExpr *fsub = cast<FSubExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fsub->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fsub->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FSub");
    return Z3ASTHandle(Z3_mk_fpa_sub(ctx,
                                     getRoundingModeSort(fsub->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FMul: {
    FMulExpr *fmul = cast<FMulExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fmul->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fmul->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FMul");
    return Z3ASTHandle(Z3_mk_fpa_mul(ctx,
                                     getRoundingModeSort(fmul->roundingMode),
                                     left, right),
                       ctx);
  }

  case Expr::FDiv: {
    FDivExpr *fdiv = cast<FDivExpr>(e);
    Z3ASTHandle left = castToFloat(construct(fdiv->left, width_out));
    Z3ASTHandle right = castToFloat(construct(fdiv->right, width_out));
    assert(*width_out != 1 && "uncanonicalized FDiv");
    return Z3ASTHandle(Z3_mk_fpa_div(ctx,
                                     getRoundingModeSort(fdiv->roundingMode),
                                     left, right),
                       ctx);
  }
  case Expr::FSqrt: {
    FSqrtExpr *fsqrt = cast<FSqrtExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(fsqrt->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FSqrt");
    return Z3ASTHandle(
        Z3_mk_fpa_sqrt(ctx, getRoundingModeSort(fsqrt->roundingMode), arg),
        ctx);
  }
  case Expr::FAbs: {
    FAbsExpr *fabsExpr = cast<FAbsExpr>(e);
    Z3ASTHandle arg = castToFloat(construct(fabsExpr->expr, width_out));
    assert(*width_out != 1 && "uncanonicalized FAbs");
    return Z3ASTHandle(Z3_mk_fpa_abs(ctx, arg), ctx);
  }
// unused due to canonicalization
#if 0
  case Expr::Ne:
  case Expr::Ugt:
  case Expr::Uge:
  case Expr::Sgt:
  case Expr::Sge:
#endif

  default:
    assert(0 && "unhandled Expr type");
    return getTrue();
  }
}

Z3ASTHandle Z3Builder::getx87FP80ExplicitSignificandIntegerBit(Z3ASTHandle e) {
#ifndef NDEBUG
  // Check the passed in expression is the right type.
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  assert(Z3_get_sort_kind(ctx, currentSort) == Z3_FLOATING_POINT_SORT);
  unsigned exponentBits = Z3_fpa_get_ebits(ctx, currentSort);
  unsigned significandBits = Z3_fpa_get_sbits(ctx, currentSort);
  assert(exponentBits == 15);
  assert(significandBits == 64);
#endif
  // If the number is a denormal or zero then the implicit integer bit is zero
  // otherwise it is one.  Z3ASTHandle isDenormal =
  Z3ASTHandle isDenormal = Z3ASTHandle(Z3_mk_fpa_is_subnormal(ctx, e), ctx);
  Z3ASTHandle isZero = Z3ASTHandle(Z3_mk_fpa_is_zero(ctx, e), ctx);

  // FIXME: Cache these constants somewhere
  Z3SortHandle oneBitBvSort = getBvSort(/*width=*/1);
#ifndef NDEBUG
  assert(Z3_get_sort_kind(ctx, oneBitBvSort) == Z3_BV_SORT);
  assert(Z3_get_bv_sort_size(ctx, oneBitBvSort) == 1);
#endif
  Z3ASTHandle oneBvOne =
      Z3ASTHandle(Z3_mk_unsigned_int64(ctx, 1, oneBitBvSort), ctx);
  Z3ASTHandle zeroBvOne =
      Z3ASTHandle(Z3_mk_unsigned_int64(ctx, 0, oneBitBvSort), ctx);
  Z3ASTHandle significandIntegerBitCondition = orExpr(isDenormal, isZero);
  Z3ASTHandle significandIntegerBitConstrainedValue = Z3ASTHandle(
      Z3_mk_ite(ctx, significandIntegerBitCondition, zeroBvOne, oneBvOne), ctx);
  return significandIntegerBitConstrainedValue;
}

Z3ASTHandle Z3Builder::castToFloat(Z3ASTHandle e) {
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  Z3_sort_kind kind = Z3_get_sort_kind(ctx, currentSort);
  switch (kind) {
  case Z3_FLOATING_POINT_SORT:
    // Already a float
    return e;
  case Z3_BV_SORT: {
    unsigned bitWidth = Z3_get_bv_sort_size(ctx, currentSort);
    switch (bitWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return Z3ASTHandle(
          Z3_mk_fpa_to_fp_bv(ctx, e, getFloatSortFromBitWidth(bitWidth)), ctx);
    case Expr::Fl80: {
      // The bit pattern used by x87 fp80 and what we use in Z3 are different
      //
      // x87 fp80
      //
      // Sign Exponent Significand
      // [1]    [15]   [1] [63]
      //
      // The exponent has bias 16383 and the significand has the integer portion
      // as an explicit bit
      //
      // 79-bit IEEE-754 encoding used in Z3
      //
      // Sign Exponent [Significand]
      // [1]    [15]       [63]
      //
      // Exponent has bias 16383 (2^(15-1) -1) and the significand has
      // the integer portion as an implicit bit.
      //
      // We need to provide the mapping here and also emit a side constraint
      // to make sure the explicit bit is appropriately constrained so when
      // Z3 generates a model we get the correct bit pattern back.
      //
      // This assumes Z3's IEEE semantics, x87 fp80 actually
      // has additional semantics due to the explicit bit (See 8.2.2
      // "Unsupported  Double Extended-Precision Floating-Point Encodings and
      // Pseudo-Denormals" in the Intel 64 and IA-32 Architectures Software
      // Developer's Manual) but this encoding means we can't model these
      // unsupported values in Z3.
      //
      // Note this code must kept in sync with `Z3Builder::castToBitVector()`.
      // Which performs the inverse operation here.
      //
      // TODO: Experiment with creating a constraint that transforms these
      // unsupported bit patterns into a Z3 NaN to approximate the behaviour
      // from those values.

      // Note we try very hard here to avoid calling into our functions
      // here that do implicit casting so we can never recursively call
      // into this function.
      Z3ASTHandle signBit =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/79, /*low=*/79, e), ctx);
      Z3ASTHandle exponentBits =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/78, /*low=*/64, e), ctx);
      Z3ASTHandle significandIntegerBit =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/63, /*low=*/63, e), ctx);
      Z3ASTHandle significandFractionBits =
          Z3ASTHandle(Z3_mk_extract(ctx, /*high=*/62, /*low=*/0, e), ctx);

      Z3ASTHandle ieeeBitPattern =
          Z3ASTHandle(Z3_mk_concat(ctx, signBit, exponentBits), ctx);
      ieeeBitPattern = Z3ASTHandle(
          Z3_mk_concat(ctx, ieeeBitPattern, significandFractionBits), ctx);
#ifndef NDEBUG
      Z3SortHandle ieeeBitPatternSort =
          Z3SortHandle(Z3_get_sort(ctx, ieeeBitPattern), ctx);
      assert(Z3_get_sort_kind(ctx, ieeeBitPatternSort) == Z3_BV_SORT);
      assert(Z3_get_bv_sort_size(ctx, ieeeBitPatternSort) == 79);
#endif

      Z3ASTHandle ieeeBitPatternAsFloat =
          Z3ASTHandle(Z3_mk_fpa_to_fp_bv(ctx, ieeeBitPattern,
                                         getFloatSortFromBitWidth(bitWidth)),
                      ctx);

      // Generate side constraint on the significand integer bit. It is not
      // used in `ieeeBitPatternAsFloat` so we need to constrain that bit to
      // have the correct value so that when Z3 gives a model the bit pattern
      // has the right value for x87 fp80.
      //
      // If the number is a denormal or zero then the implicit integer bit is
      // zero otherwise it is one.
      Z3ASTHandle significandIntegerBitConstrainedValue =
          getx87FP80ExplicitSignificandIntegerBit(ieeeBitPatternAsFloat);
      Z3ASTHandle significandIntegerBitConstraint =
          Z3ASTHandle(Z3_mk_eq(ctx, significandIntegerBit,
                               significandIntegerBitConstrainedValue),
                      ctx);
#ifndef NDEBUG
      // We will generate side constraints for constants too so we
      // need to be really careful not to generate false! Check this
      // by trying to simplify the side constraint.
      Z3ASTHandle simplifiedSignificandIntegerBitConstraint =
          Z3ASTHandle(Z3_simplify(ctx, significandIntegerBitConstraint), ctx);
      Z3_lbool simplifiedValue =
          Z3_get_bool_value(ctx, simplifiedSignificandIntegerBitConstraint);
      if (simplifiedValue == Z3_L_FALSE) {
        llvm::errs() << "Generated side constraint:\n";
        significandIntegerBitConstraint.dump();
        llvm::errs() << "\nSimplifies to false.";
        abort();
      }
#endif
      sideConstraints.push_back(significandIntegerBitConstraint);
      return ieeeBitPatternAsFloat;
    } break;
    default:
      llvm_unreachable("Unhandled width when casting bitvector to float");
    }
  }
  default:
    llvm_unreachable("Sort cannot be cast to float");
  }
}

Z3ASTHandle Z3Builder::castToBitVector(Z3ASTHandle e) {
  Z3SortHandle currentSort = Z3SortHandle(Z3_get_sort(ctx, e), ctx);
  Z3_sort_kind kind = Z3_get_sort_kind(ctx, currentSort);
  switch (kind) {
  case Z3_BV_SORT:
    // Already a bitvector
    return e;
  case Z3_FLOATING_POINT_SORT: {
    // Note this picks a single representation for NaN which means
    // `castToBitVector(castToFloat(e))` might not equal `e`.
    unsigned exponentBits = Z3_fpa_get_ebits(ctx, currentSort);
    unsigned significandBits =
        Z3_fpa_get_sbits(ctx, currentSort); // Includes implicit bit
    unsigned floatWidth = exponentBits + significandBits;
    switch (floatWidth) {
    case Expr::Int16:
    case Expr::Int32:
    case Expr::Int64:
    case Expr::Int128:
      return Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);
    case 79: {
      // This is Expr::Fl80 (64 bit exponent, 15 bit significand) but due to
      // the "implicit" bit actually being implicit in x87 fp80 the sum of
      // the exponent and significand bitwidth is 79 not 80.

      // Get Z3's IEEE representation
      Z3ASTHandle ieeeBits = Z3ASTHandle(Z3_mk_fpa_to_ieee_bv(ctx, e), ctx);

      // Construct the x87 fp80 bit representation
      Z3ASTHandle signBit = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/78, /*low=*/78, ieeeBits), ctx);
      Z3ASTHandle exponentBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/77, /*low=*/63, ieeeBits), ctx);
      Z3ASTHandle significandIntegerBit =
          getx87FP80ExplicitSignificandIntegerBit(e);
      Z3ASTHandle significandFractionBits = Z3ASTHandle(
          Z3_mk_extract(ctx, /*high=*/62, /*low=*/0, ieeeBits), ctx);

      Z3ASTHandle x87FP80Bits =
          Z3ASTHandle(Z3_mk_concat(ctx, signBit, exponentBits), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandIntegerBit), ctx);
      x87FP80Bits = Z3ASTHandle(
          Z3_mk_concat(ctx, x87FP80Bits, significandFractionBits), ctx);
#ifndef NDEBUG
      Z3SortHandle x87FP80BitsSort =
          Z3SortHandle(Z3_get_sort(ctx, x87FP80Bits), ctx);
      assert(Z3_get_sort_kind(ctx, x87FP80BitsSort) == Z3_BV_SORT);
      assert(Z3_get_bv_sort_size(ctx, x87FP80BitsSort) == 80);
#endif
      return x87FP80Bits;
    }
    default:
      llvm_unreachable("Unhandled width when casting float to bitvector");
    }
  }
  default:
    llvm_unreachable("Sort cannot be cast to float");
  }
}


Z3SortHandle Z3Builder::getFloatSortFromBitWidth(unsigned bitWidth) {
  // FIXME: Cache these
  switch (bitWidth) {
  case Expr::Int16: {
    return Z3SortHandle(Z3_mk_fpa_sort_16(ctx), ctx);
  }
  case Expr::Int32: {
    return Z3SortHandle(Z3_mk_fpa_sort_32(ctx), ctx);
  }
  case Expr::Int64: {
    return Z3SortHandle(Z3_mk_fpa_sort_64(ctx), ctx);
  }
  case Expr::Fl80: {
    // Note this is an IEEE754 with a 15 bit exponent
    // and 64 bit significand. This is not the same
    // as x87 fp80 which has a different binary encoding.
    // We can use this Z3 type to get the appropriate
    // amount of precision. We just have to be very
    // careful which casting between floats and bitvectors.
    //
    // Note that the number of significand bits includes the "implicit"
    // bit (which is not implicit for x87 fp80).
    return Z3SortHandle(Z3_mk_fpa_sort(ctx, /*ebits=*/15, /*sbits=*/64), ctx);
  }
  case Expr::Int128: {
    return Z3SortHandle(Z3_mk_fpa_sort_128(ctx), ctx);
  }
  default:
    assert(0 &&
           "bitWidth cannot converted to a IEEE-754 binary-* number by Z3");
  }
}

Z3ASTHandle Z3Builder::getRoundingModeSort(llvm::APFloat::roundingMode rm) {
  // FIXME: Cache these
  switch(rm) {
    case llvm::APFloat::rmNearestTiesToEven:
      return Z3ASTHandle(Z3_mk_fpa_round_nearest_ties_to_even(ctx), ctx);
    case llvm::APFloat::rmTowardPositive:
      return Z3ASTHandle(Z3_mk_fpa_round_toward_positive(ctx), ctx);
    case llvm::APFloat::rmTowardNegative:
      return Z3ASTHandle(Z3_mk_fpa_round_toward_negative(ctx), ctx);
    case llvm::APFloat::rmTowardZero:
      return Z3ASTHandle(Z3_mk_fpa_round_toward_zero(ctx), ctx);
    case llvm::APFloat::rmNearestTiesToAway:
      return Z3ASTHandle(Z3_mk_fpa_round_nearest_ties_to_away(ctx), ctx);
    default:
      llvm_unreachable("Unhandled rounding mode");
  }
}

Z3ASTHandle Z3Builder::getFreshBitVectorVariable(unsigned bitWidth,
                                                 const char *prefix) {
  // Create fresh variable
  Z3SortHandle sort = getBvSort(bitWidth);
  Z3ASTHandle newVar =
      Z3ASTHandle(Z3_mk_fresh_const(ctx, /*prefix=*/prefix, sort), ctx);
  return newVar;
}

bool Z3Builder::addReplacementExpr(const ref<Expr> e, Z3ASTHandle replacement) {
  std::pair<ExprHashMap<Z3ASTHandle>::iterator, bool> result =
      replaceWithExpr.insert(std::make_pair(e, replacement));
  return result.second;
}

void Z3Builder::clearReplacements() {
  // FIXME: Try to find a way to avoid doing this. We don't always
  // need to clear everything in the cache.
  // We have to clear the cached update expressions because they may
  // use replacement variables.
  _arr_hash.clearUpdates();
  replaceWithExpr.clear();
}
}
#endif // ENABLE_Z3
