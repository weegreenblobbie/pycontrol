TOPTARGETS := all clean real-clean

SUBDIRS := $(wildcard */)

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all $(TOPTARGETS) $(SUBDIRS)
