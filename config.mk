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
	DEVNULL = 2>&1 > /dev/null
else
	SILENT =
	DEVNULL =
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

# :mode=makefile: