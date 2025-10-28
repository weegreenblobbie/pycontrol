include config.mk

#-----------------------------------------------------------------------------
# Let users know verbose is an option.
#
ifeq ($(VERBOSE), 0)
    $(info Use make VERBOSE=1 to see full build commands.)
endif

TOPTARGETS := release clean real-clean

SUBDIRS := external src webapp

# Default target: build C++ things only.
all: external src

src: external

release: external src

clean real-clean: external src webapp

clean:
	@echo "$(CLEAN_COLOR)Cleaning$(RESET) $(shell pwd)"
	rm -rf coverage coverage.info

coverage: external src/camera_control/unit_tests_bin
	@echo "$(BUILD_COLOR)Generating Coverage Report$(RESET)"
	$(SILENT)src/camera_control/unit_tests_bin
	$(SILENT)lcov --quiet --capture --directory src --output-file coverage.info --no-external --rc geninfo_unexecuted_blocks=1
	$(SILENT)lcov --quiet --remove coverage.info '*/external/*' --output-file coverage.info --ignore-errors unused
	$(SILENT)mkdir -p coverage
	$(SILENT)genhtml --quiet coverage.info --output-directory coverage
	@echo "Coverage report generated at $(abspath coverage/index.html)"

src/camera_control/unit_tests_bin: external
	$(SILENT)$(MAKE) -C src clean
	$(SILENT)$(MAKE) -C src all COVERAGE=1 -j4

# Generic recursive make rule for subdirectories.
$(SUBDIRS):
	$(SILENT)$(MAKE) -C $@ $(if $(filter $@,$(MAKECMDGOALS)),all,$(if $(filter $(MAKECMDGOALS),$(TOPTARGETS)),$(MAKECMDGOALS),all))

.PHONY: all $(TOPTARGETS) $(SUBDIRS) coverage

# :mode=makefile:
ode=makefile:
