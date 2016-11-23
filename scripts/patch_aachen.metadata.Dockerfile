FROM klee-dev-fpbench-patched-squashed
ARG container_username=user
ARG klee_git_hash
LABEL klee_git_hash=${klee_git_hash}
USER ${container_username}
WORKDIR /home/${container_username}/
# For wllvm
ENV LLVM_COMPILER=clang
# For fp-bench
ENV KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/
ENV KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/lib/
CMD [ "/bin/sh", "-c", "/usr/bin/bash -l" ]
