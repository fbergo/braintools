
SUBDIRS = libip ivs aftervoxel cppmisc spineseg
CP = /bin/cp -f
RM = /bin/rm -f
INSTALL = /usr/bin/install
SHELL = /bin/bash

all: $(SUBDIRS) copybin

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

copybin:
	$(INSTALL) -d bin
	$(INSTALL) ivs/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp} bin
	$(INSTALL) aftervoxel/aftervoxel bin
	$(INSTALL) cppmisc/{splitmri,greg,braintag,t2comp} bin
	$(INSTALL) spineseg/spineseg bin
	$(INSTALL) -d bin/doc
	$(INSTALL) spineseg/man/*.1 bin/doc
	$(INSTALL) ivs/man/*.{1,5} bin/doc


clean:
	for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir clean ; done
	$(RM) bin/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp}
	$(RM) bin/{aftervoxel}
	$(RM) bin/{splitmri,greg,braintag,t2comp}
	$(RM) bin/spineseg




