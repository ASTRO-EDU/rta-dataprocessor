#!/bin/bash
set -u
set -v
set -e

MY_UID="$(id -u)"
MY_GID="$(id -g)"
SDE_TAG=git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:$1
INTERMEDIATE_TAG=local/delete_me_soon:latest

# just make sure the SDE exists on this machine
if [[ "$(docker images -q $SDE_TAG 2> /dev/null)" == "" ]]; then
    docker login oci-reg-cta.zeuthen.desy.de
    docker pull $SDE_TAG
fi

docker tag $SDE_TAG $INTERMEDIATE_TAG
docker rmi $SDE_TAG
docker build \
    --tag "${SDE_TAG}_$2" \
    - <<EOF
FROM ${INTERMEDIATE_TAG}
USER root
RUN usermod -u "${MY_UID}" astrisw && groupmod -g "${MY_GID}" astrima
USER astrisw
EOF
docker rmi $INTERMEDIATE_TAG
