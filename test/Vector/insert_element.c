// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc
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
  printf("f[0]=%u\n", f[0]);
  printf("f[1]=%u\n", f[1]);
  printf("f[2]=%u\n", f[2]);
  printf("f[3]=%u\n", f[3]);
  assert(f[0] == 255);
  assert(f[1] == 128);
  assert(f[2] == 1);
  assert(f[3] == 19);
  return 0;
}
