FROM klee-dev-fpbench-patched-squashed
ARG container_username=user
ARG klee_git_hash
LABEL klee_git_hash=${klee_git_hash}
USER ${container_username}
WORKDIR /home/${container_username}/
CMD [ "/bin/sh", "-c", "/usr/bin/bash -l" ]
