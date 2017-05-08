// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %bc-stats %t1.bc | FileCheck %s
#include <stdio.h>


int main(int argc, char** argv) {
  // CHECK-DAG: num_branches: 4
  // CHECK-DAG: num_function_defns: 1
  switch (argc) {
    case 0:
      printf("0\n");
      break;
    case 1:
      printf("1\n");
      break;
    case 2:
      printf("2\n");
      break;
    case 3:
      printf("3\n");
      break;
    default:
      printf("default\n");
      break;

  }
  return 0;
}
