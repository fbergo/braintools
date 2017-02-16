

CXX = c++
CFLAGS = -Wall -O2 `./bi-config --cflags`
LDFLAGS = `./bi-config --libs`

PROGS = splitmri

all: $(PROGS)


splitmri: splitmri.cc brain_imaging.h
	$(CXX) $(CFLAGS) splitmri.cc -o splitmri $(LDFLAGS)

clean:
	/bin/rm -f $(PROGS) *~

