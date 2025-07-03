include config.mk

#-----------------------------------------------------------------------------
# Let users know verbose is an option.
#
ifeq ($(VERBOSE), 0)
    $(info Use make VERBOSE=1 to see full build commands.)
endif

TOPTARGETS := all clean real-clean

SUBDIRS := external src webapp

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(SILENT)$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all $(TOPTARGETS) $(SUBDIRS)

# :mode=makefile: