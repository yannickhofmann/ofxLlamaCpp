################################################################################
# CONFIGURE PROJECT MAKEFILE (optional)
################################################################################

OF_ROOT = ../../..

PROJECT_DEFINES =

ifeq ($(TARGET_OS), osx)
	PROJECT_DEFINES += LLAMA_METAL=1
endif
