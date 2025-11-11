#!/bin/bash

# RTA Data Processor Framework Installation Script

set -e

INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}

echo "Installing RTA Data Processor Framework to $INSTALL_PREFIX..."

# Check write permissions before building
if [ -d "$INSTALL_PREFIX" ]; then
    # Directory exists, check if writable
    if [ ! -w "$INSTALL_PREFIX" ]; then
        echo ""
        echo "ERROR: No write permission for $INSTALL_PREFIX"
        echo ""
        echo "Solutions:"
        echo "  1. Run with sudo: sudo ./install.sh"
        echo "  2. Use custom prefix: INSTALL_PREFIX=/custom/path ./install.sh"
        echo ""
        exit 1
    fi
else
    # Directory doesn't exist, check parent directory
    PARENT_DIR=$(dirname "$INSTALL_PREFIX")
    if [ ! -w "$PARENT_DIR" ]; then
        echo ""
        echo "ERROR: No write permission for $PARENT_DIR (parent of install prefix)"
        echo ""
        echo "Solutions:"
        echo "  1. Run with sudo: sudo ./install.sh"
        echo "  2. Use custom prefix: INSTALL_PREFIX=/custom/path ./install.sh"
        echo ""
        exit 1
    fi
fi

# Build
BUILD_DIR="build"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
      ..

make -j$(nproc)

make install

echo ""
echo "✓ Installation completed successfully!"
echo ""
echo "The framework is now available via: find_package(RTADataProcessor REQUIRED)"
echo ""

