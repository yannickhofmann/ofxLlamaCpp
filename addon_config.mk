# ofxLlamaCpp addon_config.mk
# Cross-platform configuration for Linux and macOS

meta:
	ADDON_NAME = ofxLlamaCpp
	ADDON_DESCRIPTION = openFrameworks wrapper for llama.cpp
	ADDON_AUTHOR = Yannick Hofmann
	ADDON_TAGS = "ai", "nlp", "llama"
	ADDON_URL = tba

common:
	# Addon dependencies
	# ADDON_DEPENDENCIES = 

	# Include search paths - platform-independent
	ADDON_INCLUDES += libs/llama.cpp/include
	ADDON_INCLUDES += libs/llama.cpp/ggml/include

	ADDON_INCLUDES += libs/minja/include
	# Prevent OF's automatic include parser from adding the full vendored trees.
	# Keep only explicit include roots above to avoid oversized command lines.
	ADDON_INCLUDES_EXCLUDE += libs/llama.cpp
	ADDON_INCLUDES_EXCLUDE += libs/llama.cpp/%
	ADDON_INCLUDES_EXCLUDE += libs/minja
	ADDON_INCLUDES_EXCLUDE += libs/minja/%

	# Source files
	ADDON_SOURCES = src/ofxLlamaCpp.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/gguf.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/ggml-opt.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/ggml-backend-reg.cpp

	# Common compiler flags
	ADDON_CPPFLAGS = -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -DGGML_SCHED_MAX_COPIES=4 -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter
	ADDON_CPPFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter


# --- Platform-specific configuration for LINUX (x86_64) ---
linux64:
	# Auto-enable CUDA only if CUDA libs are present and CUDA+GPU are available.
	# Can still be overridden manually via:
	#   make OFX_LLAMACPP_USE_CUDA=0
	#   make OFX_LLAMACPP_USE_CUDA=1
	OFX_LLAMACPP_HAS_CUDA_LIB ?= $(wildcard $(OF_ADDONS_PATH)/ofxLlamaCpp/libs/llama.cpp/lib/linux64/libggml-cuda.a)
	OFX_LLAMACPP_HAS_NVCC ?= $(shell command -v nvcc >/dev/null 2>&1 && echo 1)
	OFX_LLAMACPP_HAS_GPU ?= $(shell command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi -L >/dev/null 2>&1 && echo 1)
	OFX_LLAMACPP_USE_CUDA ?= $(if $(and $(OFX_LLAMACPP_HAS_CUDA_LIB),$(OFX_LLAMACPP_HAS_NVCC),$(OFX_LLAMACPP_HAS_GPU)),1,0)
	OFX_LLAMACPP_CUDA_HOME ?= $(if $(CUDA_HOME),$(CUDA_HOME),$(if $(CUDA_PATH),$(CUDA_PATH),$(shell command -v nvcc >/dev/null 2>&1 && dirname $$(dirname $$(command -v nvcc)))))
	OFX_LLAMACPP_CUDA_LIB_DIR_1 ?= $(OFX_LLAMACPP_CUDA_HOME)/targets/x86_64-linux/lib
	OFX_LLAMACPP_CUDA_LIB_DIR_2 ?= $(OFX_LLAMACPP_CUDA_HOME)/lib64
	OFX_LLAMACPP_CUDART_DIR ?= $(shell ldconfig -p 2>/dev/null | awk '/libcudart\.so/{print $$NF; exit}' | xargs -r dirname)
	OFX_LLAMACPP_CUBLAS_DIR ?= $(shell ldconfig -p 2>/dev/null | awk '/libcublas\.so/{print $$NF; exit}' | xargs -r dirname)

	# any special flag that should be passed to the linker when using this addon, also used for system libraries with -lname
	ADDON_LDFLAGS = -lpthread -fopenmp
	ADDON_LIBS = libs/llama.cpp/lib/linux64/libllama.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-cpu.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-base.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcommon.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcpp-httplib.a
	ADDON_CPPFLAGS += $(if $(filter 1,$(OFX_LLAMACPP_USE_CUDA)),-DOFX_LLAMACPP_USE_CUDA)
	ADDON_LIBS += $(if $(filter 1,$(OFX_LLAMACPP_USE_CUDA)),libs/llama.cpp/lib/linux64/libggml-cuda.a)
	ADDON_LIBS += $(if $(filter 1,$(OFX_LLAMACPP_USE_CUDA)),libs/llama.cpp/lib/linux64/libggml-blas.a)
	ADDON_LDFLAGS += $(if $(filter 1,$(OFX_LLAMACPP_USE_CUDA)),-L$(OFX_LLAMACPP_CUDA_LIB_DIR_1) -L$(OFX_LLAMACPP_CUDA_LIB_DIR_2) -L$(OFX_LLAMACPP_CUDART_DIR) -L$(OFX_LLAMACPP_CUBLAS_DIR))
	ADDON_LDFLAGS += $(if $(filter 1,$(OFX_LLAMACPP_USE_CUDA)),-lcudart -lcublas -lcublasLt -lcuda)

# --- Platform-specific configuration for LINUX (aarch64 / Raspberry Pi) ---
linuxaarch64:
	LLAMA_LIB_PATH = libs/llama.cpp/lib/linuxaarch64

	# aarch64 builds are CPU-first by default.
	ADDON_LDFLAGS = -lpthread -fopenmp

	# Static libraries for linuxaarch64 - ORDER MATTERS!
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libllama.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-cpu.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-base.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libcommon.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libcpp-httplib.a

# --- Platform-specific configuration for MACOS ---
osx:
	# Library path for macOS (assuming Apple Silicon arm64)
	LLAMA_LIB_PATH = libs/llama.cpp/lib/osx-arm64
	
	# Compiler flags for macOS to enable Metal and Accelerate
	ADDON_CPPFLAGS += -DGGML_USE_METAL -DGGML_METAL_NDEBUG -DGGML_USE_ACCELERATE

	# Linker flags for macOS
	ADDON_LDFLAGS += -ObjC

	# Static libraries for macOS - ORDER MATTERS!
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libllama.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-metal.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-blas.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-cpu.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libggml-base.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libcommon.a
	ADDON_LIBS += $(LLAMA_LIB_PATH)/libcpp-httplib.a

	# System Frameworks for macOS
	ADDON_LIBS += -framework Accelerate
	ADDON_LIBS += -framework Foundation
	ADDON_LIBS += -framework Metal
	ADDON_LIBS += -framework MetalKit
	ADDON_LIBS += -framework CoreGraphics
