
BIN = libip.a
AR  = ar

-include iconfig.minc

ifndef ICONFIG
all:
	./configure
	$(MAKE)
else
all: $(BIN)
endif

LOCAL_DEFS =

ifdef ARCH_SUN
LOCAL_DEFS += -DSPARC
endif

ifdef OS_LINUX
LOCAL_DEFS += -DLINUX
endif

ifdef OS_SOLARIS
LOCAL_DEFS += -DSOLARIS
endif

ifdef OS_BSD
LOCAL_DEFS += -DBSD
endif

CC = gcc
#CFLAGS = -I. -O3 -Wall -ggdb $(LOCAL_DEFS)
CFLAGS = -I. -O3 -Wall -fno-strict-aliasing $(LOCAL_DEFS)

HDR = ip_adjacency.h  ip_common.h   ip_iftalgo.h  ip_poly.h   ip_set.h \
      ip_volume.h     ip_avfilter.h ip_dfilter.h  ip_image.h  ip_queue.h \
      ip_task.h       libip.h       ip_color.h    ip_geom.h   ip_optlib.h \
      ip_scene.h      ip_visualization.h          ip_stack.h  ip_linsolve.h \
      ip_mem.h        mvf.h

.c.o: $< $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

OBJS = adjacency.o avfilter.o color.o common.o dfilter.o \
       geom.o iftalgo.o image.o linsolve.o poly.o \
       iptask.o mem.o mvf.o optlib.o queue.o scene.o set.o stack.o \
       volume.o visualization.o

ifdef NASM
OBJS += optlib_mmx.o
NASM_FLAGS += -f elf
.s.o: $<
	$(NASM) $(NASM_FLAGS) $< -o $@
endif

ifdef YASM
OBJS += optlib_mmx.o
YASM_FLAGS += -f elf
.s.o: $<
	$(YASM) $(YASM_FLAGS) $< -o $@
endif

libip.a: $(OBJS)
	$(AR) csr libip.a $(OBJS)

clean:
	rm -f $(OBJS) *~ libip.a iconfig.minc iconfig.h

lutest: libip.a lutest.c
	gcc -Wall -O3 -ggdb -c lutest.c -o lutest.o -I.
	gcc -Wall -O3 -ggdb lutest.o libip.a -o lutest

itest: libip.a itest.c
	gcc -Wall -O3 -ggdb itest.c -I. -L. -lip -lm -lz -o itest

pgm2glyph: pgm2glyph.c
	gcc -Wall -O3 pgm2glyph.c -o pgm2glyph

