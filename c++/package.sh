#!/bin/bash

# RTA Data Processor Framework - Package Generation Script
# Creates distributable tarball package

set -e

echo "Creating RTA Data Processor Framework package..."
echo ""

# Build directory
BUILD_DIR="build"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure for packaging
echo "Configuring..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      ..

# Build
echo "Building..."
make -j$(nproc)

# Generate packages
echo ""
echo "Generating packages..."
cpack

# List generated package
echo ""
echo "✓ Package created successfully!"
echo ""
ls -lh *.tar.gz
echo ""
echo "Package location: $(pwd)"
echo ""

# Show installation instructions
echo "Installation instructions:"
echo "  tar xzf rta-dataprocessor-framework-*.tar.gz -C /usr"
echo ""
echo "Or extract to custom location:"
echo "  tar xzf rta-dataprocessor-framework-*.tar.gz -C /custom/path"
echo ""

