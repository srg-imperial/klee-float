// Compile as:
// ```
// clang++ x87_fp80_vs_APFloat.cpp -Wall $(llvm-config --cxxflags) -L$(llvm-config --libdir) $(llvm-config --libs Support) $(llvm-config --system-libs Support) -O0
// ```
// Or with `g++`.
// NOTE: Be very careful without `-O0` at the end Clang produces a binary that behaves differently.
// It's probably doing constant folding with it's buggy APFloat...
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"
#include <fenv.h>
#include <inttypes.h>
#include <math.h>
#include <string.h> // For memcpy(), memset()
#include <stdlib.h>

#ifndef __x86_64__
#error Needs to be built on x86-64
#endif

long double MakeX86FP80FromBits(uint16_t high, uint64_t low) {
  long double value = 0.0l;
  memset(&value, 0, sizeof(value));
  // Copy first 64-bits in
  memcpy(&value, &low, 8);
  // Copy remaning 16-bits
  memcpy(((uint8_t*) (&value)) + 8, &high, 2);
  return value;
}

llvm::APFloat MakeLLVMAPFloatFromBits(uint16_t high, uint64_t low) {
  uint64_t temp[] = { low, (uint64_t) high};
  llvm::APInt ApBits = llvm::APInt(/*numBits=*/80, temp);
  llvm::errs() << "MakeLLVMAPFloatFromBits: 0x"
               << ApBits.toString(/*radix=*/16, /*signed=*/false) << "\n";
  return llvm::APFloat(llvm::APFloat::x87DoubleExtended, ApBits);
}

llvm::APFloat MakeLLVMAPFloatFromLongDouble(long double ld) {
  uint64_t temp[] = { 0, 0 };
  memcpy(temp, &ld, 10);
  llvm::APInt ApBits = llvm::APInt(/*numBits=*/80, temp);
  llvm::errs() << "MakeLLVMAPFloatFromBits: 0x"
               << ApBits.toString(/*radix=*/16, /*signed=*/false) << "\n";
  return llvm::APFloat(llvm::APFloat::x87DoubleExtended, ApBits);
}

long double MakeLongDoubleFromLLVMAPFloat(llvm::APFloat apf) {
  if (&(apf.getSemantics()) != &(llvm::APFloat::x87DoubleExtended)) {
    llvm::errs() << "Wrong semantics\n";
    abort();
  }
  llvm::APInt apint = apf.bitcastToAPInt();
  if (apint.getBitWidth() != 80) {
    llvm::errs() << "Wrong width\n";
    abort();
  }
  long double value = 0.0l;
  memcpy(&value, apint.getRawData(), sizeof(value));
  return value;
  
}

void ClearExceptions() {
  bool failed = feclearexcept(FE_ALL_EXCEPT);
  if (failed) {
    llvm::errs() << "Failed to clear exceptions\n";
    abort();
  }
}

void CheckInvalidOperationException() {
  int fired = fetestexcept(FE_INVALID);
  if (fired) {
    llvm::errs() << "Invalid operation exception raised\n";
  }
}

llvm::APFloat::cmpResult LLVMAPFloatCompare(llvm::APFloat &lhs, llvm::APFloat &rhs) {
  llvm::APFloat::cmpResult result = lhs.compare(rhs);
  llvm::errs() << "lhs ";
  switch (result) {
    #define CASE(X) \
      case llvm::APFloat::X: \
        llvm::errs() << #X ; \
        break;
    CASE(cmpLessThan)
    CASE(cmpEqual)
    CASE(cmpGreaterThan)
    CASE(cmpUnordered)
#undef CASE
    default:
      llvm::errs() << "Unhandled case\n";
      abort();
  }
  llvm::errs() << " rhs\n";
  return result;
}

const char* LLVMAPFloatOpToString(llvm::APFloat::opStatus status) {
  switch(status) {
    #define CASE(X) \
      case llvm::APFloat::X: \
        return #X ; \
        break;
      CASE(opOK)
      CASE(opInvalidOp)
      CASE(opDivByZero)
      CASE(opOverflow)
      CASE(opUnderflow)
      CASE(opInexact)
#undef CASE
      default:
        llvm::errs() << "Unhandled case\n";
        abort();
      
  }
}

const char* fpclassifyString(long double ld) {
  switch(fpclassify(ld)) {
    #define CASE(X) \
      case X: \
        return #X;
    CASE(FP_NAN)
    CASE(FP_INFINITE)
    CASE(FP_ZERO)
    CASE(FP_SUBNORMAL)
    CASE(FP_NORMAL)
#undef CASE
      default:
    return "Unknown classification!";
  }
}

#define CHECK(X) \
  result = X ;\
  if (!result) llvm::errs() << "[FAILURE] "; \
  llvm::errs() << "Condition:" #X << ": " << ((result)?"true":"false") << "\n";


void banner() {
  llvm::errs() << "**********************************************************"
                  "**********************\n";
}

void CheckBitsAreEqual(long double a, long double b) {
  uint64_t aBits[] = { 0, 0};
  uint64_t bBits[] = { 0, 0};
  memcpy(aBits, &a, 10); // 80 bits
  memcpy(bBits, &b, 10); // 80 bits
  bool result = false;
  CHECK(aBits[0] == bBits[0]);
  bool oldResult = result;
  CHECK(aBits[1] == bBits[1]);
  if (!result || !oldResult) {
    printf("a: 0x%.4" PRIx16 " %.16" PRIx64 "\n", (uint16_t) aBits[1], aBits[0]);
    printf("b: 0x%.4" PRIx16 " %.16" PRIx64 "\n", (uint16_t) bBits[1], bBits[0]);
  }
}

void PositivePseudoNaNsQuiet()
{
  // Positive Pseuduo-NaNs quiet
  const uint64_t lowBits = 0x7fffffffffffffff;
  const uint16_t highBits = 0x7fff;
  bool result = false;
  llvm::errs() << "Quiet Positive Pseudo-NaNs\n";
  llvm::errs() << "Native:\n";
  ClearExceptions();
  long double ldv = MakeX86FP80FromBits(highBits, lowBits);
  printf("printf: %Lf\n", ldv);
  CHECK(ldv != ldv);
  bool nativeCompareNotEqual = result;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(!isnanl(ldv)); // BUG in glibc?: This should probably be false even though glibc says true!
  CheckInvalidOperationException();
  ClearExceptions();

  llvm::errs() << "Performing addition:\n";
  long double nativeAdd = ldv + 1.0l;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isnanl(nativeAdd));
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "\n";

  llvm::errs() << "APFloat:\n";
  llvm::APFloat ldvLLVMAPFloat =
      MakeLLVMAPFloatFromBits(highBits, lowBits);

  llvm::APFloat::cmpResult cmpResult =
      LLVMAPFloatCompare(ldvLLVMAPFloat, ldvLLVMAPFloat);
  bool apFloatCompareNotEqual = (cmpResult != llvm::APFloat::cmpEqual);

  CHECK(apFloatCompareNotEqual == nativeCompareNotEqual);

  CHECK(!ldvLLVMAPFloat.isNaN()); // BUG?: This isn't a IEEE754 NaN.
  CHECK(!ldvLLVMAPFloat.isSignaling());
  llvm::errs() << "Performing addition:\n";
  llvm::APFloat oneLLVMAPFloat = MakeLLVMAPFloatFromLongDouble(1.0l);
  llvm::APFloat::opStatus opStat =
      ldvLLVMAPFloat.add(oneLLVMAPFloat, llvm::APFloat::rmNearestTiesToEven);
  llvm::errs() << "Operation status:" << LLVMAPFloatOpToString(opStat) << "\n";
  // BUG: Even though this is a quiet NaN because
  // `ldvLLVMAPFloat` is an invalid operand an invalid operation exception
  // should be raised.
  CHECK(opStat == llvm::APFloat::opInvalidOp);
  CHECK(ldvLLVMAPFloat.isNaN());

  // Check their bit representation
  long double ldvLLMAPFloatAsNative = MakeLongDoubleFromLLVMAPFloat(ldvLLVMAPFloat);
  // BUG? : This fails but given the operands were invalid I think it's reasonable
  // for the results to differ. It would be nice if we matched the hardware behaviour
  // though.
  // ldvLLMAPFloatAsNative: 0x7fff 7fffffffffffffff (+ve pseudo quiet NaN)
  // nativeAdd:             0xffff c000000000000000 (-ve quiet NaN)
  CheckBitsAreEqual(ldvLLMAPFloatAsNative, nativeAdd);
}

void PositivePseudoNaNsSignaling()
{
  // Positive Pseudo-NaNs Signaling
  const uint64_t lowBits = 0x3fffffffffffffff;
  const uint16_t highBits = 0x7fff;
  bool result = false;
  banner();
  llvm::errs() << "Signaling Positive Pseudo-NaNs\n";
  llvm::errs() << "Native:\n";
  ClearExceptions();
  long double ldv = MakeX86FP80FromBits(highBits, lowBits);
  printf("printf: %Lf\n", ldv);

  // Compare equal to itself.
  CHECK(ldv != ldv);
  bool nativeCompareNotEqual = result;
  CheckInvalidOperationException();
  ClearExceptions();
  // Is a NaN?
  CHECK(!isnanl(ldv)); // BUG in glibc? This should probably be false even though glibc says true!
  CheckInvalidOperationException();
  ClearExceptions();

  llvm::errs() << "Performing addition:\n";
  // Add 1
  long double nativeAdd = ldv + 1.0l;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isnanl(nativeAdd));
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "\n";

  llvm::errs() << "APFloat:\n";
  llvm::APFloat ldvLLVMAPFloat =
      MakeLLVMAPFloatFromBits(highBits, lowBits);

  // Compare equal to itself.
  llvm::APFloat::cmpResult cmpResult =
      LLVMAPFloatCompare(ldvLLVMAPFloat, ldvLLVMAPFloat);
  bool apFloatCompareNotEqual = (cmpResult != llvm::APFloat::cmpEqual);

  CHECK(apFloatCompareNotEqual == nativeCompareNotEqual);

  // Is a NaN
  CHECK(ldvLLVMAPFloat.isNaN()); // BUG? This isn't a IEEE754 NaN.
  CHECK(ldvLLVMAPFloat.isSignaling()); // BUG? This isn't a IEEE754 signaling NaN.
  // Add 1
  llvm::errs() << "Performing addition:\n";
  llvm::APFloat oneLLVMAPFloat = MakeLLVMAPFloatFromLongDouble(1.0l);
  llvm::APFloat::opStatus opStat =
      ldvLLVMAPFloat.add(oneLLVMAPFloat, llvm::APFloat::rmNearestTiesToEven);
  llvm::errs() << "Operation status:" << LLVMAPFloatOpToString(opStat) << "\n";
  // BUG:`ldvLLVMAPFloat` is an invalid operand an invalid operation
  // exception should be raised.
  CHECK(opStat == llvm::APFloat::opInvalidOp);
  CHECK(ldvLLVMAPFloat.isNaN());

  // Check their bit representation
  long double ldvLLMAPFloatAsNative = MakeLongDoubleFromLLVMAPFloat(ldvLLVMAPFloat);
  // BUG? : This fails but given the operands were invalid I think it's reasonable
  // for the results to differ. It would be nice if we matched the hardware behaviour
  // though.
  // ldvLLMAPFloatAsNative: 0x7fff 3fffffffffffffff (+ve pseudo signaling NaN)
  // nativeAdd:             0xffff c000000000000000 (-ve quiet NaN)
  CheckBitsAreEqual(ldvLLMAPFloatAsNative, nativeAdd);
}

void PositivePseudoInfinity()
{
  // Positive Pseudo-infinity
  const uint64_t lowBits = 0x0000000000000000;
  const uint16_t highBits = 0x7fff;
  bool result = false;
  banner();
  llvm::errs() << "Positive Pseudo-infinity\n";
  llvm::errs() << "Native:\n";
  ClearExceptions();
  long double ldv = MakeX86FP80FromBits(highBits, lowBits);
  printf("printf: %Lf\n", ldv);

  // Compare equal to itself.
  CHECK(ldv != ldv);
  bool nativeCompareNotEqual = result;
  CheckInvalidOperationException();
  ClearExceptions();
  // Is inf?
  CHECK(isinfl(ldv) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  // Is nan?
  CHECK(!isnanl(ldv)); // BUG in glibc? This should probably be false even though glibc says true!

  CheckInvalidOperationException();
  ClearExceptions();

  llvm::errs() << "Performing addition:\n";
  // Add 1
  long double nativeAdd = ldv + 1.0l;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isinfl(nativeAdd) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "\n";

  llvm::errs() << "APFloat:\n";
  llvm::APFloat ldvLLVMAPFloat =
      MakeLLVMAPFloatFromBits(highBits, lowBits);

  // Compare equal to itself.
  llvm::APFloat::cmpResult cmpResult =
      LLVMAPFloatCompare(ldvLLVMAPFloat, ldvLLVMAPFloat);
  bool apFloatCompareNotEqual = (cmpResult != llvm::APFloat::cmpEqual);

  CHECK(apFloatCompareNotEqual == nativeCompareNotEqual);

  // Is inf?
  CHECK(!ldvLLVMAPFloat.isInfinity());
  CHECK(!ldvLLVMAPFloat.isNaN()); // BUG?: This is not a IEEE754 NaN.
  // Add 1
  llvm::errs() << "Performing addition:\n";
  llvm::APFloat oneLLVMAPFloat = MakeLLVMAPFloatFromLongDouble(1.0l);
  llvm::APFloat::opStatus opStat =
      ldvLLVMAPFloat.add(oneLLVMAPFloat, llvm::APFloat::rmNearestTiesToEven);
  llvm::errs() << "Operation status:" << LLVMAPFloatOpToString(opStat) << "\n";
  // BUG:`ldvLLVMAPFloat` is an invalid operand an invalid operation
  // exception should be raised.
  CHECK(opStat == llvm::APFloat::opInvalidOp);

  // Check their bit representation
  long double ldvLLMAPFloatAsNative = MakeLongDoubleFromLLVMAPFloat(ldvLLVMAPFloat);
  // BUG? : This fails but given the operands were invalid I think it's reasonable
  // for the results to differ. It would be nice if we matched the hardware behaviour
  // though.
  // ldvLLMAPFloatAsNative: 0x7fff 0000000000000000 (+ve pseudo infinity)
  // nativeAdd:             0xffff c000000000000000 (-ve quiet NaN)
  CheckBitsAreEqual(ldvLLMAPFloatAsNative, nativeAdd);
}

void PositiveUnnormal()
{
  // Positive un-normal (not the same as denormal)
  const uint64_t lowBits = 0x0000000000000001;
  const uint16_t highBits = 0x0001;
  bool result = false;
  banner();
  llvm::errs() << "Positive unnormal\n";
  llvm::errs() << "Native:\n";
  ClearExceptions();
  long double ldv = MakeX86FP80FromBits(highBits, lowBits);
  printf("printf: %Lf\n", ldv);

  // Compare equal to itself.
  CHECK(ldv != ldv);
  bool nativeCompareNotEqual = result;
  CheckInvalidOperationException();
  ClearExceptions();
  // Is inf?
  CHECK(isinfl(ldv) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  // Is nan?
  CHECK(!isnanl(ldv));
  CheckInvalidOperationException();
  ClearExceptions();

  llvm::errs() << "Performing addition:\n";
  // Add 1
  long double nativeAdd = ldv + 1.0l;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isinfl(nativeAdd) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isnanl(nativeAdd)); // Expected?
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "\n";

  llvm::errs() << "APFloat:\n";
  llvm::APFloat ldvLLVMAPFloat =
      MakeLLVMAPFloatFromBits(highBits, lowBits);

  // Compare equal to itself.
  // BUG: Invalid operand (unnormal) shouldn't compare equal to itself.
  llvm::APFloat::cmpResult cmpResult =
      LLVMAPFloatCompare(ldvLLVMAPFloat, ldvLLVMAPFloat);
  bool apFloatCompareNotEqual = (cmpResult != llvm::APFloat::cmpEqual);

  CHECK(apFloatCompareNotEqual == nativeCompareNotEqual);

  // Is inf?
  CHECK(!ldvLLVMAPFloat.isInfinity());
  CHECK(!ldvLLVMAPFloat.isNaN());
  // Add 1
  llvm::errs() << "Performing addition:\n";
  llvm::APFloat oneLLVMAPFloat = MakeLLVMAPFloatFromLongDouble(1.0l);

  llvm::APFloat::opStatus opStat =
      ldvLLVMAPFloat.add(oneLLVMAPFloat, llvm::APFloat::rmNearestTiesToEven);

  llvm::errs() << "Operation status:" << LLVMAPFloatOpToString(opStat) << "\n";

  // BUG:`ldvLLVMAPFloat` is an invalid operand an invalid operation
  // exception should be raised.
  CHECK(opStat == llvm::APFloat::opInvalidOp);

  // Check their bit representation
  long double ldvLLMAPFloatAsNative = MakeLongDoubleFromLLVMAPFloat(ldvLLVMAPFloat);
  printf("ldvLLMAPFloatAsNative: %Lf\n", ldvLLMAPFloatAsNative);
  // BUG? : This fails but given the operands were invalid I think it's reasonable
  // for the results to differ. It would be nice if we matched the hardware behaviour
  // though.
  // ldvLLMAPFloatAsNative: 0x3fff 8000000000000000 (+ve normal)
  // nativeAdd:             0xffff c000000000000000 (-ve quiet NaN)
  CheckBitsAreEqual(ldvLLMAPFloatAsNative, nativeAdd);
}

void PositivePseudoDenormal()
{
  // Positive pseudo denormal.
  // On x86_64 it seems (at least for this value) that these "unsupported" values
  // are handled.
  // 8.2.2 says
  // ```
  // Beginning with the Intel 387 math coprocessor, the encodings formerly known
  // as pseudo-denormal numbers are
  // not generated by IA-32 processors. When encountered as operands, however,
  // they are handled correctly; that is,
  // they are treated as denormals and a denormal exception is generated.
  // Pseudo-denormal numbers should not be
  // used as operand values. They are supported by current IA-32 processors (as
  // described here) to support legacy
  // code.
  // ```
  //
  // The text above doesn't say anything about x86_64 but it looks like it too
  // treats these as denormals.
  const uint64_t lowBits = 0x8000000000000001;
  const uint16_t highBits = 0x0000;
  bool result = false;
  banner();
  llvm::errs() << "Positive pseudo denormal\n";
  llvm::errs() << "Native:\n";
  ClearExceptions();
  long double ldv = MakeX86FP80FromBits(highBits, lowBits);
  printf("printf: %Lf\n", ldv);

  // Compare equal to itself.
  CHECK(ldv == ldv);
  bool nativeCompareEqual = result;
  CheckInvalidOperationException();
  ClearExceptions();
  // Is inf?
  CHECK(isinfl(ldv) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  // Is nan?
  CHECK(isnanl(ldv) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(ldv != 0.0l);
  CheckInvalidOperationException();
  ClearExceptions();
  // Is denormal?
  CHECK(fpclassify(ldv) == FP_SUBNORMAL); // BUG in glibc? Not sure what this should be classifed as.
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "ldv classified as " << fpclassifyString(ldv) << "\n";

  llvm::errs() << "Performing addition:\n";
  // Add 1
  long double nativeAdd = ldv + 1.0l;
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(isinfl(nativeAdd) == 0);
  CheckInvalidOperationException();
  ClearExceptions();
  CHECK(!isnanl(nativeAdd));
  CheckInvalidOperationException();
  ClearExceptions();
  llvm::errs() << "\n";

  llvm::errs() << "APFloat:\n";
  llvm::APFloat ldvLLVMAPFloat =
      MakeLLVMAPFloatFromBits(highBits, lowBits);

  // Compare equal to itself.
  llvm::APFloat::cmpResult cmpResult =
      LLVMAPFloatCompare(ldvLLVMAPFloat, ldvLLVMAPFloat);
  bool apFloatCompareEqual = (cmpResult == llvm::APFloat::cmpEqual);

  CHECK(apFloatCompareEqual == nativeCompareEqual);

  // Is inf?
  CHECK(!ldvLLVMAPFloat.isInfinity());
  CHECK(!ldvLLVMAPFloat.isNaN());
  // Add 1
  llvm::errs() << "Performing addition:\n";
  llvm::APFloat oneLLVMAPFloat = MakeLLVMAPFloatFromLongDouble(1.0l);

  llvm::APFloat::opStatus opStat =
      ldvLLVMAPFloat.add(oneLLVMAPFloat, llvm::APFloat::rmNearestTiesToEven);

  llvm::errs() << "Operation status:" << LLVMAPFloatOpToString(opStat) << "\n";

  CHECK(opStat != llvm::APFloat::opInvalidOp);

  // Check their bit representation
  long double ldvLLMAPFloatAsNative = MakeLongDoubleFromLLVMAPFloat(ldvLLVMAPFloat);
  CheckBitsAreEqual(ldvLLMAPFloatAsNative, nativeAdd);
}

int main(int argc, char** argv) {
  // Try an example from each class from "8.2.2 Unsupported Double
  // Extended-Precision Float-Point Encodings and Pseudo-Denormals" from the
  // Intel(R) 64 and IA-32 Architectures Software Developer's Manual.

  /*
    ```
      The double extended-precision floating-point format permits many
encodings that do not fall into any of the categories shown in Table 4-3.
Table 8-3 shows these unsupported encodings. Some of these encodings were
supported by the Intel 287 math coprocessor; however, most of them are not
supported by the Intel 387 math coprocessor and later IA-32 processors. These
encodings are no longer supported due to changes made in the final version of
IEEE Standard 754 that eliminated these encodings.

      Specifically, the categories of encodings formerly known as pseudo-NaNs,
pseudo-infinities, and un-normal numbers are not supported and should not be
used as operand values. The Intel 387 math coprocessor and later IA-32
processors generate an invalid-operation exception when these encodings are
encountered as operands.

      Beginning with the Intel 387 math coprocessor, the encodings formerly
known as pseudo-denormal numbers are not generated by IA-32 processors. When
encountered as operands, however, they are handled correctly; that is, they are
treated as denormals and a denormal exception is generated.  Pseudo-denormal
numbers should not be used as operand values. They are supported by current
IA-32 processors (as described here) to support legacy code.
  ```
  */
  banner();
  PositivePseudoNaNsQuiet();
  banner();
  PositivePseudoNaNsSignaling();
  banner();
  PositivePseudoInfinity();
  banner();
  PositiveUnnormal();
  banner();
  PositivePseudoDenormal();

  // TODO: Check negative variants of the above.

  return 0;
}
