#!/bin/bash

# Build and install C++ dependencies from source

set -e

TEMP_DIR=${TEMP_DIR:-/tmp}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}

echo "Building dependencies in: $TEMP_DIR"
echo "Installing to: $INSTALL_PREFIX"

# Build and install cppzmq
echo ""
echo "=== Building cppzmq ==="
cd "$TEMP_DIR"
git clone https://github.com/zeromq/cppzmq.git
cd cppzmq
mkdir build
cd build
cmake ..
make -j$(nproc)
make install
ldconfig
cd "$TEMP_DIR"
rm -rf cppzmq
echo "cppzmq installed"

# Build and install Avro C++
echo ""
echo "=== Building Avro C++ ==="
cd "$TEMP_DIR"
git clone https://github.com/apache/avro.git
cd avro/lang/c++
git checkout release-1.11.0
git clean -fdx
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install
ldconfig
cd "$TEMP_DIR"
rm -rf avro
echo "Avro C++ installed"

# Install spdlog from source
echo ""
echo "=== Building spdlog ==="
cd "$TEMP_DIR"
git clone --branch v1.14.1 https://github.com/gabime/spdlog.git
cd spdlog
mkdir build
cd build
cmake ..
cmake --build .
make install
cd "$TEMP_DIR"
rm -rf spdlog
echo "spdlog installed"

echo ""
echo "All dependencies built and installed successfully"

