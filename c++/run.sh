#!/bin/bash

# Exit on error
set -e

# Check if an executable name was provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <executable_name>"
    echo "Available executables:"
    echo "  ProcessDataConsumer1"
    echo "  ProcessDataConsumer2"
    echo "  CCSDSCppCons"
    echo "  CCSDSCppProd"
    echo "  HKtest"
    echo "  WFtest"
    exit 1
fi

EXECUTABLE=$1

# Check if the executable exists in the build directory
if [ ! -f "build/$EXECUTABLE" ]; then
    echo "Error: Executable '$EXECUTABLE' not found in build directory."
    echo "Make sure you have built the project first using ./build.sh"
    exit 1
fi

# Run the executable in Docker container
echo "Running $EXECUTABLE in Docker container..."
docker run -it --rm --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/../data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rta-dataprocessor \
    rta-dataprocessor:ubuntu \
    bash -c "cd /home/worker/workspace/c++/build && ./$EXECUTABLE" 