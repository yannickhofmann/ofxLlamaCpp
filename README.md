# ofxLlamaCpp

openFrameworks addon for running LLM inference directly in C++ using [llama.cpp](https://github.com/ggerganov/llama.cpp).

## What is ofxLlamaCpp?

ofxLlamaCpp brings large-language-model inference into the openFrameworks ecosystem. It acts as a simple bridge between:

*   **openFrameworks (oF)**
*   **llama.cpp** (C/C++ inference engine)
*   **minja.hpp** (LLM chat-template rendering)

This makes it possible to integrate LLMs into creative coding projects, interactive installations, experimental interfaces, and real-time generative applications—all running locally without Python or external dependencies.

In addition to local GGUF inference, the examples `example_basics` and `example_chat` can also talk to remote OpenAI-compatible APIs. This includes self-hosted or proxied endpoints such as LiteLLM as well as Azure OpenAI deployments.

![ofxLlamaCpp](https://github.com/user-attachments/assets/1eff6ff2-d54f-4cec-87d6-9fa6e923d5ee)

## Tested Environments

This dev branch of ofxLlamaCpp has been validated on:

*   x86_64 (Ubuntu 24.04.3 LTS, Debian 12 & 13)
*   ARM 64-bit (Raspberry Pi)
*   macOS (Apple Silicon M2)
*   Windows 11 (tested CPU-only)
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

### Build Tool Dependency

**Linux and macOS**

* **CMake**
    is required for configuring and building the external **llama.cpp** and **minja.hpp** libraries that this addon utilizes.

**Ubuntu/Debian**

```bash
sudo apt update
sudo apt install cmake
```

**macOS**

```bash
brew install cmake
```

**Windows**

The openFrameworks Project Generator is used instead to create a Microsoft Visual Studio project.

### Example Dependencies

* **ofxDropdown** (developed by [roymacdonald](https://github.com/roymacdonald)) is used in the **example_chat** to provide a graphical user interface for selecting LLM models and chat templates via a dropdown menu.
    [https://github.com/roymacdonald/ofxDropdown](https://github.com/roymacdonald/ofxDropdown)


## Setup

**Linux and macOS**

Navigate to the `scripts` folder and run:
```bash
python install_llama.py
```

`install_llama.py` uses the shell scripts (`setup_libs.sh` and `build_llama_static.sh`).

**Windows**

Run `python install_llama.py` from the `x64 Native Tools Command Prompt for VS`.

`install_llama.py` automatically runs the PowerShell scripts (`setup_libs.ps1` and `build_llama_static.ps1`).

The build-related files are created outside the `ofxLlamaCpp` folder in `addons/ofxLlamaCpp_build/...` (on Windows, for example, in `addons/ofxLlamaCpp_build/windows/build`). These files can be deleted when no longer needed. If you want to remove them directly via the installer, run:
```bash
python install_llama.py --purge-all
```


### Build and Run the Examples

Once the static library is compiled and the GGUF models are in place, you can build and run the example projects.

**Linux and macOS**

Navigate into the example folder you wish to build (e.g., `example_chat`):

```bash
cd example_chat
```

Run **make** to compile the openFrameworks example:

```bash
make
```

Run the release executable:

```bash
make RunRelease
```

**Windows**

Create the example project with the openFrameworks Project Generator and build/run it using Microsoft Visual Studio.

### Remote API Support in the Examples

`example_basics` and `example_chat` support two backend modes:

* `Local`: loads GGUF models from `bin/data/models`
* `Remote`: sends requests to an OpenAI-compatible HTTP API

Remote mode is configured through `bin/data/models/remote_api_config.json`.

This allows the examples to work not only with local llama.cpp models, but also with:

* LiteLLM or other OpenAI-compatible gateways
* Azure OpenAI chat-completions endpoints
* compatible self-hosted APIs that expose `/v1/chat/completions` and, when available, `/v1/models`

The JSON config can be adapted to your target backend by setting values such as `api_type`, `api_endpoint`, `api_key`, `models_url`, and `preferred_model`.

In `example_chat`, the remote model list is loaded dynamically from the configured endpoint when the backend is switched to `Remote`.


## Models

Please use models in GGUF, a binary format that is optimized for quick loading and saving of models, making it highly efficient for inference purposes. Don't forget to get a GGUF model (e.g., from HuggingFace). Models in GGUF format need to get dropped into the `bin/data` folders of the examples.


## Known issues

*   Slow CPU inference with large models
*   Increasing latency due to In-Context Memory

## License

Copyright (c) 2025 Yannick Hofmann.

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL WARRANTIES, see the file, "LICENSE.txt," in this distribution.
