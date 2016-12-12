// RUN: %llvmgcc %s -emit-llvm -O0 -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --single-repr-for-nan=1 -search=dfs -use-construct-hash-z3=1 -debug-assignment-validating-solver -z3-validate-models --output-dir=%t.klee-out --exit-on-error %t1.bc
#include "klee/klee.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// NOTE: --single-repr-for-nan is required for this test
// because this benchmark branches on the lower bits
// in a NaN whose value are not defined by IEEE754.
// Without this option KLEE's Expr and Z3's Expr
// do not agree and the evaluation of FAdd on a NaN
// resulting in un-satisfiable constraints.

int sink = 0;

int main() {
  float f;
  klee_make_symbolic(&f, sizeof(float), "f");
  assert(sizeof(int) == sizeof(float));

  // Do some operation that means that f is more
  // than just a read of some bytes. It now is
  // an expression of type float.
  f = f + 1;

  int x = 0;
  memcpy(&x, &f, sizeof(float));

  // Now do some bitvector computation on x. The purpose
  // of this is catch expression casting issues in the Z3Builder
  // where ``x`` is a float expression but then we try to do
  // bitvector operations on it.
  int action = klee_range(0, 100, "action");
  int newValue = 0;
  unsigned int newValueUnsigned = 0;
  switch (action) {
    case 0: // +
      newValue = x + 1;
      printf("add\n");
      break;
    case 1: {
      // signed sub
      // Don't use constants here to avoid canonicalizing as AddExpr
      int havoc = 0;
      klee_make_symbolic(&havoc, sizeof(int), "havoc");
      newValue = x - havoc;
      printf("signed sub\n");
      klee_print_expr("signed_sub_expr", newValue);
      break;
    }
    case 2: {
      // unsigned sub
      // Don't use constants here to avoid canonicalizing as AddExpr
      unsigned int havoc = 0;
      klee_make_symbolic(&havoc, sizeof(unsigned int), "havoc");
      newValue = ( (unsigned) x) - havoc;
      printf("unsigned sub\n");
      break;
    }
    case 3: // *
      newValue = x * 3;
      printf("multiply\n");
      break;
    case 4: // Unsigned divide
      newValueUnsigned = ((unsigned int) x) / 3;
      printf("signed divide\n");
      break;
    case 5: // Signed divide
      newValue = x / 3;
      printf("signed divide\n");
      break;
    case 6: // Unsigned
      newValueUnsigned = ((unsigned int) x) % 3;
      printf("unsigned modulo\n");
      break;
    case 7: // Signed modulo
      newValue = x % 3;
      printf("signed modulo\n");
      break;
    case 8: // &
      newValue = x & 16;
      printf("and\n");
      break;
    case 9: // |
      newValue = x | 13;
      printf("or\n");
      break;
    case 10: // ^
      newValue = x ^ 13;
      printf("xor\n");
      break;
    case 11:
      // Bitwise not
      // FIXME: This gets canonicalised as an xor with -1 so we
      // aren't really testing this in the Z3Builder.
      newValue = ~x;
      printf("bitwise not\n");
      break;
    case 12:
      // << by constant
      newValue = x << 3;
      printf("<< by constant\n");
      break;
    case 13: {
      // << by symbolic
      unsigned int shiftAmount = klee_range(0, 32, "shiftAmount");
      newValue = x << shiftAmount;
      printf("<< by symbolic\n");
      break;
    }
    case 14:
      // Arithmetic right shift by constant
      newValue = x >> 3;
      printf("Arithmetic right shift by constant\n");
      break;
    case 15: {
      // Arithmetic right shift by symbolic
      unsigned int shiftAmount = klee_range(0, 32, "shiftAmount1");
      newValue = x >> shiftAmount;
      printf("Arithmetic right shift by symbolic\n");
      break;
    }
    case 16:
      // Logical right shift by constant
      newValueUnsigned = ((unsigned int) x) >> 3;
      printf("Logical right shift by constant\n");
      break;
    case 17: {
      // Logical right shift by symbolic
      unsigned int shiftAmount = klee_range(0, 32, "shiftAmount1");
      newValueUnsigned = ((unsigned int) x) >> shiftAmount;
      printf("Logical right shift by symbolic\n");
      break;
    }
    case 18:
      // slt
      newValue = (x < 5);
      printf("slt\n");
      break;
    case 19:
      // sle
      newValue = (x <= 5);
      printf("sle\n");
      break;
    case 20:
      // sgt
      // Note due to canonicalisation this becomes sle.
      newValue = (x > 5);
      printf("sgt\n");
      break;
    case 21:
      // sge
      // Note due to canonicalisation this becomes slt.
      newValue = (x >= 5);
      printf("sge\n");
      break;
    case 22:
      // ult
      newValueUnsigned = ((unsigned int) x) < 5;
      printf("ult\n");
      break;
    case 23:
      // ule
      newValueUnsigned = ((unsigned int) x) <= 5;
      printf("ule\n");
      break;
    case 24:
      // ugt
      // Note due to canonicalisation this becomes ule
      newValueUnsigned = ((unsigned int) x) > 5;
      printf("ugt\n");
      break;
    case 25:
      // uge
      // Note due to canonicalisation this becomes ult
      newValueUnsigned = ((unsigned int) x) >= 5;
      printf("uge\n");
      break;
    case 26: {
      // SItoFP
      float dummy = (float) x;
      newValue = dummy > 0.0f;
      printf("SItoFP\n");
      break;
    }
    case 27: {
      // UItoFP
      float dummy = (float) ((unsigned int) x);
      newValueUnsigned = dummy > 0.0f;
      printf("UItoFP\n");
      break;
    }
    case 28: {
      // Sign extend
      int64_t extended = x;
      newValue = extended > 22;
      printf("sign extend\n");
      break;
    }
    case 29: {
      // zero extend
      uint64_t extended = (unsigned int) x;
      newValueUnsigned = extended > 22;
      printf("zero extend\n");
      break;
    }
    case 30: {
      // Use x as an index
      int havoc = 0;
      klee_make_symbolic(&havoc, sizeof(int), "havoc");
      char a[2] = { (char) havoc, (char) havoc };
      // Testing that we can use x as an index.
      if (x < 1 && x >= 0) {
        newValue = a[x] > 5;
      }
      break;
    }
    case 31: {
      // ExtractExpr
      char small_value = 0;
      // Trigger truncate llvm instruction. KLEE should build
      // an ExtractExpr for this.
      small_value = (int8_t) x;
      newValue = small_value > 0;
      break;
    }
    case 32: {
      // EqExpr
      int havoc = 0;
      klee_make_symbolic(&havoc, sizeof(int), "havoc2");
      newValue = (x == havoc);
      printf("EqExpr\n");
    }
    case 33: {
      // SelectExpr
      int havoc = 0;
      klee_make_symbolic(&havoc, sizeof(int), "havoc3");
      newValue = havoc ? x : 0;
      printf("SelectExpr\n");
    }
    // FIXME: We need to test ConcatExpr and ReadExpr but there isn't
    // an obvious way to do that.
    default:
      break;
  }

  // Use expression in condition so that we force a call to the solver
  if (newValue > 0) {
    sink += 1;
  }

  if (newValueUnsigned > 5) {
    sink += 1;
  }

  // Prevent removal of unused values
  return sink;
}
