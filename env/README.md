# RTA Data Processor - Docker Environment

This directory contains Docker configurations for building the RTA Data Processor environment.

## Docker Image Architecture

The build is organized in layers:

1. **Base Image** (`rta-dataprocessor-base`) - System dependencies and C++ libraries
2. **Production Image** (`rta-dataprocessor-prod`) - RTA Data Processor framework installed

## Quick Start

### Build Base Image (once)

```bash
cd rta-dataprocessor/env
./build-base-image.sh
```

This creates `rta-dataprocessor-base:v1.0.0` with:
- Ubuntu 22.04
- GCC 11, CMake, Python 3
- Boost, ZeroMQ, Avro, spdlog
- System libraries (HDF5, TinyXML2, etc.)

### Build Production Image

```bash
cd rta-dataprocessor/env
./build-prod-image.sh
```

This creates `rta-dataprocessor-prod:<branch>-<commit>` with:
- Everything from base image
- RTA Data Processor framework compiled and installed

### Bootstrap for User Permissions

```bash
./bootstrap.sh rta-dataprocessor-prod:<tag> $USER
```

Creates a user-specific image that can write to your host filesystem.

## Manual Build

If you prefer to build manually:

```bash
# Base image
docker build -t rta-dataprocessor-base:v1.0.0 \
    -f docker_base/Dockerfile.base \
    ..

# Production image
docker build -t rta-dataprocessor-prod:latest \
    --build-arg BASE_IMAGE=rta-dataprocessor-base:v1.0.0 \
    -f Dockerfile.prod \
    ..
```

## Using the Images

### Interactive Shell

```bash
docker run -it --rm \
    -v $(pwd)/..:/home/worker/rta-dataprocessor \
    rta-dataprocessor-prod:latest \
    bash
```

### Run Application

```bash
docker run -it --rm \
    -v $(pwd)/..:/home/worker/rta-dataprocessor \
    rta-dataprocessor-prod:latest \
    /home/worker/rta-dataprocessor/c++/build/ProcessDataConsumer1
```

## Directory Structure

```
env/
├── Dockerfile.base          # Base image with system dependencies
├── Dockerfile.prod          # Production image with framework installed
├── docker_base/
│   ├── install-system-packages.sh  # Install Ubuntu packages
│   └── build-dependencies.sh       # Build C++ libraries from source
├── build-base-image.sh      # Script to build base image
├── build-prod-image.sh      # Script to build production image
└── bootstrap.sh             # Create user-specific image
```

## Updating Images

### Update System Dependencies

Edit `docker_base/install-system-packages.sh` or `docker_base/build-dependencies.sh`, then rebuild base image.

### Update Framework

Just rebuild the production image - it will recompile the latest code:

```bash
./build-prod-image.sh
```

## Troubleshooting

### Build fails with permission errors

Make sure you run bootstrap:
```bash
./bootstrap.sh <image-name> $USER
```

### Image not found

List available images:
```bash
docker images | grep rta-dataprocessor
```

### Clean rebuild

Remove old images and rebuild:
```bash
docker rmi rta-dataprocessor-prod:latest
docker rmi rta-dataprocessor-base:v1.0.0
./build-base-image.sh
./build-prod-image.sh
```

## Notes

- Base image needs to be rebuilt only when system dependencies change
- Production image should be rebuilt when framework code changes
- Image tags include git branch and commit hash for traceability
- Bootstrap creates a user-specific image suffixed with your username
