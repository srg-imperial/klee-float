#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(bc-stats
  main.cpp
)

set(LLVM_COMPONENTS
  core
  bitreader
  support
)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(bc-stats PRIVATE ${LLVM_LIBS})
target_compile_options(bc-stats PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(bc-stats PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_include_directories(bc-stats PRIVATE ${KLEE_COMPONENT_EXTRA_INCLUDE_DIRS})
