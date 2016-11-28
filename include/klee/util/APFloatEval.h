//===-- APFloatEvalSqrt.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_APFLOAT_EVAL_H
#define KLEE_APFLOAT_EVAL_H
#include "llvm/ADT/APFloat.h"

namespace klee {

llvm::APFloat evalSqrt(llvm::APFloat v, llvm::APFloat::roundingMode rm);
}
#endif
