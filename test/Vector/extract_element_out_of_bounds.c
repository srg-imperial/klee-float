// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --exit-on-error %t1.bc > %t.log 2>&1
// RUN: FileCheck -input-file=%t.log %s
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t v4ui __attribute__ ((vector_size (16)));
int main() {
  v4ui f = { 0, 1, 2, 3 };
  // Performing these reads should be ExtractElement instructions
  printf("f[0]=%u\n", f[0]);
  printf("f[1]=%u\n", f[1]);
  printf("f[2]=%u\n", f[2]);
  printf("f[3]=%u\n", f[3]);
  // CHECK: extract_element_out_of_bounds.c:[[@LINE+1]]: Out of bounds read when extracting element
  printf("f[4]=%u\n", f[4]); // Out of bounds
  return 0;
}
