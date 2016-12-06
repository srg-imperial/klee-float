// Build me with Clang built with debug symbols at -O2.
#include <stdint.h>
#include <stdio.h>

int main() {
  // These special constants are part of the "Positive Floating point
  // Unnormals" as defined in 8.2.2 "Unsupported Double Extended-Precision
  // Floating-Point Encodings and Pseduo-Denormals" of the Intel(R) 64 and IA-32
  // Architectures Software Developer's Manual.
  //
  //                              Significand
  // Sign Exponent             Integer Fraction
  // [0]  [111 1111 1111 1110] [1]     [ 63 ones ]
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
