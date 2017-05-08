// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %bc-stats %t1.bc | FileCheck %s
#include "klee/klee.h"
#include <stdio.h>


int main(int argc, char** argv) {
  uint32_t a;
  klee_make_symbolic(&a, sizeof(a), "a");
  // CHECK-DAG: num_branches: 1
  // CHECK-DAG: num_function_defns: 1
  // CHECK-DAG: estimated_num_symbolic_bytes: 4
  if (argc) {
    printf("argc\n");
  } else {
    printf("!argc\n");
  }
  return 0;
}
