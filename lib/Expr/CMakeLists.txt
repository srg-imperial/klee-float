#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleaverExpr
  APFloatEval.cpp
  ArrayCache.cpp
  Assigment.cpp
  Constraints.cpp
  ExprBuilder.cpp
  Expr.cpp
  ExprEvaluator.cpp
  ExprPPrinter.cpp
  ExprSMTLIBPrinter.cpp
  ExprUtil.cpp
  ExprVisitor.cpp
  FindArrayAckermannizationVisitor.cpp
  Lexer.cpp
  Parser.cpp
  Updates.cpp
)

set(LLVM_COMPONENTS
  support
)
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
# FIXME: Refactor some of this x87 fp80 stuff out into kleeSupport so we don't need to depend on kleeSupport.
target_link_libraries(kleaverExpr PUBLIC kleeSupport ${LLVM_LIBS})
