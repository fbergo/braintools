
SUBDIRS = libip ivs aftervoxel
CP = /bin/cp -f
RM = /bin/rm -f

all: $(SUBDIRS) copybin

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

copybin:
	$(CP) ivs/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp} bin
	$(CP) aftervoxel/aftervoxel bin

clean:
	for dir in $(SUBDIRS) ; do $(MAKE) -C $$dir clean ; done
	$(RM) bin/{ivs,dicom2scn,ana2scn,scn2ana,scntool,scncomp,aftervoxel}

