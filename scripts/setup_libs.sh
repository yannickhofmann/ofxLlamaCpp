#!/bin/bash

set -euo pipefail

# This script clones the required libraries for ofxLlamaCpp.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBS_DIR="$SCRIPT_DIR/../libs"

mkdir -p "$LIBS_DIR"

clone_if_missing() {
    local url=$1
    local target=$2

    if [[ -d "$target" ]]; then
        echo "Skipping $(basename "$target"): already exists at $target"
        return
    fi

    git clone "$url" "$target"
}

clone_if_missing https://github.com/google/minja "$LIBS_DIR/minja"
clone_if_missing https://github.com/ggml-org/llama.cpp "$LIBS_DIR/llama.cpp"

echo "Libraries prepared successfully."
