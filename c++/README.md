# RTA Data Processor C++ Components

This directory contains the C++ components of the RTA Data Processor project. The project includes several executables for data processing and communication.

## Prerequisites

- Docker
- Git
- CMake (3.10 or higher)
- C++17 compatible compiler

## Cleaning Previous Builds

Before building the project, you may want to clean any previous builds. You can do this in two ways:

1. Clean the build directory:
```bash
rm -rf c++/build
```

2. Clean the Docker container and image:
```bash
# Stop and remove the container if it exists
docker rm -f rta-dataprocessor 2>/dev/null || true

# Remove the Docker image
docker rmi rta-dataprocessor:ubuntu 2>/dev/null || true
```

## Building the Project

### Using Docker (Recommended)

1. Build the Docker image:
```bash
docker build -t rta-dataprocessor:ubuntu -f ../env/Dockerfile.ubuntu ..
```

2. Run the Docker container:
```bash
docker run -it --rm --platform linux/amd64 \
    -v "$(pwd)/..:/home/worker/workspace" \
    -v "$(pwd)/../shared_dir:/shared_dir" \
    -v "$(pwd)/../data01:/data01" \
    -v "$(pwd)/../data02:/data02" \
    --name rta-dataprocessor \
    rta-dataprocessor:ubuntu
```

3. Inside the container, build the project:
```bash
cd /home/worker/workspace/c++
mkdir -p build
cd build
cmake -DENABLE_LOGGING=OFF ..
make -j$(nproc)
```

### Manual Build (Not Recommended)

If you need to build without Docker, ensure you have all dependencies installed:

- Boost (1.73.0 or higher)
- ZeroMQ (4.3.4 or higher)
- Avro C++ (1.11.0)
- spdlog (1.14.1 or higher)

Then follow the same build steps as in the Docker container.

## Available Executables

After building, the following executables will be available in the `build` directory:

1. `ProcessDataConsumer1` - Main data processor (version 1)
2. `ProcessDataConsumer2` - Main data processor (version 2)
3. `CCSDSCppCons` - CCSDS Consumer
4. `CCSDSCppProd` - CCSDS Producer
5. `HKtest` - Housekeeping test
6. `WFtest` - Waveform test

## Running the Components

### Process Monitor

1. Start the Python ProcessMonitoring:
```bash
cd /rtadp-c/workers
python ProcessMonitoring.py ../c++/config.json
```

### Data Processors

1. Start ProcessDataConsumer1:
```bash
./ProcessDataConsumer1 ../config.json
```

2. Start ProcessDataConsumer2:
```bash
./ProcessDataConsumer2 ../config.json
```

### Command Sender

1. Send the start command to the ProcessDataConsumer in order to start processing the data:
```bash
cd /rtadp-c/workers
python SendCommand.py ../c++/config.json start all
```

### DAMS Components (GitLab version)

1. Start the DAMS Producer-Streamer:
```bash
cd dams/dl0
python gfse.py --addr x.x.x.x --port 1234 --indir dl0_simulated/ --rpid 1 --wform-sec 100
```

### Optional Test Components

1. Run Housekeeping test:
```bash
./HKtest
```

2. Run Waveform test:
```bash
./WFtest
```

## Project Structure

- `include/` - Header files
- `src/` - Source files
- `gs_examples_communication/` - Communication library
- `build/` - Build directory (created during build)

## Configuration

The project uses configuration files in JSON format. Make sure to set up the appropriate configuration files before running the components.

## Troubleshooting

1. If you encounter missing library errors, ensure all dependencies are properly installed.
2. For Docker-related issues, make sure Docker is running and you have sufficient permissions.
3. If build fails, try cleaning the build directory and rebuilding:
```bash
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## License

See the main project LICENSE file for details.
