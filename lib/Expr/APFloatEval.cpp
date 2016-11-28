//===-- APFloatEvalSqrt.cpp -------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/util/APFloatEval.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>
#include <fenv.h>
#include <math.h>

namespace {
void change_to_rounding_mode(llvm::APFloat::roundingMode rm) {
  switch (rm) {
  case llvm::APFloat::rmNearestTiesToEven:
    fesetround(FE_TONEAREST);
    break;
  case llvm::APFloat::rmTowardPositive:
    fesetround(FE_UPWARD);
    break;
  case llvm::APFloat::rmTowardNegative:
    fesetround(FE_DOWNWARD);
    break;
  case llvm::APFloat::rmTowardZero:
    fesetround(FE_TOWARDZERO);
    break;
  case llvm::APFloat::rmNearestTiesToAway: {
    llvm::errs() << "rmNearestTiesToAway not supported natively\n";
    abort();
  }
  default:
    llvm_unreachable("Unhandled rounding mode");
  }
}

void restore_fenv(const fenv_t *oldEnv) {
  int result = fesetenv(oldEnv);
  if (result) {
    llvm::errs() << "Failed to restore fenv\n";
    abort();
  }
}
}

namespace klee {

llvm::APFloat evalSqrt(llvm::APFloat v, llvm::APFloat::roundingMode rm) {
  // FIXME: This is such a hack.
  // llvm::APFloat doesn't implement sqrt so evaluate it natively if we
  // can. Otherwise abort.
  //
  // We should figure out how to implement sqrt using APFloat only and
  // upstream the implementation.

  // Store the old floating point environment.
  fenv_t oldEnv;
  int result = fegetenv(&oldEnv);
  if (result) {
    llvm::errs() << "Failed to store fenv\n";
    abort();
  }

  // Eurgh the APFloat API sucks here!
  const llvm::fltSemantics *sem = &(v.getSemantics());
  llvm::APFloat resultAPF = llvm::APFloat::getZero(*sem);
  if (sem == &(llvm::APFloat::IEEEsingle)) {
    float asF = v.convertToFloat();
    assert(sizeof(float) * 8 == 32);

    change_to_rounding_mode(rm);
    float evaluatedValue = sqrtf(asF); // Calculate natively
    // Restore floating point environment
    restore_fenv(&oldEnv);
    resultAPF = llvm::APFloat(evaluatedValue);

  } else if (sem == &(llvm::APFloat::IEEEdouble)) {
    double asD = v.convertToDouble();
    assert(sizeof(double) * 8 == 64);

    change_to_rounding_mode(rm);
    double evaluatedValue = sqrt(asD); // Calculate natively
    // Restore floating point environment
    restore_fenv(&oldEnv);
    resultAPF = llvm::APFloat(evaluatedValue);

  } else {
    llvm::errs() << "Float semantics not supported\n";
    abort();
  }
  return resultAPF;
}
}
