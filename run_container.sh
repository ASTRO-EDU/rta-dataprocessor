#!/bin/bash

# Create necessary directories if they don't exist
mkdir -p shared_dir data01 data02

# Run the container with all necessary configurations
docker run -it --rm \
    --platform linux/amd64 \
    -v "$(pwd):/home/worker/workspace" \
    -v "$(pwd)/shared_dir:/shared_dir" \
    -v "$(pwd)/data01:/data01" \
    -v "$(pwd)/data02:/data02" \
    --name rta-dataprocessor \
    rta-dataprocessor:ubuntu 