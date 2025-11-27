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

	ADDON_INCLUDES += libs/minja/include


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
	# any special flag that should be passed to the linker when using this
	# addon, also used for system libraries with -lname
	ADDON_LDFLAGS = -lpthread -fopenmp -Wl,--no-as-needed -L/usr/local/cuda-12.4/targets/x86_64-linux/lib -lcudart -lcublas /lib/x86_64-linux-gnu/libcuda.so -Wl,--as-needed
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libllama.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-cpu.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-base.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcommon.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcpp-httplib.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-cuda.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-blas.a

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
