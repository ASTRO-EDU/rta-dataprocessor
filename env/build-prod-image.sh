#!/bin/bash

# Exit on error
set -e

BASE_IMAGE="rta-dataprocessor-base:v1.0.0"

IMAGE_MANIFEST="rta-dataprocessor-prod"

## take current git tag
GIT_TAG=$(git describe --tags)

echo "Git tag: $GIT_TAG"

# Build Docker image if it doesn't exist
echo "Building Docker image..."
docker build --build-arg BASE_IMAGE=$BASE_IMAGE -t $IMAGE_MANIFEST:$GIT_TAG -f ./Dockerfile.prod .. --no-cache

echo "Docker image $IMAGE_MANIFEST:$GIT_TAG built successfully!"
docker tag $IMAGE_MANIFEST:$GIT_TAG $IMAGE_MANIFEST:latest
echo "Docker image $IMAGE_MANIFEST:latest built successfully!"

# Bootstrap the image to allow the container's standard user to write on user host
echo "Bootstrapping Docker image..."
./bootstrap.sh $IMAGE_MANIFEST:$GIT_TAG $USER