#!/bin/bash

# This script clones the required libraries for ofxLlama.

# The target directory for the libraries, relative to the script location.
LIBS_DIR="$(dirname "$0")/../libs"

# Create the libs directory if it doesn't exist.
mkdir -p "$LIBS_DIR"

# Clone the repositories.
git clone https://github.com/google/minja "$LIBS_DIR/minja"
git clone https://github.com/ggml-org/llama.cpp "$LIBS_DIR/llama.cpp"

echo "Libraries cloned successfully."
