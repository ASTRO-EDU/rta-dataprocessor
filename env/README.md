# gammasky-env
gammasky-env

# RTADP-C++ Environment Setup and Execution Guide

## Setup Docker

### Option 1: Use the prebuilt docker image
```bash
# Login to the registry
docker login git.ia2.inaf.it:5050

# Pull the image
docker pull git.ia2.inaf.it:5050/gammasky/gammasky-cimone/git.ia2.inaf.it:5050/gammasky/gammasky-cimone/rta-dataprocessor:1.0.1

# Run the container
docker run -dt --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/./data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rtadataprocessor \
    git.ia2.inaf.it:5050/gammasky/gammasky-cimone/rta-dataprocessor:1.0.1

# Enter the container
docker exec -it -u0 rtadataprocessor bash
```

### Option 2: Build it yourself
```bash
# Navigate to the C++ directory
cd rta-dataprocessor/c++

# Build the Docker image
docker build -t rta-dataprocessor:1.0.1 -f ../env/Dockerfile.ubuntu ..

# Run the container
docker run -dt --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/./data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rtadataprocessor \
    rta-dataprocessor:1.0.1

# Enter the container
docker exec -it -u0 rtadataprocessor bash
```

## Build and Compile Project
Once inside the container, follow these steps to build the project:
```bash
# Navigate to the C++ directory
cd rta-dataprocessor/c++

# Clean and create build directory
rm -rf build
mkdir build && cd build

# Configure with CMake
cmake -DENABLE_LOGGING=OFF ..

# Build the project
make -j4
```

## Run Pipeline Test
To run the integration test:
```bash
# Navigate to the test directory
cd rta-dataprocessor/test

# Run the integration test
python3 test_integration.py
```

