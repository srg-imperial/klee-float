// Build me with Clang built with debug symbols at -O2.
#include <stdint.h>
#include <stdio.h>

int main() {
  const uint64_t lowBits = 0x7fffffffffffffff;
  const uint16_t highBits = 0x7ffe;
  long double v = 0.0l;
  uint64_t* vptr = (uint64_t*) &v;
  *vptr = lowBits;
  uint16_t* vhighptr = (uint16_t*) ( ((uint8_t*) vptr) + 8 );
  *vhighptr = highBits;

  // Constant folding this hits an assert in llvm::APFloat.
  long double result = v + 1;
  return result;
}
