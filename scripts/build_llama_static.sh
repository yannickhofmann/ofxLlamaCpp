#!/bin/bash

# This script builds llama.cpp as a static library for ofxLlama on Linux and macOS.

# Exit on error
set -e

# --- 1. Betriebssystem-Erkennung und Konfiguration ---
# Set the destination directory based on the operating system
# Linux: lib/linux64
# macOS: lib/osx-arm64 (for Apple Silicon like M2)

DEST_DIR=""
LLAMA_LIB_NAME="libllama.a" # Generell derselbe Name
CMAKE_FLAGS="-DBUILD_SHARED_LIBS=OFF"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux (x86_64)
    DEST_DIR="lib/linux64"
    echo "Detected OS: Linux (Targeting $DEST_DIR)"
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS (Apple Silicon M2 / arm64)
    DEST_DIR="lib/osx-arm64" 
    CMAKE_FLAGS+=" -DGGML_METAL=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
    echo "Detected OS: macOS (Apple Silicon M2, Targeting $DEST_DIR)"

else
    echo "Error: Unsupported operating system ($OSTYPE)."
    exit 1
fi

# --- 2. Vorbereitung ---

# Get the directory of this script, and navigate to the parent directory (ofxLlama)
cd "$( dirname "${BASH_SOURCE[0]}" )/.."

# Go to the llama.cpp directory
cd libs/llama.cpp

# Clean up previous build
echo "Cleaning up previous build..."
rm -rf build

# --- 3. CMake Konfiguration und Build ---

# Configure CMake for static build
echo "Configuring CMake for static build..."
cmake -B build $CMAKE_FLAGS

# Build llama.cpp
echo "Building llama.cpp..."
cmake --build build --config Release

# --- 4. Verschieben der Bibliotheken ---

# Create the destination directory
echo "Creating destination directory: $DEST_DIR"
mkdir -p "$DEST_DIR"

# Move the static libraries
echo "Moving static libraries to $DEST_DIR..."

# In neueren llama.cpp Versionen sind einige ggml Bibliotheken zusammengefasst.
# Hier eine Liste, die die gängigsten statischen Bibliotheken abdeckt, falls sie existieren.

# Check if the specific library files exist before moving (optional, for robustness)
move_if_exists() {
    local source=$1
    local destination=$2
    if [ -f "$source" ]; then
        mv "$source" "$destination"
        echo "Moved $source"
    else
        echo "Warning: $source not found, skipping."
    fi
}

move_if_exists "build/src/libllama.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml-cpu.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml-base.a" "$DEST_DIR/" # Ältere Versionen
move_if_exists "build/common/libcommon.a" "$DEST_DIR/"
move_if_exists "build/vendor/cpp-httplib/libcpp-httplib.a" "$DEST_DIR/"

if [[ "$OSTYPE" == "darwin"* ]]; then
    move_if_exists "build/ggml/src/ggml-metal/libggml-metal.a" "$DEST_DIR/"
    move_if_exists "build/ggml/src/ggml-blas/libggml-blas.a" "$DEST_DIR/"
fi


echo "llama.cpp built successfully and libraries moved to $DEST_DIR"