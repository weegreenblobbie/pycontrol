# =========================================================================
# config.mk: Shared Project Configuration
# =========================================================================

MAKEFLAGS += --no-print-directory

# --- Verbose Build Switch ---
# This is the single source of truth for the verbose setting.
# Default to a quiet build (VERBOSE=0) unless overridden from the command line
# via `make VERBOSE=1`.
VERBOSE ?= 0

# Set the @ symbol for silencing commands based on the VERBOSE flag.
ifeq ($(VERBOSE), 0)
	SILENT = @
	DEVNULL = 2>&1 > /dev/null
else
	SILENT =
	DEVNULL =
endif

# Define project directories (optional but very useful)
ROOT_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
SRC_DIR := $(ROOT_DIR)src
EXTERNAL_DIR := $(ROOT_DIR)external

# Color defs.
ifneq (,$(shell which tput))
	BLUE    := $(shell tput setaf 4)
	BOLD    := $(shell tput bold)
	CYAN    := $(shell tput setaf 6)
	GREEN   := $(shell tput setaf 2)
	MAGENTA := $(shell tput setaf 5)
	ORANGE  := $(shell tput setaf 208) # 256-color terminal support needed
	PURPLE  := $(shell tput setaf 5)
	RED     := $(shell tput setaf 1)
	RESET   := $(shell tput sgr0)
else
	BLUE    :=
	BOLD    :=
	CYAN    :=
	GREEN   :=
	MAGENTA :=
	ORANGE  :=
	PURPLE  :=
	RED     :=
	RESET   :=
endif

BUILD_COLOR     := $(BOLD)$(BLUE)
CLEAN_COLOR     := $(BOLD)
CLONE_COLOR     := $(BOLD)$(PURPLE)
CONFIGURE_COLOR := $(BOLD)$(CYAN)
INSTALL_COLOR   := $(BOLD)$(MAGENTA)
LINK_COLOR      := $(BOLD)$(RED)

#-----------------------------------------------------------------------------
# Top level C++ build definitions.
#
AR = ar
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2 -g -MMD -MP
CPPINCLUDES = -I$(SRC_DIR) -I$(EXTERNAL_DIR)/include

LINKFLAGS = \
    -L$(EXTERNAL_DIR)/lib \
	-Wl,-rpath,$(abspath $(EXTERNAL_DIR)/lib)

LIBS = \
    -lgphoto2 \
    -lgphoto2_port \
    -lgphoto2_port \
    -lcactus_rt \
    -lcactus_tracing_embedded_perfetto_protos \
    -lquill \
	-lprotobuf \
	-lpthread \
	-lCatch2 \
	-lCatch2Main

%.o: %.cc $(MAKEFILE_LIST)
	@echo "$(BUILD_COLOR)Building$(RESET) $<"
	$(SILENT)$(CXX) $(CXXFLAGS) $(CPPINCLUDES) -c $< -o $@

# :mode=makefile:
