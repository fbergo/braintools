# test

-include iconfig.minc

ifndef ICONFIG
all:
	./configure
	$(MAKE)
else
all: ivs dicom2scn scnsunfix specs ana2scn scn2ana scntool scnmask spm2mask scncomp 
endif

IVSVER=2.10
D2SVER=2011.1

CC = gcc

GTKFLAGS = `pkg-config --cflags gtk+-2.0`
IPFLAGS  = -I../libip
#GFLAGS   = -Wall -O2 -ggdb -DVERSION=\"$(IVSVER)\"
GFLAGS   = -Wall -O3 -ggdb -fno-strict-aliasing -DVERSION=\"$(IVSVER)\"
CFLAGS   = $(GFLAGS) $(GTKFLAGS) $(IPFLAGS)

GTKLIB   = `pkg-config --libs gtk+-2.0`
IPLIB    = -L../libip -lip -lpthread -lz -lm
LIBS     = $(GTKLIB) $(IPLIB)

# thread semaphores on solaris
ifdef OS_SOLARIS
LIBS += -lrt
endif

OBJS     = animate.o be.o ct.o filemgr.o fuzzy.o guiparser.o msgbox.o \
           onion.o preproc.o tasklog.o ivs.o util.o volumetry.o

HEADERS  = animate.h be.h ct.h filemgr.h fuzzy.h guiparser.h msgbox.h \
           onion.h preproc.h tasklog.h ivs.h util.h volumetry.h

libip:
	$(MAKE) -C ../libip

preproc.o: preproc.c preproc_ls.c preproc_gs.c preproc_mbgs.c preproc_thres.c preproc_gblur.c preproc_modal.c preproc_median.c preproc_mgrad.c preproc_dgrad.c preproc_sgrad.c preproc_interp.c preproc_crop.c preproc_plesion.c preproc_ssr.c preproc_reg.c preproc_homog.c preproc_morph.c preproc_arith.c

.c.o: $< $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

ivs: libip $(OBJS)
	$(CC) $(OBJS) -o ivs $(LIBS)

dicom2scn: dicom2scn.c dicomdict.h
	$(CC) -Wall -O2 dicom2scn.c -o dicom2scn -lm

ana2scn: ana2scn.c fio.c fio.h
	$(CC) -Wall ana2scn.c fio.c -o ana2scn

scn2ana: scn2ana.c fio.c fio.h
	$(CC) -Wall scn2ana.c fio.c -o scn2ana

scnmask: libip scnmask.c
	$(CC) $(IPFLAGS) -Wall -O3 scnmask.c -o scnmask $(IPLIB) -lm

spm2mask: libip spm2mask.c
	$(CC) $(IPFLAGS) -Wall -O3 spm2mask.c -o spm2mask $(IPLIB) -lm

scncomp: libip scncomp.c
	$(CC) $(IPFLAGS) -Wall -O3 scncomp.c -o scncomp $(IPLIB) -lm

scntool: scntool.c
	$(CC) -Wall -O3 scntool.c -o scntool -lm

scnsunfix: scnsunfix.c
	$(CC) -Wall -O3 scnsunfix.c -o scnsunfix

clean:
	rm -f *.o *~ ivs dicom2scn ana2scn scn2ana scnsunfix scntool
	rm -f iconfig.minc iconfig.h
	rm -f ivs-full.spec ivs-unicamp.spec ivs-web.spec

cleanall: clean
	$(MAKE) -C ../libip clean

SUBBER=while($$_=<>) { s/VERSION/$(IVSVER)/g; print "$$_"; }

specs:
	@perl -e '$(SUBBER)' <ivs-full.spec.in >ivs-full.spec
	@perl -e '$(SUBBER)' <ivs-unicamp.spec.in >ivs-unicamp.spec
	@perl -e '$(SUBBER)' <ivs-web.spec.in >ivs-web.spec
	@perl -e '$(SUBBER)' <dicom2scn.spec.in >dicom2scn.spec

binball:
	rm -rf tmpbin
	mkdir tmpbin
	mkdir tmpbin/bin
	mkdir tmpbin/man
	mkdir tmpbin/man/man1
	mkdir tmpbin/man/man5
	mkdir tmpbin/share
	mkdir tmpbin/share/icons
	cp -f ivs dicom2scn scn2ana ana2scn scnsunfix scntool tmpbin/bin
	cp -f man/*.1 tmpbin/man/man1
	cp -f man/*.5 tmpbin/man/man5
	cp -f xpm/ivs.xpm tmpbin/share/icons
	gzip tmpbin/man/man1/*
	gzip tmpbin/man/man5/*
	cd tmpbin ; tar zcvf ivs-$(IVSVER).tar.gz bin man share

rpminstall_dicom2scn: dicom2scn
	install -d $(DESTDIR)/usr/bin
	install dicom2scn $(DESTDIR)/usr/bin
	gzip man/*.1 man/*.5
	install -d $(DESTDIR)/usr/man/man1
	install man/dicom2scn.1.gz  $(DESTDIR)/usr/man/man1
	install -d $(DESTDIR)/usr/man/man5
	install man/scn.5.gz        $(DESTDIR)/usr/man/man5
	gzip -d man/*.1.gz man/*.5.gz

rpminstall: ivs dicom2scn ana2scn scn2ana scnsunfix scntool
	install -d $(DESTDIR)/usr/bin
	install ivs        $(DESTDIR)/usr/bin
	install minctoscn  $(DESTDIR)/usr/bin
	install dicom2scn  $(DESTDIR)/usr/bin
	install ana2scn    $(DESTDIR)/usr/bin
	install scn2ana    $(DESTDIR)/usr/bin
	install scntool    $(DESTDIR)/usr/bin
	install scnsunfix  $(DESTDIR)/usr/bin
	gzip man/*.1 man/*.5
	install -d $(DESTDIR)/usr/man/man1
	install man/ana2scn.1.gz    $(DESTDIR)/usr/man/man1
	install man/dicom2scn.1.gz  $(DESTDIR)/usr/man/man1
	install man/ivs.1.gz        $(DESTDIR)/usr/man/man1
	install man/scn2ana.1.gz    $(DESTDIR)/usr/man/man1
	install man/scntool.1.gz    $(DESTDIR)/usr/man/man1
	install -d $(DESTDIR)/usr/man/man5
	install man/mvf.5.gz        $(DESTDIR)/usr/man/man5
	install man/scn.5.gz        $(DESTDIR)/usr/man/man5
	gzip -d man/*.1.gz man/*.5.gz

dirtyinstall: ivs dicom2scn ana2scn scn2ana scnsunfix scntool
	rm -rf dirty
	mkdir  dirty
	install -d dirty/usr/bin
	install -d dirty/usr/doc
	install -d dirty/usr/man/man1
	install -d dirty/usr/man/man5
	install ivs             dirty/usr/bin
	install minctoscn       dirty/usr/bin
	install dicom2scn       dirty/usr/bin
	install ana2scn         dirty/usr/bin
	install scn2ana         dirty/usr/bin
	install scntool         dirty/usr/bin
	install scnsunfix       dirty/usr/bin
	install LICENSE         dirty/usr/doc
	install LICENSE.web     dirty/usr/doc
	make -C man psman
	install man/manpages.ps dirty/usr/doc/ivs-manpages.ps
	cp -f man/*.1 dirty/usr/man/man1
	cp -f man/*.5 dirty/usr/man/man5
	gzip dirty/usr/man/man1/*.1
	gzip dirty/usr/man/man5/*.5
	chmod 644 dirty/usr/man/man1/*.1.gz
	chmod 644 dirty/usr/man/man5/*.5.gz
	rm -rf ivs-$(IVSVER)-`uname -s`-binary
	mv -f dirty  ivs-$(IVSVER)-`uname -s`-binary
	tar zcvf ivs-$(IVSVER)-`uname -s`-binary.tar.gz ivs-$(IVSVER)-`uname -s`-binary
	rm -rf ivs-$(IVSVER)-`uname -s`-binary
	@echo wrote ivs-$(IVSVER)-`uname -s`-binary.tar.gz


tarball_dicom2scn:
	rm -rf dicom2scn-$(IVSVER)
	mkdir dicom2scn-$(IVSVER)
	mkdir dicom2scn-$(IVSVER)/ivs
	mkdir dicom2scn-$(IVSVER)/ivs/xpm
	mkdir dicom2scn-$(IVSVER)/ivs/man
	mkdir dicom2scn-$(IVSVER)/libip
	cp -f LICENSE LICENSE.web dicom2scn-$(IVSVER)
	cp -f Makefile configure iconfig.pl LICENSE LICENSE.web minctoscn *.{c,h,spec.in} dicom2scn-$(IVSVER)/ivs
	cp -f xpm/*.xpm dicom2scn-$(IVSVER)/ivs/xpm
	cp -f man/*.{1,5} man/Makefile dicom2scn-$(IVSVER)/ivs/man
	cp -f ../libip/*.{c,h,s} ../libip/Makefile ../libip/configure ../libip/iconfig.pl dicom2scn-$(IVSVER)/libip
	tar zcvf dicom2scn-$(IVSVER).tar.gz dicom2scn-$(IVSVER)
	rm -rf dicom2scn-$(IVSVER)

tarball:
	rm -rf ivs-$(IVSVER)
	mkdir ivs-$(IVSVER)
	mkdir ivs-$(IVSVER)/ivs
	mkdir ivs-$(IVSVER)/ivs/xpm
	mkdir ivs-$(IVSVER)/ivs/man
	mkdir ivs-$(IVSVER)/libip
	cp -f LICENSE LICENSE.web ivs-$(IVSVER)
	cp -f Makefile configure iconfig.pl LICENSE LICENSE.web minctoscn *.{c,h,spec.in} ivs-$(IVSVER)/ivs
	cp -f xpm/*.xpm ivs-$(IVSVER)/ivs/xpm
	cp -f man/*.{1,5} man/Makefile ivs-$(IVSVER)/ivs/man
	cp -f ../libip/*.{c,h,s} ../libip/Makefile ../libip/configure ../libip/iconfig.pl ivs-$(IVSVER)/libip
	tar zcvf ivs-$(IVSVER).tar.gz ivs-$(IVSVER)
	rm -rf ivs-$(IVSVER)

gplball:
	rm -rf dicom2scn-$(D2SVER) dicom2scn-$(D2SVER).tar.gz
	mkdir dicom2scn-$(D2SVER)
	mkdir dicom2scn-$(D2SVER)/man
	cp dicom2scn.c dicomdict.h ana2scn.c scn2ana.c scntool.c fio.h fio.c dicom2scn-$(D2SVER)
	cp man/scn.5 man/dicom2scn.1 man/ana2scn.1 man/scn2ana.1 man/scntool.1 dicom2scn-$(D2SVER)/man
	cp README.dicom2scn dicom2scn-$(D2SVER)/README
	cp Makefile.dicom2scn dicom2scn-$(D2SVER)/Makefile
	cp COPYING.dicom2scn dicom2scn-$(D2SVER)/COPYING
	tar zcvf dicom2scn-$(D2SVER).tar.gz dicom2scn-$(D2SVER)
	rm -rf dicom2scn-$(D2SVER)

