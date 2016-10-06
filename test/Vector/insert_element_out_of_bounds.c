// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t.log 2>&1
// RUN: FileCheck -input-file=%t.log %s
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
int main() {
  v4ui f = { 0, 1, 2, 3 };
  klee_print_expr("f:=", f);
  // Performing these writes should be InsertElement instructions
  f[0] = 255;
  klee_print_expr("f after write to [0]", f);
  f[1] = 128;
  klee_print_expr("f after write to [1]", f);
  f[2] = 1;
  klee_print_expr("f after write to [2]", f);
  f[3] = 19;
  klee_print_expr("f after write to [3]", f);
  // CHECK: insert_element_out_of_bounds.c:[[@LINE+1]]: Out of bounds write when inserting element
  f[4] = 255; // Out of bounds write
  klee_print_expr("f after write to [4]", f);
  return 0;
}
