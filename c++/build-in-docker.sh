#!/bin/bash

# Exit on error
set -e

# Run Docker container and build the project
echo "Building project in Docker container..."
docker run -it --rm --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace/dams" \
    -v /data01/users/castaldini/rta-dataprocessor/c++:/home/worker/workspace/ \
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
