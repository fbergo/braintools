

CXX = c++
CFLAGS         = -Wall -O2 `./bi-config --cflags`
CFLAGS_GTK     = $(CFLAGS) `pkg-config --cflags gtk+-2.0`
LDFLAGS        = `./bi-config --libs`
LDFLAGS_GTK    = $(LDFLAGS) `pkg-config --libs gtk+-2.0`

PROGS = splitmri greg braintag

GREG_SRC = greg.cc greg_chs.cc greg_msgd.cc mygtk.cc
BI_H     = brain_imaging.h

all: $(PROGS)

splitmri: splitmri.cc $(BI_H)
	$(CXX) $(CFLAGS) splitmri.cc -o splitmri $(LDFLAGS)

greg: $(GREG_SRC) $(BI_H)
	$(CXX) $(CFLAGS_GTK) $(GREG_SRC) -o $@ $(LDFLAGS_GTK)

braintag: braintag.cc braintag_ui.h $(BI_H)
	$(CXX) $(CFLAGS_GTK) braintag.cc -o $@ $(LDFLAGS_GTK)

clean:
	/bin/rm -f $(PROGS) *~


