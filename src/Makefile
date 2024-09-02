TOPTARGETS := all clean real-clean

SUBDIRS := common camera_control control

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all $(TOPTARGETS) $(SUBDIRS)
