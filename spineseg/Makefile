
CXX = c++
CFLAGS = -Wall -O2 `pkg-config --cflags gtk+-2.0` -Ixpm
LDFLAGS = -lbz2 -lpthread -lm `pkg-config --libs gtk+-2.0`

HDR = $(sort $(wildcard *.h))

all: spineseg

spineseg: spineseg.cc $(HDR)
	$(CXX) $(CFLAGS) spineseg.cc -o $@ $(LDFLAGS)

clean:
	/bin/rm -f *~ spineseg


