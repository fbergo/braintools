
GTKLIB = `pkg-config --libs gtk+-2.0`
GTKHDR = `pkg-config --cflags gtk+-2.0`
JPGLIB = `pkg-config --libs libjpeg`
JPGHDR = `pkg-config --cflags libjpeg`
PNGLIB = `pkg-config --libs libpng`
PNGHDR = `pkg-config --cflags libpng`

CC = gcc
#CFLAGS = -Wall -O2 $(GTKHDR) -I../libip
CFLAGS = -Wall -ggdb $(GTKHDR) $(JPGHDR) $(PNGHDR) -I../libip
LIB = $(GTKLIB) $(JPGLIB) $(PNGLIB) -L../libip -lip -lpthread -lz -lpng -lm

all: compile_libip aftervoxel

SRC = about.c app.c bg.c contour.c dicom.c dicomgui.c fio.c info.c labels.c measure.c msgbox.c orient.c render.c roi.c segment.c session.c tp.c undo.c util.c view2d.c view3d.c volumetry.c xport.c

OBJ = about.o app.o bg.o contour.o dicom.o dicomgui.o fio.o info.o labels.o measure.o msgbox.o orient.o render.o roi.o segment.o session.o tp.o undo.o util.o view2d.o view3d.o volumetry.o xport.o

HDR = about.h app.h bg.h contour.h dicom.h dicomgui.h fio.h info.h labels.h measure.h msgbox.h orient.h render.h roi.h segment.h session.h tp.h undo.h util.h view2d.h view3d.h volumetry.h xport.h

.c.o: $_ $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

aftervoxel: $(HDR) $(OBJ)
	$(CC) $(OBJ) -o aftervoxel $(LIB)

clean:
	rm -f aftervoxel *.o *~

compile_libip:
	$(MAKE) -C ../libip
