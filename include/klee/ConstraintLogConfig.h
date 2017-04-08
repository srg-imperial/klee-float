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
  enum ConstraintLogConfigKind { CLCK_Z3 };

private:
  const ConstraintLogConfigKind kind;

public:
  ConstraintLogConfigKind getKind() const { return kind; }
  // Allocate memory for a copy of this configuration.
  // Clients must free using `delete`.
  virtual ConstraintLogConfig *alloc() const = 0;
  virtual ~ConstraintLogConfig() {}

protected:
  ConstraintLogConfig(ConstraintLogConfigKind k) : kind(k) {}
};

class Z3ConstraintLogConfig : public ConstraintLogConfig {
public:
  // Configuration settings
  bool ackermannizeArrays;
  bool useToIEEEBVFunction;

  Z3ConstraintLogConfig()
      : ConstraintLogConfig(CLCK_Z3), ackermannizeArrays(false),
        useToIEEEBVFunction(true) {}
  static bool classof(const ConstraintLogConfig *clc) {
    return clc->getKind() == CLCK_Z3;
  }

  virtual ConstraintLogConfig *alloc() const {
    ConstraintLogConfig *clc = new Z3ConstraintLogConfig(*this);
    return clc;
  }
};
}

#endif /* CONSTRAINT_LOG_CONFIG_H */
