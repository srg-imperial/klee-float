/*===-- fpclassify.c ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===*/
#include "klee/klee.h"

// These are implementations of internal functions found in libm for classifying
// floating point numbers. They have different names to avoid name collisions
// during linking.

// __isnanf
int klee_internal_isnanf(float f) {
  return klee_is_nan_float(f);
}

// __isnan
int klee_internal_isnan(double d) {
  return klee_is_nan_double(d);
}

// __isinff
// returns 1 if +inf, 0 is not infinite, -1 if -inf
int klee_internal_isinff(float f) {
  _Bool isinf = klee_is_infinite_float(f);
  return isinf ? (f > 0 ? 1 : -1) : 0;
}

// __isinf
// returns 1 if +inf, 0 is not infinite, -1 if -inf
int klee_internal_isinf(double d) {
  _Bool isinf = klee_is_infinite_double(d);
  return isinf ? (d > 0 ? 1 : -1) : 0;
}
