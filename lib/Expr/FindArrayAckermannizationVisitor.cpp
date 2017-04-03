//===-- FindArrayAckermannizationVisitor.cpp --------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "FindArrayAckermannizationVisitor.h"
#include "llvm/ADT/ArrayRef.h"

namespace klee {

ArrayAckermannizationInfo::ArrayAckermannizationInfo()
    : contiguousMSBitIndex(0), contiguousLSBitIndex(0) {}

const Array *ArrayAckermannizationInfo::getArray() const {
  if (ReadExpr *re = dyn_cast<ReadExpr>(toReplace)) {
    return re->updates.root;
  } else if (ConcatExpr *ce = dyn_cast<ConcatExpr>(toReplace)) {
    assert(ce->getKid(0)->getKind() == Expr::Read &&
           "left child must be a ReadExpr");
    ReadExpr *re = dyn_cast<ReadExpr>(ce->getKid(0));
    return re->updates.root;
  }
  llvm_unreachable("Unhandled expr type");
  return NULL;
}

bool ArrayAckermannizationInfo::isWholeArray() const {
  const Array *theArray = getArray();
  unsigned bitWidthOfArray = theArray->size * theArray->range;
  assert(contiguousMSBitIndex > contiguousLSBitIndex);
  if (bitWidthOfArray == getWidth()) {
    return true;
  }
  return false;
}

unsigned ArrayAckermannizationInfo::getWidth() const {
  return (contiguousMSBitIndex - contiguousLSBitIndex) + 1;
}

void ArrayAckermannizationInfo::dump() const {
  llvm::errs() << "isWholeArray: " << isWholeArray() << "\n";
  llvm::errs() << "contiguousMSBitIndex:" << contiguousMSBitIndex << "\n";
  llvm::errs() << "contiguousLSBitIndex:" << contiguousLSBitIndex << "\n";
  llvm::errs() << "width:" << getWidth() << "\n";
  llvm::errs() << "toReplace:\n" << toReplace << "\n";
}

bool ArrayAckermannizationInfo::overlapsWith(ArrayAckermannizationInfo& other) const {
  // right edge of other inside this
  if (other.contiguousLSBitIndex >= this->contiguousLSBitIndex &&
      other.contiguousLSBitIndex <= this->contiguousMSBitIndex) {
    return true;
  }
  // left edge of other inside this
  if (other.contiguousMSBitIndex >= this->contiguousLSBitIndex &&
      other.contiguousMSBitIndex <= this->contiguousMSBitIndex) {
    return true;
  }
  return false;
}

ref<Expr> ArrayAckermannizationInfo::getReplacement() const {
  const Array *theArray = getArray();
  // Should be replaced with a plain variable
  if (theArray->isSymbolicArray())
    return ref<Expr>(NULL);

  // Replacement is read from a constant array. Make the equivilent
  // ConstantExpr.
  assert(theArray->isConstantArray() && "array should be constant");
  assert((64 % theArray->range) == 0 &&
         "array range must divide exactly into 64");
  size_t chunks = (getWidth() + 63) / 64;
  std::vector<uint64_t> bits(chunks);
  assert(bits.size() == chunks);
  size_t currentConstantArrayIndex =
      (contiguousLSBitIndex % 8); // Start at array read
  for (size_t chunkIndex = 0; chunkIndex < chunks; ++chunkIndex) {
    size_t currentConstantArrayBitsRead = 0;
    uint64_t chunkData = 0;
    while (currentConstantArrayBitsRead < 64 &&
           currentConstantArrayIndex < ((contiguousMSBitIndex + 1) % 8)) {
      ref<ConstantExpr> chunkFragment =
          theArray->constantValues[currentConstantArrayIndex];
      uint64_t value = chunkFragment->getZExtValue();
      assert(currentConstantArrayBitsRead < 64 && "would overshift");
      chunkData |= (value) << currentConstantArrayBitsRead;
      currentConstantArrayBitsRead += chunkFragment->getWidth();
      ++currentConstantArrayIndex;
    }
    bits[chunkIndex] = chunkData;
  }
  // Finally make the APInt
  llvm::APInt largeConstant(
      /*numBits=*/getWidth(),
      /*bigVal=*/llvm::ArrayRef<uint64_t>(bits.data(), bits.size()));

  return ConstantExpr::alloc(largeConstant);
}

FindArrayAckermannizationVisitor::FindArrayAckermannizationVisitor(
    bool recursive, bool ackermannizeConstantArrays)
    : ExprVisitor(recursive),
      ackermannizeConstantArrays(ackermannizeConstantArrays) {}

std::vector<ArrayAckermannizationInfo> *
FindArrayAckermannizationVisitor::getOrInsertAckermannizationInfo(
    const Array *arr, bool *wasInsert) {
  // Try to insert empty info. This will fail if we already have info on this
  // Array and we'll get the existing ArrayAckermannizationInfo instead.
  std::pair<ArrayToAckermannizationInfoMapTy::iterator, bool> pair =
      ackermannizationInfo.insert(
          std::make_pair(arr, std::vector<ArrayAckermannizationInfo>(0)));
  if (wasInsert)
    *wasInsert = pair.second;

  return &((pair.first)->second);
}

/* This method looks for nested concatenated ReadExpr that don't involve
 * updates or constant arrays that are contigous. We look for ConcatExpr
 * unbalanced to the right that operate on the same array. E.g.
 *
 *                  ConcatExpr
 *                 /       \
 *    ReadExpr 3bv32 Ar     \
 *                        ConcatExpr
 *                         /    \
 *           ReadExpr 2bv32 Ar   \
 *                              ConcatExpr
 *                              /        \
 *                             /          \
 *                ReadExpr 1bv32 Ar       ReadExpr 0bv32 Ar
 */
ExprVisitor::Action
FindArrayAckermannizationVisitor::visitConcat(const ConcatExpr &ce) {
  const Array *theArray = NULL;
  std::vector<ArrayAckermannizationInfo> *ackInfos = NULL;
  ArrayAckermannizationInfo ackInfo;
  const ConcatExpr *currentConcat = &ce;
  std::vector<ref<ReadExpr> > reads;
  bool isFirst = true;
  unsigned MSBitIndex = 0;
  unsigned LSBitIndex = 0;
  unsigned widthReadSoFar = 0; // In bits

  // I'm sorry about the use of goto here but without having lambdas (we're
  // using C++03) it's kind of hard to have readable and efficient code that
  // handles the case when we fail to match without using gotos.

  // Try to find the array
  if (ReadExpr *lhsRead = dyn_cast<ReadExpr>(ce.getKid(0))) {
    theArray = lhsRead->updates.root;
    assert(theArray && "theArray cannot be NULL");

    // Try getting existing ArrayAckermannizationInfos
    bool wasInsert = true;
    ackInfos = getOrInsertAckermannizationInfo(theArray, &wasInsert);
    if (!wasInsert && ackInfos->size() == 0) {
      // We've seen this array before and it can't be ackermannized
      goto failedMatch;
    }

    if (!ackermannizeConstantArrays && theArray->isConstantArray()) {
      goto failedMatch;
    }

  } else {
    goto failedMatch;
  }

  // Collect the ordered ReadExprs
  while (true) {
    ref<Expr> lhs = currentConcat->getKid(0);
    ref<Expr> rhs = currentConcat->getKid(1);

    // Lhs must be a ReadExpr
    if (ReadExpr *lhsre = dyn_cast<ReadExpr>(lhs)) {
      reads.push_back(lhsre);
    } else {
      goto failedMatch;
    }

    // Rhs must be a ConcatExpr or ReadExpr
    if (ReadExpr *rhsre = dyn_cast<ReadExpr>(rhs)) {
      reads.push_back(rhsre);
      break; // Hit the right most leaf node.
    } else if (ConcatExpr *rhsconcat = dyn_cast<ConcatExpr>(rhs)) {
      currentConcat = rhsconcat;
      continue;
    }
    goto failedMatch;
  }

  // Go through the ordered reads checking they match the expected pattern
  assert(isFirst);
  for (std::vector<ref<ReadExpr> >::const_iterator bi = reads.begin(),
                                                   be = reads.end();
       bi != be; ++bi) {

    ref<ReadExpr> read = *bi;
    // Check we are looking at the same array that we found earlier
    if (theArray != read->updates.root) {
      goto failedMatch;
    }

    // FIXME: Figure out how to handle updates. For now pretend we can't
    // ackermannize this array
    if (read->updates.head != NULL) {
      goto failedMatch;
    }

    widthReadSoFar += read->getWidth();

    // Check index is constant and that we are doing contiguous reads
    if (ConstantExpr *index = dyn_cast<ConstantExpr>(read->index)) {
      if (!isFirst) {
        // Check we are reading the next region along in the array. This
        // implementation supports ReadExpr of different sizes although
        // currently in KLEE they are always 8-bits (1 byte).
        unsigned difference =
            LSBitIndex - (index->getZExtValue() * read->getWidth());
        if (difference != read->getWidth()) {
          goto failedMatch;
        }
      } else {
        // Compute most significant bit
        // E.g. if index was 2 and width is 8 then this is a byte read
        // but the most significant bit read is not 16, it is 23.
        MSBitIndex = (index->getZExtValue() * read->getWidth()) + (read->getWidth() -1);
      }
      // Record the least significant bit read
      LSBitIndex = index->getZExtValue() * read->getWidth();
    } else {
      goto failedMatch;
    }

    isFirst = false;
  }

  // We found a match
  ackInfo.toReplace = ref<Expr>(const_cast<ConcatExpr *>(&ce));
  ackInfo.contiguousMSBitIndex = MSBitIndex;
  ackInfo.contiguousLSBitIndex = LSBitIndex;
  assert(ackInfo.contiguousMSBitIndex > ackInfo.contiguousLSBitIndex && "bit indicies incorrectly ordered");

  // FIXME: This needs re-thinking. We should allow overlapping regions
  // (especially regions where the regions are completly inside another).
  // `ArrayAckermannizationInfo` needs to be worked to convey this information
  // better. For now disallow overlapping regions.
  for (std::vector<ArrayAckermannizationInfo>::const_iterator
           i = ackInfos->begin(),
           ie = ackInfos->end();
       i != ie; ++i) {
    if (i->overlapsWith(ackInfo)) {
      goto failedMatch;
    }
  }

  ackInfos->push_back(ackInfo);
  // We know the indices are simple constants so need to traverse children
  return Action::skipChildren();

failedMatch :
  // Any empty list of ArrayAckermannizationInfo indicates that the array cannot
  // be ackermannized.
  if (ackInfos)
    ackInfos->clear();
  return Action::doChildren();
}

ExprVisitor::Action
FindArrayAckermannizationVisitor::visitRead(const ReadExpr &re) {
  const Array *theArray = re.updates.root;
  bool wasInsert = true;
  ArrayAckermannizationInfo ackInfo;
  std::vector<ArrayAckermannizationInfo> *ackInfos =
      getOrInsertAckermannizationInfo(theArray, &wasInsert);

  // I'm sorry about the use of goto here but without having lambdas (we're
  // using C++03) it's kind of hard to have readable and efficient code that
  // handles the case when we fail to match without using gotos.

  if (!wasInsert && ackInfos->size() == 0) {
    // We've seen this array before and it can't be ackermannized.
    goto failedMatch;
  }

  if (!ackermannizeConstantArrays && theArray->isConstantArray()) {
    goto failedMatch;
  }

  // FIXME: Figure out how to handle updates to the array. I think we can
  // handle this using bitwise masking and or'ing and shifting. For now pretend
  // that they can't be ackermannized.
  if (re.updates.head != NULL) {
    goto failedMatch;
  }

  if (ConstantExpr *index = dyn_cast<ConstantExpr>(re.index)) {
    ackInfo.contiguousLSBitIndex = index->getZExtValue() * re.getWidth();
    ackInfo.contiguousMSBitIndex = ((index->getZExtValue() + 1) * re.getWidth()) -1;
  } else {
    goto failedMatch;
  }

  // This is an array read without constants values or updates so we
  // can definitely ackermannize this based on what we've seen so far.
  ackInfo.toReplace = ref<Expr>(const_cast<ReadExpr *>(&re));

  // FIXME: This needs re-thinking. We should allow overlapping regions
  // (especially regions where the regions are completly inside another).
  // `ArrayAckermannizationInfo` needs to be worked to convey this information
  // better to Z3SolverImpl. For now disallow overlapping regions.
  for (std::vector<ArrayAckermannizationInfo>::const_iterator
           i = ackInfos->begin(),
           ie = ackInfos->end();
       i != ie; ++i) {
    if (i->overlapsWith(ackInfo)) {
      goto failedMatch;
    }
  }
  ackInfos->push_back(ackInfo);
  return Action::doChildren(); // Traverse index expression

failedMatch :
  // Clear any existing ArrayAckermannizationInfos to indicate that the Array
  // cannot be ackermannized.
  ackInfos->clear();
  return Action::doChildren(); // Traverse index expression
}

void FindArrayAckermannizationVisitor::clear() { ackermannizationInfo.clear(); }

void FindArrayAckermannizationVisitor::dump() const {
  llvm::errs() << "[FindArrayAckermannizationVisitor: "
               << ackermannizationInfo.size() << " arrays]\n";
  for (ArrayToAckermannizationInfoMapTy::const_iterator
           i = ackermannizationInfo.begin(),
           e = ackermannizationInfo.end();
       i != e; ++i) {
    llvm::errs() << "Array:" << i->first->name << "\n"
                 << i->second.size() << " array ackermannization info(s)"
                 << "\n";
    for (std::vector<ArrayAckermannizationInfo>::const_iterator
             iaai = i->second.begin(),
             eaai = i->second.end();
         iaai != eaai; ++iaai) {
      iaai->dump();
    }
  }
}
}
