// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %bc-stats %t1.bc | FileCheck %s
#include <stdio.h>


int main(int argc, char** argv) {
  // CHECK-DAG: num_branches: 1
  // CHECK-DAG: num_function_defns: 1
  if (argc) {
    printf("argc\n");
  } else {
    printf("!argc\n");
  }
  return 0;
}
