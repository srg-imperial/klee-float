#!/bin/bash -x

# This script is a bit of a hack. A Dockerfile could
# do this for us.

set -e

IMAGE="klee-dev-fpbench-patched"
# Patch the image to use the user id on this host
docker build -t ${IMAGE} --build-arg host_user_id="$(id -u)" -f scripts/patch_aachen.Dockerfile .

# Create a base image

# Build KLEE inside the container
TEMP_NAME="klee_afr_temp"
NEW_IMAGE_NAME="local_klee_afr"
WHOLE_PROGRAM_LLVM_TEMP_DIR="$(pwd)/temp_whole_program_llvm"

# Use patched makefile so we don't build with STP.
docker kill ${TEMP_NAME} || echo "${TEMP_NAME} isn't running"
docker rm ${TEMP_NAME} || echo "${TEMP_NAME} doesn't exist"
docker run --rm --user="$(id -u):$(id -g)" --name=${TEMP_NAME} -ti ${NETWORK_FLAGS} \
  -v `pwd`:/home/user/klee \
  -v `pwd`/scripts/container.Makefile:/home/user/makefile \
  ${IMAGE} \
  /bin/bash -c 'cd /home/user && make klee'


# Build a new image from the build
# First delete the existing KLEE stuff
docker run --detach --name=${TEMP_NAME} ${IMAGE} tail -f /proc/uptime
sleep 2
docker exec -ti ${TEMP_NAME} rm -rf /home/user/klee

# Grab whole-program-llvm
docker exec -ti ${TEMP_NAME} git clone --depth 1 https://github.com/travitch/whole-program-llvm.git /home/user/whole-program-llvm
docker exec -ti ${TEMP_NAME} "/bin/bash" "-c" "echo \"PATH=\$PATH:/home/user/whole-program-llvm\" >> /home/user/.bashrc"

# Add stuff needed by fp-bench
docker exec -ti ${TEMP_NAME} "/bin/bash" "-c" "echo \"export KLEE_NATIVE_RUNTIME_INCLUDE_DIR=/home/user/klee/include/\" >> /home/user/.bashrc"
docker exec -ti ${TEMP_NAME} "/bin/bash" "-c" "echo \"export KLEE_NATIVE_RUNTIME_LIB_DIR=/home/user/klee/build/Release+Asserts/lib/\" >> /home/user/.bashrc"
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/pip install jsonschema PyYAML

# Copy in what was just built
docker cp `pwd`/ ${TEMP_NAME}:/home/user/klee

# Copy in our patched makefile
docker cp `pwd`/scripts/container.Makefile ${TEMP_NAME}:/home/user/klee/makefile

# Fix permissions
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/whole-program-llvm
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/klee
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown -R user: /home/user/.bashrc
docker exec --user=root -ti ${TEMP_NAME} /usr/bin/chown user: /home/user/makefile

docker kill ${TEMP_NAME}
docker commit ${TEMP_NAME} ${NEW_IMAGE_NAME}
docker rm ${TEMP_NAME}
