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
docker pull git.ia2.inaf.it:5050/gammasky/gammasky-cimone/rta-dataprocessor:1.0.2
```

### Option 2: Build it yourself
```bash
# Navigate to the C++ directory
cd rta-dataprocessor/c++

# Build the Docker image
docker build -t rta-dataprocessor:1.0.2 -f ../env/Dockerfile.ubuntu ..
```

### Run bootstrap
```bash
# Bootstrap the image to allow the container's standard user to write on user host
cd ../env
./bootstrap.sh rta-dataprocessor:1.0.2 $USER
```

### Run and enter the container
```bash
# Run the container
docker run -dt --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/./data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rtadataprocessor \
    rta-dataprocessor:1.0.2

# Enter the container
docker exec -it rtadataprocessor bash
```

## Build and Compile Project
Once inside the container, follow these steps to build the project:
```bash
# Navigate to the C++ directory
cd workspace/c++

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
echo 'export RTADP_MODEL_PATH="/home/worker/workspace/test/ml_models/float_16.tflite"' >> ~/.bashrc

# Reload the bash configuration
source ~/.bashrc
```
Note: By default the float 16 quantized model (float_16.tflite) can be found under test/ml_models

## Run Pipeline Tests
1. To run the integration test of ProcessDataConsumer1 (inference + DL2 writing):
```bash
# Navigate to the test directory
cd $HOME/workspace/test

# Run the integration test
python3 test_integration_rtadp1.py
```

2. To run the integration test of ProcessDataConsumer2 (basic data processing):
```bash
# Navigate to the test directory
cd $HOME/workspace/test

# Run the integration test
python3 test_integration_rtadp2.py
```

## Run the Components Separately
1. DAMS-side (Terminal 1):
```bash
# Launch the simulator/streamer
cd $HOME/workspace/test/
python gfse.py --addr 127.0.0.1 --port 1234 --indir dl0_simulated/ --rpid 1 --wform-sec 200
```

2. RTA-DP C++-side (requires 3 parallel terminals):

Terminal 2 - Process Monitoring:
```bash
cd $HOME/workspace/workers
python ProcessMonitoring.py $RTACONFIG
```

Terminal 3 - Consumer:
```bash
cd $HOME/workspace/c++/build
./ProcessDataConsumer1 $RTACONFIG
```
Or:
```bash
cd $HOME/workspace/c++/build
./ProcessDataConsumer2 $RTACONFIG
```

Terminal 4 - Send Command:
```bash
cd $HOME/workspace/workers
python SendCommand.py $RTACONFIG start all
```
