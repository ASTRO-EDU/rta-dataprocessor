# gammasky-env
gammasky-env

# RTADP-C++ Environment Setup and Execution Guide

## Setup Docker

### Clone the repository:
```bash
git clone --branch rta-dp-c++ https://github.com/ASTRO-EDU/rta-dataprocessor.git
```

### Option 1: Use the prebuilt docker image
```bash
# Login to the registry
docker login git.ia2.inaf.it:5050

# Pull the image
docker pull git.ia2.inaf.it:5050/gammasky/gammasky-cimone/rta-dataprocessor:1.0.1

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

## Set Up Model Environment
Before running the tests, set up the environment variable for the inference model:
```bash
# Set the model path environment variable
echo 'export RTADP_MODEL_PATH="/path/to/model/float_16.tflite"' >> ~/.bashrc

# Reload the bash configuration
source ~/.bashrc
```
Note: By default the float 16 quantized model (float_16.tflite) can be found under rta-dataprocessor/test/ml_models

## Run Pipeline Tests
1. To run the integration test of ProcessDataConsumer1 (inference + DL2 writing):
```bash
# Navigate to the test directory
cd rta-dataprocessor/test

# Run the integration test
python3 test_integration_rtadp1.py
```

2. To run the integration test of ProcessDataConsumer2 (basic data processing):
```bash
# Navigate to the test directory
cd rta-dataprocessor/test

# Run the integration test
python3 test_integration_rtadp2.py
```

## Run the Components Separately
To run the 4 components individually follow the guide: https://github.com/ASTRO-EDU/rta-dataprocessor/tree/rta-dp-c%2B%2B/c%2B%2B#running-the-components

