# ofxLlamaCpp

A lightweight openFrameworks addon for running LLM inference directly in C++ using [llama.cpp](https://github.com/ggerganov/llama.cpp).

## What is ofxLlamaCpp?

ofxLlama brings efficient large-language-model inference into the openFrameworks ecosystem. It acts as a simple bridge between:

*   **openFrameworks (oF)**
*   **llama.cpp** (C/C++ inference engine)
*   **minja.hpp** (LLM chat-template rendering)

This makes it possible to integrate LLMs into creative coding projects, interactive installations, experimental interfaces, and real-time generative applications—all running locally without Python or external dependencies.

## Tested Environments

ofxLlama has been validated on:

*   Ubuntu 24.04.3 LTS
*   openFrameworks:
    *   `of_v0.12.0_linux64gcc6_release`
    *   `of_v0.12.1_linux64_gcc6_release`

## Dependencies

*   **llama.cpp**
    High-performance LLM inference in pure C/C++.
    [https://github.com/ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp)
    
*   **minja.hpp**
    A minimalistic C++ Jinja-style templating engine used to prepare LLM chat prompts.
    [https://github.com/google/minja](https://github.com/google/minja)

## Setup

In the `scripts` folder, you will find a shell script to automatically clone the proper libraries. Navigate to the `scripts` folder and execute:
```bash
chmod +x setup_libs.sh
./setup_libs.sh
```

You will have to build `llama.cpp` as a static pre-compiled library. In the `scripts` folder, you will find a shell script to automatically build `llama.cpp` as a static pre-compiled library. Navigate to the `scripts` folder and execute:
```bash
chmod +x build_llama_static.sh
./build_llama_static.sh
```

## Models

Please use models in GGUF, a binary format that is optimized for quick loading and saving of models, making it highly efficient for inference purposes. Don't forget to get a GGUF model (e.g., from HuggingFace). Models in GGUF format need to get dropped into the `bin/data` folders of the examples.

## License

Copyright (c) 2025 Yannick Hofmann.

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL WARRANTIES, see the file, "LICENSE.txt," in this distribution.
