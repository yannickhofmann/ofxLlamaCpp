# This file has been generated to make the ofxLlamaCpp addon compatible with the
# openFrameworks Makefile-based build system. The original addon only provided
# a CMake-based build setup.

meta:
	ADDON_NAME = ofxLlamaCpp
	ADDON_DESCRIPTION = openFrameworks wrapper for llama.cpp
	ADDON_AUTHOR = Yannick Hofmann
	ADDON_TAGS = "ai", "nlp", "llama"
	ADDON_URL = tba

common:
	# Addon dependencies, if any
	# ADDON_DEPENDENCIES = 

	# Include search paths
	ADDON_INCLUDES += libs/llama.cpp/include
	ADDON_INCLUDES += libs/minja/include

	# Source files
	ADDON_SOURCES = src/ofxLlamaCpp.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/gguf.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/ggml-opt.cpp
	ADDON_SOURCES += libs/llama.cpp/ggml/src/ggml-backend-reg.cpp

	# ggml sources

	# llama sources

	# common sources

	# Compiler flags
		ADDON_CPPFLAGS = -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -DGGML_SCHED_MAX_COPIES=4 -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -DGGML_USE_CPU
		ADDON_CPPFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libllama.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-cpu.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-base.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcommon.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcpp-httplib.a

linux64:
	# any special flag that should be passed to the linker when using this
	# addon, also used for system libraries with -lname
	ADDON_LDFLAGS = -lpthread -fopenmp
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libllama.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-cpu.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libggml-base.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcommon.a
	ADDON_LIBS += libs/llama.cpp/lib/linux64/libcpp-httplib.a
