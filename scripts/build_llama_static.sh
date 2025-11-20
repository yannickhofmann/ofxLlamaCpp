#!/bin/bash

# This script builds llama.cpp as a static library for ofxLlama.

# Exit on error
set -e

# Get the directory of this script, and navigate to the parent directory (ofxLlama)
cd "$( dirname "${BASH_SOURCE[0]}" )/.."

# Go to the llama.cpp directory
cd libs/llama.cpp

# Clean up previous build
echo "Cleaning up previous build..."
rm -rf build

# Configure CMake for static build
echo "Configuring CMake for static build..."
cmake -B build -DBUILD_SHARED_LIBS=OFF

# Build llama.cpp
echo "Building llama.cpp..."
cmake --build build --config Release

# Create the destination directory
echo "Creating destination directory..."
mkdir -p lib/linux64

# Move the static libraries
echo "Moving static libraries..."
mv build/src/libllama.a lib/linux64/
mv build/ggml/src/libggml.a lib/linux64/
mv build/ggml/src/libggml-cpu.a lib/linux64/
mv build/ggml/src/libggml-base.a lib/linux64/
mv build/common/libcommon.a lib/linux64/
mv build/vendor/cpp-httplib/libcpp-httplib.a lib/linux64/

echo "llama.cpp built successfully and libraries moved to lib/linux64"
