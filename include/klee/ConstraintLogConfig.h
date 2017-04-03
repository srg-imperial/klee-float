//===-- ConstraintLogConfig.h -----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef CONSTRAINT_LOG_CONFIG_H
#define CONSTRAINT_LOG_CONFIG_H
#include "klee/Config/config.h"
#include "llvm/Support/Casting.h"

namespace klee {

class ConstraintLogConfig {
public:
  enum ConstraintLogConfigKind { CLCK_TOP, CLCK_Z3 };

private:
  const ConstraintLogConfigKind kind;

public:
  ConstraintLogConfigKind getKind() const { return kind; }
  ConstraintLogConfig() : kind(CLCK_TOP){};
  static bool classof(const ConstraintLogConfig *clc) { return true; }

protected:
  ConstraintLogConfig(ConstraintLogConfigKind k) : kind(k) {}
};

class Z3ConstraintLogConfig : public ConstraintLogConfig {
public:
  Z3ConstraintLogConfig() : ConstraintLogConfig(CLCK_Z3) {}
  static bool classof(const ConstraintLogConfig *clc) {
    return clc->getKind() == CLCK_Z3;
  }
  // Configuration settings
  bool ackermannizeArrays = false;
};
}

#endif /* CONSTRAINT_LOG_CONFIG_H */
