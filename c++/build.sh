#!/bin/bash

# Exit on error
set -e

# Build Docker image if it doesn't exist
if ! docker image inspect rta-dataprocessor:ubuntu_$USER >/dev/null 2>&1; then
    if ! docker image inspect rta-dataprocessor:ubuntu >/dev/null 2>&1; then
        echo "Building Docker image..."
        docker build -t rta-dataprocessor:ubuntu -f ../env/Dockerfile.ubuntu ..
    fi
    echo "Bootstrapping Docker image..."
    # Bootstrap the image to allow the container's standard user to write on user host
    ../env/bootstrap.sh rta-dataprocessor:ubuntu $USER
fi


# Run Docker container and build the project
echo "Building project in Docker container..."
docker run -it --rm --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/../data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rta-dataprocessor \
    rta-dataprocessor:ubuntu_$USER \
    bash -c "cd /home/worker/workspace/c++ && \
             rm -rf build && \
             mkdir build && \
             cd build && \
             cmake .. && \
             make -j$(nproc)"

echo "Build completed successfully!" 