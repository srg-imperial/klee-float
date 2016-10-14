#!/bin/bash -x

# This script is a bit of a hack. A Dockerfile could
# do this for us.

set -e

# Build KLEE inside the container
IMAGE="${IMAGE:-comsys/klee-dev-fpbench-prebuilt}"
NETWORK_FLAGS="--network=none"
TEMP_NAME="klee_afr_temp"
NEW_IMAGE_NAME="local_klee_afr"
WHOLE_PROGRAM_LLVM_TEMP_DIR="$(pwd)/temp_whole_program_llvm"

# Used patched makefile so we don't build with STP.
docker kill ${TEMP_NAME} || echo "${TEMP_NAME} isn't running"
docker rm ${TEMP_NAME} || echo "${TEMP_NAME} doesn't exist"
docker run --rm --name=${TEMP_NAME} -ti ${NETWORK_FLAGS} \
  -v `pwd`:/home/user/klee \
  -v `pwd`/scripts/container.Makefile:/home/user/makefile \
  ${IMAGE} \
  /bin/bash -c 'cd /home/user && make klee'


# Build a new image from the build
# First delete the existing KLEE stuff
docker run --detach ${NETWORK_FLAGS} --name=${TEMP_NAME} ${IMAGE} tail -f /proc/uptime
sleep 2
docker exec -ti ${TEMP_NAME} rm -rf /home/user/klee

# Grab whole-program-llvm
git clone --depth 1 https://github.com/travitch/whole-program-llvm.git ${WHOLE_PROGRAM_LLVM_TEMP_DIR}
docker cp ${WHOLE_PROGRAM_LLVM_TEMP_DIR} ${TEMP_NAME}:/home/user/whole-program-llvm
docker exec -ti ${TEMP_NAME} "/bin/bash" "-c" "echo \"PATH=\$PATH:/home/user/whole-program-llvm\" >> /home/user/.bashrc"

# Copy in what was just built
docker cp `pwd`/ ${TEMP_NAME}:/home/user/klee

# Copy in our patched makefile
docker cp `pwd`/scripts/container.Makefile ${TEMP_NAME}:/home/user/klee/makefile

# Fix permissions
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/whole-program-llvm
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/klee
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/.bashrc
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown user: /home/user/makefile

rm -rf ${WHOLE_PROGRAM_LLVM_TEMP_DIR}

docker kill ${TEMP_NAME}
docker commit ${TEMP_NAME} ${NEW_IMAGE_NAME}
docker rm ${TEMP_NAME}
