// REQUIRES: z3
// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --solver-backend=z3 --use-query-log=all:core,solver:core %t1.bc
// RUN: test -f %t.klee-out/all-queries.core_solver.smt2
// RUN: test -f %t.klee-out/solver-queries.core_solver.smt2
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>

int main() {
  int c;
  klee_make_symbolic(&c, sizeof(c), "c");
  if (c > 0) {
    printf("c>0\n");
  } else if (c == 0) {
    printf("c==0\n");
  } else {
    assert(c < 0);
    printf("c<0\n");
  }
  return 0;
}
