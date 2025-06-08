TOPTARGETS := all clean real-clean

SUBDIRS := common camera_control

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all $(TOPTARGETS) $(SUBDIRS)
