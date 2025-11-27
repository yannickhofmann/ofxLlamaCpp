#!/bin/bash

# This script builds llama.cpp as a static library for ofxLlama on Linux and macOS, now with full GPU detection.

set -e

# -----------------------------
# 1. OS Detection
# -----------------------------

DEST_DIR=""
LLAMA_LIB_NAME="libllama.a"
CMAKE_FLAGS="-DBUILD_SHARED_LIBS=OFF"

echo "========================================="
echo "   GPU-Aware llama.cpp Build Script"
echo "========================================="

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux (Ubuntu / general Linux)
    DEST_DIR="lib/linux64"
    echo "Detected OS: Linux"
    echo "→ Checking for CUDA..."

    # Check CUDA toolkit presence
    if command -v nvcc >/dev/null 2>&1; then
        echo "✔ CUDA detected — enabling GGML_CUDA and GGML_BLAS"
        CMAKE_FLAGS+=" -DGGML_CUDA=ON -DGGML_BLAS=ON"
    else
        echo "✖ CUDA not found."
        echo "→ Checking for Vulkan SDK..."

        if pkg-config --exists vulkan; then
            echo "✔ Vulkan detected — enabling GGML_VULKAN"
            CMAKE_FLAGS+=" -DGGML_VULKAN=ON"
        else
            echo "✖ Vulkan SDK not found."
            echo "⚠ Building CPU-only fallback!"
        fi
    fi


elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS (Apple Silicon)
    DEST_DIR="lib/osx-arm64"
    echo "Detected OS: macOS (Apple Silicon assumed)"

    echo "✔ Enabling Metal backend"
    CMAKE_FLAGS+=" -DGGML_METAL=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"

else
    echo "Error: Unsupported OS ($OSTYPE)"
    exit 1
fi

echo "Using CMake flags: $CMAKE_FLAGS"
echo "Destination directory: $DEST_DIR"
echo "-----------------------------------------"

# -----------------------------
# 2. Preparation
# -----------------------------

cd "$( dirname "${BASH_SOURCE[0]}" )/.."
cd libs/llama.cpp

# cleanup
echo "Cleaning up previous build..."
rm -rf build

# -----------------------------
# 3. CMake Configure & Build
# -----------------------------

echo "Configuring CMake..."
cmake -B build $CMAKE_FLAGS

echo "Building llama.cpp..."
cmake --build build --config Release

# -----------------------------
# 4. Move Libraries
# -----------------------------

echo "Creating destination directory: $DEST_DIR"
mkdir -p "$DEST_DIR"

move_if_exists() {
    local source=$1
    local destination=$2
    if [ -f "$source" ]; then
        mv "$source" "$destination"
        echo "Moved: $source"
    else
        echo "Warning: $source not found"
    fi
}

move_if_exists "build/src/libllama.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml-cpu.a" "$DEST_DIR/"
move_if_exists "build/ggml/src/libggml-base.a" "$DEST_DIR/"
move_if_exists "build/common/libcommon.a" "$DEST_DIR/"
move_if_exists "build/vendor/cpp-httplib/libcpp-httplib.a" "$DEST_DIR/"

# Linux GPU libs
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    move_if_exists "build/ggml/src/ggml-cuda/libggml-cuda.a" "$DEST_DIR/"
    move_if_exists "build/ggml/src/ggml-blas/libggml-blas.a" "$DEST_DIR/"
fi

# macOS GPU libs
if [[ "$OSTYPE" == "darwin"* ]]; then
    move_if_exists "build/ggml/src/ggml-metal/libggml-metal.a" "$DEST_DIR/"
    move_if_exists "build/ggml/src/ggml-blas/libggml-blas.a" "$DEST_DIR/"
fi

echo "========================================="
echo "llama.cpp built successfully with GPU support (if available)."
echo "Libraries located in: $DEST_DIR"
echo "========================================="
