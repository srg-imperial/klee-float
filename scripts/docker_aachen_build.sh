#!/bin/bash -x

# This script is a bit of a hack. A Dockerfile could
# do this for us.

set -e

SCRIPTS_PATH=$( cd ${BASH_SOURCE[0]%/*} ; echo "$PWD" )
KLEE_GIT_HASH=$(cd ${SCRIPTS_PATH}/../ && git rev-parse HEAD)
echo "KLEE_GIT_HASH: ${KLEE_GIT_HASH}"

IMAGE_RAW="klee-dev-fpbench-patched"
IMAGE_SQUASHED="klee-dev-fpbench-patched-squashed"
IMAGE_FINAL="klee-dev-fpbench-patched-final"
# Patch the image
docker build -t ${IMAGE_RAW} --build-arg host_user_id="$(id -u)" -f ${SCRIPTS_PATH}/patch_aachen.Dockerfile .

# Squash image
sleep 2
container=$(docker run -d ${IMAGE_RAW} /bin/true)
docker export ${container} | docker import - ${IMAGE_SQUASHED}
docker build -t ${IMAGE_FINAL} --build-arg klee_git_hash="${KLEE_GIT_HASH}" - < ${SCRIPTS_PATH}/patch_aachen.metadata.Dockerfile

# Remove temporaries
sleep 2
docker rm ${container}
sleep 2
docker rmi ${IMAGE_SQUASHED}
docker rmi ${IMAGE_RAW}
