
SUBDIRS = libip ivs aftervoxel
CP = /bin/cp -f
RM = /bin/rm -f
INSTALL = /usr/bin/install

all: $(SUBDIRS) copybin

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

copybin:
	$(INSTALL) -d bin
	$(INSTALL) ivs/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp} bin
	$(INSTALL) aftervoxel/aftervoxel bin

clean:
	for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir clean ; done
	$(RM) bin/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp,aftervoxel}

