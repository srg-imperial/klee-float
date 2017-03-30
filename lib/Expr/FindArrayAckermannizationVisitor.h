//===-- FindArrayAckermannizationVisitor.h ----------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_FIND_ARRAY_ACKERMANNIZATION_VISITOR_H
#define KLEE_FIND_ARRAY_ACKERMANNIZATION_VISITOR_H

#include "klee/util/ExprHashMap.h"
#include "klee/util/ExprVisitor.h"
#include <map>

namespace klee {

struct ArrayAckermannizationInfo {
  ExprHashSet toReplace;
  unsigned contiguousMSBitIndex;
  unsigned contiguousLSBitIndex;
  unsigned getWidth() const;
  const Array *getArray() const;
  bool isWholeArray() const;
  ArrayAckermannizationInfo();
  void dump() const;
  bool overlapsWith(ArrayAckermannizationInfo& other) const;
  bool hasSameBounds(ArrayAckermannizationInfo &other) const;
  bool containsByte(unsigned offset) const;
  bool containsBit(unsigned offset) const;
};

/// This class looks for opportunities to perform ackermannization (ackermann's
/// reduction) of array reads.  The reduction here is transforming array reads
/// into uses of bitvector variables. Note this visitor doesn't actually
/// modify the expressions given to it, instead it just looks for
/// opportunities to apply the reduction.
class FindArrayAckermannizationVisitor : public ExprVisitor {
public:
  FindArrayAckermannizationVisitor(bool recursive);
  void clear();

  // FIXME: Should we hide this behind an interface?
  /// The recorded Array's that can be ackermannized.
  /// If an Array maps to empty vector then the detected array cannot be
  /// ackermannized.
  /// If an Array maps to a non-empty vector then that vector gives all the
  /// expressions
  /// that can be ackermannized.
  typedef std::map<const Array *, std::vector<ArrayAckermannizationInfo> >
      ArrayToAckermannizationInfoMapTy;
  ArrayToAckermannizationInfoMapTy ackermannizationInfo;
  std::vector<ArrayAckermannizationInfo> *
  getOrInsertAckermannizationInfo(const Array *, bool *wasInsert);
  void dump() const;

protected:
  Action visitConcat(const ConcatExpr &);
  Action visitRead(const ReadExpr &);
};
}

#endif
