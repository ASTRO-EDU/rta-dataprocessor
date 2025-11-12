#!/bin/bash

# Exit on error
set -e

IMAGE_MANIFEST="git.ia2.inaf.it:5050/gammasky/gammasky-cimone/rta-dataprocessor-base"
TAG="v1.0.0"

# Build Docker image if it doesn't exist
echo "Building Docker image..."
docker build -t $IMAGE_MANIFEST:$TAG -f ./Dockerfile.base .. --no-cache
echo "Bootstrapping Docker image..."
# Bootstrap the image to allow the container's standard user to write on user host
./bootstrap.sh $IMAGE_MANIFEST:$TAG $USER

echo "Docker image $IMAGE_MANIFEST:$TAG built successfully!"
docker tag $IMAGE_MANIFEST:$TAG $IMAGE_MANIFEST:latest
echo "Docker image $IMAGE_MANIFEST:latest built successfully!"
