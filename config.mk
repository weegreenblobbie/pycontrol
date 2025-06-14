# =========================================================================
# config.mk: Shared Project Configuration
# =========================================================================

# --- Verbose Build Switch ---
# This is the single source of truth for the verbose setting.
# Default to a quiet build (VERBOSE=0) unless overridden from the command line
# via `make VERBOSE=1`.
VERBOSE ?= 0

# Set the @ symbol for silencing commands based on the VERBOSE flag.
ifeq ($(VERBOSE), 0)
    SILENT = @
else
    SILENT =
endif

# --- Global Project Settings ---
# It's good practice to put other shared variables here too.
AR = ar
CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2 -g -MMD -MP

# Define project directories (optional but very useful)
ROOT_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
SRC_DIR := $(ROOT_DIR)src

# Color defs.
ifneq (,$(shell which tput))
	GREEN  := $(shell tput setaf 2)
	BLUE   := $(shell tput setaf 4)
	BOLD   := $(shell tput bold)
	RESET  := $(shell tput sgr0)
else
	GREEN  :=
	BLUE   :=
	BOLD   :=
	RESET  :=
endif

# :mode=makefile: