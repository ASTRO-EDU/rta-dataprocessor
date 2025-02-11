#!/bin/bash

# Get the directory containing the script
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

cd "$SCRIPT_DIR"

mkdir -p build
pushd .
cd build

cmake ..
make

popd

