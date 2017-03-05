#!/bin/bash -x

# This script is a bit of a hack. A Dockerfile could
# do this for us.

set -e

EXPECTED_COMSYS_IMG_SHA256="sha256:8713ef77aa81af12c6a54d69bda11d149efa00b3be2ee55c0c58077dda051deb"

# Check the base image version
BASE_IMG="comsys/klee-dev-fpbench-prebuilt:latest"

docker inspect --format "ok" ${BASE_IMG}
if [ "$?" -ne 0 ]; then
  echo "Could not find image ${BASE_IMG}"
fi

COMSYS_IMG_SHA256=$(docker inspect --format "{{.Id}}" ${BASE_IMG})

if [ "X${COMSYS_IMG_SHA256}" != "X${EXPECTED_COMSYS_IMG_SHA256}" ]; then
  echo "Unexpected base image: ${COMSYS_IMG_SHA256}"
  echo "Expect base image is: ${EXPECTED_COMSYS_IMG_SHA256}"
  exit 1
fi

SCRIPTS_PATH=$( cd ${BASH_SOURCE[0]%/*} ; echo "$PWD" )
KLEE_GIT_HASH=$(cd ${SCRIPTS_PATH}/../ && git rev-parse HEAD)
echo "KLEE_GIT_HASH: ${KLEE_GIT_HASH}"

IMAGE_RAW="klee-afr-dev-fpbench-patched"
IMAGE_SQUASHED="klee-afr-dev-fpbench-patched-squashed"
IMAGE_FINAL="klee-afr-dev-fpbench-patched-final"
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
