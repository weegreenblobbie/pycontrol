TOPTARGETS := all release clean real-clean test

SUBDIRS := common camera_control

$(TOPTARGETS): $(SUBDIRS)

camera_control: common

$(SUBDIRS):
	$(SILENT)$(MAKE) -C $@ $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)

.PHONY: all $(TOPTARGETS) $(SUBDIRS)
