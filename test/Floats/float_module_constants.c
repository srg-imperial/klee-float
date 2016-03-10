// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t-output.txt 2>&1
// RUN: FileCheck -input-file=%t-output.txt %s
#include "klee/klee.h"

int main() {
  const float x = 0.5;
  const double y = 0.75;
  // OLD_CHECK: x:0x1p-1
  // CHECK: x:5.0E-1
  klee_print_expr("x", x);
  // OLD_CHECK: y:0x1.8p-1
  // CHECK: y:7.5E-1
  klee_print_expr("y", y);
  return 0;
}
// CHECK: KLEE: done: completed paths = 1
