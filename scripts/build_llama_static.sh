#!/bin/bash

set -euo pipefail

DEST_DIR=""
CMAKE_FLAGS="-DBUILD_SHARED_LIBS=OFF -DLLAMA_BUILD_COMMON=ON -DLLAMA_BUILD_TOOLS=ON"
BUILD_DIR=""
CLEAN_BUILD=0
PURGE_BUILD=0
PURGE_ALL=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        --purge-build)
            PURGE_BUILD=1
            shift
            ;;
        --purge-all)
            PURGE_ALL=1
            shift
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

echo "========================================="
echo "   GPU-Aware llama.cpp Build Script"
echo "========================================="

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ARCH="$(uname -m)"
    if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
        DEST_DIR="lib/linuxaarch64"
    else
        DEST_DIR="lib/linux64"
    fi

    echo "Detected OS: Linux ($ARCH)"

    if command -v nvcc >/dev/null 2>&1 && command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1; then
        echo "Enabling GGML_CUDA and GGML_BLAS"
        CMAKE_FLAGS+=" -DGGML_CUDA=ON -DGGML_BLAS=ON"
    else
        if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
            if [[ "${OFX_LLAMACPP_ENABLE_VULKAN:-0}" == "1" ]]; then
                if pkg-config --exists vulkan && command -v glslc >/dev/null 2>&1; then
                    echo "Enabling GGML_VULKAN"
                    CMAKE_FLAGS+=" -DGGML_VULKAN=ON"
                else
                    echo "Warning: Vulkan requested but SDK or glslc is missing. Building CPU-only fallback."
                fi
            else
                echo "Building CPU-only fallback."
            fi
        else
            if pkg-config --exists vulkan && command -v glslc >/dev/null 2>&1; then
                echo "Enabling GGML_VULKAN"
                CMAKE_FLAGS+=" -DGGML_VULKAN=ON"
            else
                echo "Building CPU-only fallback."
            fi
        fi
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    DEST_DIR="lib/osx-arm64"
    echo "Detected OS: macOS"
    CMAKE_FLAGS+=" -DGGML_METAL=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
else
    echo "Error: Unsupported OS ($OSTYPE)"
    exit 1
fi

cd "$( dirname "${BASH_SOURCE[0]}" )/.."
ADDON_DIR="$(pwd)"
ADDONS_DIR="$(cd .. && pwd)"
if [[ "$OSTYPE" == "darwin"* ]]; then
    EXTERNAL_ROOT="$ADDONS_DIR/ofxLlamaCpp_build/macos"
else
    EXTERNAL_ROOT="$ADDONS_DIR/ofxLlamaCpp_build/linux"
fi

if [[ -z "$BUILD_DIR" ]]; then
    BUILD_DIR="$EXTERNAL_ROOT/build"
fi

if [[ ! -d libs/llama.cpp ]]; then
    echo "Error: libs/llama.cpp not found. Run setup_libs.sh first."
    exit 1
fi

LLAMA_DIR="$ADDON_DIR/libs/llama.cpp"
BUILD_DIR="$(python3 - <<'PY' "$BUILD_DIR"
import os, sys
print(os.path.abspath(sys.argv[1]))
PY
)"

if [[ "$CLEAN_BUILD" == "1" ]]; then
    echo "Cleaning previous external build..."
    rm -rf "$BUILD_DIR"
fi

echo "Destination directory: $DEST_DIR"
echo "Build directory: $BUILD_DIR"
echo "Configuring CMake..."
cmake -S "$LLAMA_DIR" -B "$BUILD_DIR" $CMAKE_FLAGS

echo "Building llama.cpp..."
cmake --build "$BUILD_DIR" --config Release

mkdir -p "$LLAMA_DIR/$DEST_DIR"

copy_if_exists() {
    local source=$1
    local destination=$2
    if [[ -f "$source" ]]; then
        cp -f "$source" "$destination"
        echo "Copied: $source"
    else
        echo "Warning: $source not found"
    fi
}

copy_if_exists "$BUILD_DIR/src/libllama.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/ggml/src/libggml.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/ggml/src/libggml-cpu.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/ggml/src/libggml-base.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/common/libcommon.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/tools/mtmd/libmtmd.a" "$LLAMA_DIR/$DEST_DIR/"
copy_if_exists "$BUILD_DIR/vendor/cpp-httplib/libcpp-httplib.a" "$LLAMA_DIR/$DEST_DIR/"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    copy_if_exists "$BUILD_DIR/ggml/src/ggml-cuda/libggml-cuda.a" "$LLAMA_DIR/$DEST_DIR/"
    copy_if_exists "$BUILD_DIR/ggml/src/ggml-blas/libggml-blas.a" "$LLAMA_DIR/$DEST_DIR/"
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    copy_if_exists "$BUILD_DIR/ggml/src/ggml-metal/libggml-metal.a" "$LLAMA_DIR/$DEST_DIR/"
    copy_if_exists "$BUILD_DIR/ggml/src/ggml-blas/libggml-blas.a" "$LLAMA_DIR/$DEST_DIR/"
fi

if [[ "$PURGE_BUILD" == "1" ]]; then
    echo "Purging external build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

if [[ "$PURGE_ALL" == "1" ]]; then
    echo "Purging external workspace: $EXTERNAL_ROOT"
    rm -rf "$EXTERNAL_ROOT"
fi

echo "========================================="
echo "llama.cpp build complete"
echo "Libraries located in: $LLAMA_DIR/$DEST_DIR"
echo "========================================="
