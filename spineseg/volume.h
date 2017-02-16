/*

    SpineSeg
    http://www.liv.ic.unicamp.br/~bergo/spineseg
    Copyright (C) 2009-2011 Felipe Paulo Guazzi Bergo
    fbergo/at/gmail/dot/com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

// classes here: VolumeDomain BoundLocation LocationIterator Volume<T>

#ifndef VOLUME_H
#define VOLUME_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <bzlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include "ift.h"
#include "geometry.h"

using namespace std;


//! A 3D discrete domain
class VolumeDomain {

 public:

  int W,H,D,N,WxH;

  //! Constructor
  VolumeDomain() {
    tby = NULL;
    tbz = NULL;
    W = H = D = WxH = N = 0;
  }
  
  //! Constructor, creates a domain with given width, height and depth
  VolumeDomain(int w,int h,int d) {
    tby = NULL;
    tbz = NULL;
    resize(w,h,d);
  }

  //! Destructor
  virtual ~VolumeDomain() {
    if (tby) delete tby;
    if (tbz) delete tbz;
  }

  //! Resizes this domain
  virtual void resize(int w,int h,int d) {
    int i;

    W = w;
    H = h;
    D = d;
    N = W*H*D;
    WxH = W*H;
    if (tby!=NULL) delete tby;
    if (tbz!=NULL) delete tbz;
    if (N==0) {
      tby = NULL;
      tbz = NULL;
      return;
    }
    tby = new int[H];
    tbz = new int[D];
    for(i=0;i<H;i++) tby[i] = W*i;
    for(i=0;i<D;i++) tbz[i] = WxH*i;
  }

  //! Returns the linear address of voxel (x,y,z)
  int  address(int x,int y,int z) {
    return(x+tby[y]+tbz[z]);
  }

  //! Returns the linear address of \ref Location loc
  int  address(const Location &loc) {
    return(loc.X+tby[loc.Y]+tbz[loc.Z]);
  }

  //! Returns true if voxel (x,y,z) is within this domain
  bool valid(int x,int y,int z) {
    return((x>=0)&&(y>=0)&&(z>=0)&&(x<W)&&(y<H)&&(z<D));
  }

  //! Returns true if voxel (x,y,z) is within this domain
  bool valid(float x,float y,float z) {
    return((x>=0)&&(y>=0)&&(z>=0)&&(x<W)&&(y<H)&&(z<D));
  }

  //! Returns true if \ref Location loc is within this domain
  bool valid(const Location &loc) {
    return(valid(loc.X,loc.Y,loc.Z));
  }
  
  //! Returns the X component of the linear address a
  int  xOf(int a) { return( (a%WxH) % W ); }

  //! Returns the Y component of the linear address a
  int  yOf(int a) { return( (a%WxH) / W ); }

  //! Returns the Z component of the linear address a
  int  zOf(int a) { return( a / WxH ); }

  //! Returns the length of the diagonal segment (0,0,0)-(W,H,D)
  int diagonalLength() { return((int) (sqrt(W*W+H*H+D*D))); }

 private:
  int *tby, *tbz;
};

//! Point in bound discrete 3D space
class BoundLocation : public Location {
 public:
  //! Constructor
  BoundLocation() : Location() { bind(0,0,0); }

  //! Constructor
  BoundLocation(VolumeDomain &vd) : Location() {
    bind(vd);
  }
    
  //! Constructor
  BoundLocation(int w,int h,int d) : Location() {
    bind(w,h,d);
  }

  //! Copy constructor
  BoundLocation(BoundLocation &b) {
    (*this) = b;
  }

  virtual ~BoundLocation() { }

  //! Binds this location to a \ref VolumeDomain
  void bind(VolumeDomain &vd) {
    W = vd.W; H = vd.H; D = vd.D;
    postBind();
  }

  //! Binds this location to a (w,h,d) domain
  void bind(int w,int h,int d) {
    W = w; H = h; D = d;
    postBind();
  }

  //! Advances to the next location within the bound domain
  BoundLocation & operator++(int a) {
    ++X; 
    if (X==W) { X=0; ++Y; }
    if (Y==H) { Y=0; Z++; } 
    if (Z>=D) { X=0; Y=0; Z=D; } 
    return(*this);
  }

  //! Assignment operator
  BoundLocation & operator=(const BoundLocation &b) { 
    X=b.X; Y=b.Y; Z=b.Z; bind(b.W,b.H,b.D); return(*this); }

  //! Assignment operator
  BoundLocation & operator=(const Location &b) { 
    X=b.X; Y=b.Y; Z=b.Z; return(*this); }

  //! Returns a reference to the first location in the bound domain
  Location & begin() { return pb; }

  //! Returns a reference to the location after the last one in the bound domain
  Location & end() { return pe; }

  //! Prints this location to stdout.
  virtual void print() { cout << "(" << X << "," << Y << "," << Z << "), (" << W << "," << H << "," << D << ")" << endl; }


 private:
  int W,H,D;
  Location pb, pe;

  void postBind() {
    pb.set(0,0,0);
    pe.set(0,0,D);
  }

};

class LocationIterator : public Location {
 public:
  LocationIterator(int w,int h,int d) : Location() { W=w; H=h; D=d; }
  LocationIterator(VolumeDomain *vd) : Location() { W=vd->W; H=vd->H; D=vd->D; }
  void begin() { X=Y=Z=0; }

  bool end() { return(Z==D); }

  LocationIterator & operator++(int a) { ++X; if (X==W) { X=0; ++Y; if (Y==H) { Y=0; ++Z; } } return(*this); }

 private:
  int W,H,D;
};

template <class T> class Volume : public VolumeDomain {

 public:
  float dx,dy,dz;
  T lastmax;

  typedef BoundLocation iterator;
  
  Volume() : VolumeDomain() {
    data = 0;
    dx = dy = dz = 1.0;
    lastmax = 0;
  }

  Volume(int w,int h,int d, 
	 float pdx=1.0, float pdy=1.0,float pdz=1.0) :  VolumeDomain(w,h,d) {
    int i;

    dx = pdx;
    dy = pdy;
    dz = pdz;
    data = new T[N];
    if (!data) return;
    for(i=0;i<N;i++) data[i] = 0;
    lastmax = 0;

    anyi.bind(*this);
    ibegin = anyi.begin(); ibegin.bind(*this);
    iend   = anyi.end();   iend.bind(*this);
  }


  Volume(const char *filename) : VolumeDomain() {
    data = 0;
    lastmax = 0;
    readSCN(filename);
  }

  
  // copy constructor
  Volume(Volume<T> *src) {
    W = src->W;
    H = src->H;
    D = src->D;
    N = W*H*D;
    dx = src->dx;
    dy = src->dy;
    dz = src->dz;
    resize(W,H,D);
    data = new T[N];
    if (data != NULL)
      for(int i=0;i<N;i++)
	data[i] = src->data[i];
    lastmax = src->lastmax;
  }

  virtual ~Volume() {
    if (data!=NULL)  delete data;
  }

  iterator & begin() { return(ibegin); }
  iterator & end()   { return(iend); }

  void resize(int w,int h,int d) {
    VolumeDomain::resize(w,h,d);
    anyi.bind(*this);
    ibegin = anyi.begin();
    iend   = anyi.end();
    ibegin.bind(*this);
    iend.bind(*this);
  }

  void clear() {
    if (data!=NULL) delete data;
    data = 0;
    resize(0,0,0);
    szPartial = szTotal = szCompressed = 0;
    lastmax = 0;
  }

  int * getSizeTotalPtr() { return(&szTotal); }
  int * getSizePartialPtr() { return(&szPartial); }

  int getSizeTotal() {
    if (data == NULL) return 0;
    return szTotal;
  }

  int getSizePartial() {
    if (data == NULL) return 0;
    return szPartial;
  }

  int getSizeCompressed() {
    if (data == NULL) return 0;
    return szCompressed;
  }

  int writeSCN(const char *filename) {
    switch(sizeof(T)) {
    case 1: return(writeSCN(filename,8,false));
    case 2: return(writeSCN(filename,16,true));
    case 4: return(writeSCN(filename,32,true));
    default: return -1;
    }
  }

  int writeBzSCN(const char * filename, int bits, bool sign, int level) {
    if (bits < 0) {
      int64_t a = (int64_t) minimum();
      int64_t b = (int64_t) maximum();
      if (a>=0 && b<=255) { 
	bits = 8; sign = false;
      } else if (a>=-128 && b<=127) {
	bits = 8; sign = true;
      } else if (a>=0 && b<=65535) {
	bits= 16; sign = false;
      } else if (a>=-32768 && b<=32767) {
	bits = 16; sign = true;
      } else {
	bits = 32;
	sign = (a < 0);
      }
    }

    if (bits!=8 && bits!=16 && bits!=32) {
      cerr << "** writeSCN: invalid bits value\n\n";
      return -1;
    }

    if (level < 1) level = 1;
    if (level > 9) level = 9;

    FILE *f;
    BZFILE *bf;
    int berr, i, j;
    char out[256];

    f = fopen(filename, "w");
    if (f==NULL) return -1;

    bf = BZ2_bzWriteOpen(&berr, f, level, 0, 30);
    if (berr != BZ_OK) goto bzwfail;

    szTotal = N * (bits/8);
    szPartial = 0;
    szCompressed = 0;

    sprintf(out,"SCN\n%d %d %d\n%.4f %.4f %.4f\n%d\n",W,H,D,dx,dy,dz,bits);
    BZ2_bzWrite(&berr, bf, out, strlen(out));
    if (berr != BZ_OK) goto bzwfail;

    if (bits==8 && !sign) {      
      uint8_t *xd = new uint8_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 255 ? 255 : (uint8_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, W);
	szPartial += W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    } else if (bits==8 && sign) {
      int8_t *xd = new int8_t[W];
      int x;
      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -128 ? -128 : (x > 127 ? 127 : (int8_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, W);
	szPartial += W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    } else if (bits==16 && !sign) {
      uint16_t *xd = new uint16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 65535 ? 65535 : (uint16_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, 2*W);
	szPartial += 2*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    } else if (bits==16 && sign) {
      int16_t *xd = new int16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -32768 ? -32768 : (x > 32767 ? 32767 : (int16_t) x ));
	}
	BZ2_bzWrite(&berr, bf, xd, 2*W);
	szPartial += 2*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    } else if (bits==32 && !sign) {
      uint32_t *xd = new uint32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (uint32_t) x );
	}
	BZ2_bzWrite(&berr, bf, xd, 4*W);
	szPartial += 4*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    } else if (bits==32 && sign) {
      int32_t *xd = new int32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (int32_t) x;
	}
	BZ2_bzWrite(&berr, bf, xd, 4*W);
	szPartial += 4*W;
	if (berr != BZ_OK) goto bzwfail;
      }
      delete xd;
    }

    unsigned int bin, bout;
    BZ2_bzWriteClose(&berr, bf, 0, &bin, &bout);
    fclose(f);
    if (berr != BZ_OK) return -1;

    szTotal = szPartial = (int) bin;
    szCompressed = bout;

    return 0;

  bzwfail:
    BZ2_bzWriteClose(&berr, bf, 1, NULL, NULL);
    fclose(f);
    return -1;
  }

  int writeSCN(const char * filename, int bits, bool sign) {

    if (bits < 0) {
      int64_t a = (int64_t) minimum();
      int64_t b = (int64_t) maximum();
      if (a>=0 && b<=255) { 
	bits = 8; sign = false;
      } else if (a>=-128 && b<=127) {
	bits = 8; sign = true;
      } else if (a>=0 && b<=65535) {
	bits= 16; sign = false;
      } else if (a>=-32768 && b<=32767) {
	bits = 16; sign = true;
      } else {
	bits = 32;
	sign = (a < 0);
      }
    }

    if (bits!=8 && bits!=16 && bits!=32) {
      cerr << "** writeSCN: invalid bits value\n\n";
      return -1;
    }

    ofstream f;
    int i,j;

    szTotal = N * (bits/8);
    szPartial = szCompressed = 0;

    f.open(filename,ofstream::binary);
    if (!f.good()) return -1;
    
    f << "SCN\n" << W << " " << H << " " << D << "\n";
    f << dx << " " << dy << " " << dz << "\n";
    f << bits << "\n";

    if (bits==8 && !sign) {      
      uint8_t *xd = new uint8_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 255 ? 255 : (uint8_t) x ));
	}
	f.write( (char *) xd, 1 * W);
	szPartial += W;
      }
      delete xd;
    } else if (bits==8 && sign) {
      int8_t *xd = new int8_t[W];
      int x;
      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -128 ? -128 : (x > 127 ? 127 : (int8_t) x ));
	}
	f.write( (char *) xd, 1 * W);
	szPartial += W;
      }
      delete xd;
    } else if (bits==16 && !sign) {
      uint16_t *xd = new uint16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (x > 65535 ? 65535 : (uint16_t) x ));
	}
	f.write( (char *) xd, 2 * W);
	szPartial += 2*W;
      }
      delete xd;
    } else if (bits==16 && sign) {
      int16_t *xd = new int16_t[W];
      int x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int) data[i*W + j];
	  xd[j] = (x < -32768 ? -32768 : (x > 32767 ? 32767 : (int16_t) x ));
	}
	f.write( (char *) xd, 2 * W);
	szPartial += 2*W;
      }
      delete xd;
    } else if (bits==32 && !sign) {
      uint32_t *xd = new uint32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (x < 0 ? 0 : (uint32_t) x );
	}
	f.write( (char *) xd, 4 * W);
	szPartial += 4*W;
      }
      delete xd;
    } else if (bits==32 && sign) {
      int32_t *xd = new int32_t[W];
      int64_t x;

      for(i=0;i<H*D;i++) {
	for(j=0;j<W;j++) {
	  x = (int64_t) data[i*W + j];
	  xd[j] = (int32_t) x;
	}
	f.write( (char *) xd, 4 * W);
	szPartial += 4*W;
      }
      delete xd;
    }

    szCompressed = szPartial;

    f.close();
    return 0;
  }

  void readBzSCN(const char *filename) {
    FILE *f;
    BZFILE *bf;
    int i,w=0,h=0,d=0,state,bpp=0;
    float y[7];
    string x;
    char hdr[4],c;
    int berr,bn;

    clear();

    f = fopen(filename,"r");
    if (f==NULL) return;
    bf = BZ2_bzReadOpen(&berr, f, 0, 0, NULL, 0);
    if (berr != BZ_OK) goto bzrfail;

    bn = BZ2_bzRead(&berr,bf,hdr,4);
    if (bn!=4 || berr!=BZ_OK) goto bzrfail;

    if (strncmp("SCN\n",hdr,4)!=0) goto bzrfail;

    x.clear();    
    for(state=0;state!=7;) {
      bn=BZ2_bzRead(&berr,bf,&c,1);
      if (bn!=1 || berr!=BZ_OK) goto bzrfail;
      if (c==' ' || c=='\t' || c=='\n') {
	y[state] = atof(x.c_str());
	x.clear();
	state++;
      } else {
	x += c;
      }
      if (state==7 && c!='\n') { 
	do { bn=BZ2_bzRead(&berr,bf,&c,1); } while(c!='\n' && berr==BZ_OK); 
      }
    }
    if (berr != BZ_OK) goto bzrfail;
    w = (int) y[0];
    h = (int) y[1];
    d = (int) y[2];
    dx = y[3];
    dy = y[4];
    dz = y[5];
    bpp = (int) y[6];

    resize(w,h,d);
    data = new T[N];
    if (data==NULL) goto bzrfail;

    szTotal = N * (bpp/8);
    szPartial = 0;

    switch(bpp) {
    case 8:
      uint8_t *td8;
      td8 = new uint8_t[N];
      if (!td8) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td8[i*W],W);
	szPartial += W;
	if (bn!=W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td8[i]);
      delete td8;
      break;
    case 16:
      uint16_t *td16;
      td16 = new uint16_t[N];
      if (!td16) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td16[i*W],2*W);
	szPartial += 2*W;
	if (bn!=2*W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td16[i]);
      delete td16;
      break;
    case 32:
      int32_t *td32;
      td32 = new int32_t[N];
      if (!td32) goto bzrfail;
      for(i=0;i<H*D;i++) {
	bn=BZ2_bzRead(&berr,bf,&td32[i*W],4*W);
	szPartial += 4*W;
	if (bn!=4*W || (berr!=BZ_OK && berr!=BZ_STREAM_END)) goto bzrfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td32[i]);
      delete td32;
      break;
    }

    BZ2_bzReadClose(&berr, bf);
    fclose(f);
    return;

  bzrfail:
    //printf("berr = %d\n",berr);
    clear();
    BZ2_bzReadClose(&berr, bf);
    fclose(f);
    return;
  }

  void readSCN(const char *filename) {
    FILE *f;
    int i,w=0,h=0,d=0,c,state,bpp=0;
    float y[7];
    string x;
    char hdr[4];

    clear();

    f = fopen(filename,"r");
    if (f==NULL) return;
    if (fread(hdr,1,4,f)!=4) goto rfail;

    if (strncmp("BZh",hdr,3)==0) {
      fclose(f);
      readBzSCN(filename);
      return;
    }

    if (strncmp("SCN\n",hdr,4)!=0) goto rfail;

    x.clear();    
    for(state=0;state!=7;) {
      c = fgetc(f);
      if (ferror(f)) goto rfail;
      if (c==' ' || c=='\t' || c=='\n') {
	y[state] = atof(x.c_str());
	x.clear();
	state++;
      } else {
	x += c;
      }
      if (state==7 && c!='\n') { do { c=fgetc(f); } while(c!='\n' && !ferror(f)); }
    }
    if (ferror(f)) goto rfail;
    w = (int) y[0];
    h = (int) y[1];
    d = (int) y[2];
    dx = y[3];
    dy = y[4];
    dz = y[5];
    bpp = (int) y[6];

    resize(w,h,d);
    data = new T[N];
    if (data==NULL) goto rfail;

    szTotal = N * (bpp/8);
    szPartial = 0;

    switch(bpp) {
    case 8:
      uint8_t *td8;
      td8 = new uint8_t[N];
      if (!td8) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td8[i*W]), 1, W, f);
	szPartial += W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td8[i]);
      delete td8;
      break;
    case 16:
      uint16_t *td16;
      td16 = new uint16_t[N];
      if (!td16) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td16[i*W]), 1, 2*W, f);
	szPartial += 2*W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td16[i]);
      delete td16;
      break;
    case 32:
      int32_t *td32;
      td32 = new int32_t[N];
      if (!td32) goto rfail;
      for(i=0;i<H*D;i++) {
	fread( (void *) (&td32[i*W]), 1, 4*W, f);
	szPartial += 4*W;
	if (ferror(f)) goto rfail;
      }
      for(i=0;i<N;i++) data[i] = (T) (td32[i]);
      delete td32;
      break;
    }
    fclose(f);
    return;

  rfail:
    clear();
    fclose(f);
    return;
  }

  bool ok() { return(data!=0); }

  T & voxel(int x,int y,int z) { 
    #ifdef SAFE
    assert(address(x,y,z)>=0 && address(x,y,z)<N);
    #endif
    return(data[address(x,y,z)]); 
  }
  T & voxel(float x,float y,float z) { 
    #ifdef SAFE
    return(voxel((int)x,(int)y,(int)z));
    #else
    return(data[address((int)x,(int)y,(int)z)]);
    #endif
  }
  T & voxel(int a) { 
    #ifdef SAFE
    assert(a>=0 && a<N);
    #endif
    return(data[a]);
  }
  T & voxel(Location & loc) { 
    #ifdef SAFE
    assert(address(loc)>=0 && address(loc)<N);
    #endif
    return(data[address(loc)]);
  }

  T maximum(bool prevok=false) {
    if (prevok) return lastmax;
    T x = data[0];
    for(int i=0;i<N;i++) if (data[i] > x) x = data[i];
    lastmax = x;
    return x;
  }

  T maximum(double perc) {
    int i, pn = (int) (perc * N);
    T rmv;
    int *histo;
    rmv = maximum();
    histo = new int[rmv+1];
    for(i=0;i<=rmv;i++) histo[i] = 0;
    for(i=0;i<N;i++) histo[voxel(i)]++;
    for(i=1;i<=rmv;i++) histo[i] += histo[i-1];
    for(i=rmv;histo[i] > pn;i--) ;
    delete histo;
    return( (T) i );
  }

  T minimum() {
    T x = data[0];
    for(int i=0;i<N;i++) if (data[i] < x) x = data[i];
    return x;
  }

  double mean() {
    double sum=0.0, n;
    n = (double) N;
    for(int i=0;i<N;i++)
      sum += (data[i] / n);
    return sum;
  }

  double stdev() {
    return(stdev(mean()));
  }

  double stdev(double mean) {
    double sum=0.0, n;
    n = (double) N;
    for(int i=0;i<N;i++)
      sum += ( (data[i]-mean)*(data[i]-mean) ) / n;
    return(sqrt(sum));    
  }

  void gradient(int x,int y,int z,R3 &g) {
    int p[3],q[3];

    p[0] = x-1 < 0 ? x : x-1;
    p[1] = y-1 < 0 ? y : y-1;
    p[2] = z-1 < 0 ? z : z-1;

    q[0] = x+1 >= W ? x : x+1;
    q[1] = y+1 >= H ? y : y+1;
    q[2] = z+1 >= D ? z : z+1;

    g.X = (float) (voxel(q[0],y,z) - voxel(p[0],y,z));
    g.Y = (float) (voxel(x,q[1],z) - voxel(x,p[1],z));
    g.Z = (float) (voxel(x,y,q[2]) - voxel(x,y,p[2]));
  }

  void fill(T val) { for(int i=0;i<N;i++) data[i] = val; }

  /* algorithms */

  int max(const int a,const int b) { return((a>b)?a:b); }
  int min(const int a,const int b) { return((a<b)?a:b); }

  void incrementBorder(float radius=1.5) {
    SphericalAdjacency A(radius);
    iterator p;
    Location q;
    int i;

    for(p=begin();p!=end();p++) {
      if (voxel(p)!=0)
	for(i=0;i<A.size();i++) {
	  q = A.neighbor(p,i);
	  if (valid(q))
	    if (voxel(q)==0) {
	      voxel(p)++;
	      break;
	    }
	}
    }
  }
  
  Volume<char> * treePruning(Volume<char> *seeds) {
    BasicPQ *Q;
    Volume<int> *cost, *pred, *dcount;
    Volume<char> *seg, *B;
    SphericalAdjacency A6(1.0);
    int i,j,k,maxval,inf,p,c1,c2,dmax,pmax=0,gmax;
    Location q,r;
    queue<int> fifo;

    /* compute IFT */
    cost = new Volume<int>(W,H,D);
    pred = new Volume<int>(W,H,D);

    pred->fill(-1);

    maxval = (int) maximum();
    inf = maxval + 1;

    Q = new BasicPQ(N,maxval+1);

    for(i=0;i<N;i++) {
      if (seeds->voxel(i) != 0) {
	cost->voxel(i) = 0;
	Q->insert(i,0);
      } else {
	cost->voxel(i) = inf;
      }
    }

    while(!Q->empty()) {
      p = Q->remove();
      q.set( xOf(p), yOf(p), zOf(p) );
      for(i=0;i<A6.size();i++) {
	r = A6.neighbor(q,i);
	if (valid(r)) {
	  c1 = cost->voxel(r);
	  c2 = max( cost->voxel(q), this->voxel(r) );	  
	  if (c2 < c1) {
	    cost->voxel(r) = c2;
	    pred->voxel(r) = p;
	    Q->insert(address(r), c2);
	  }
	}
      }
    }

    delete cost;

     /* compute descendant count (old method) */
    dcount = new Volume<int>(W,H,D);
    B = this->border();
    for(i=0;i<N;i++) {
      if (B->voxel(i) != 0) {
	j = i;
	while(pred->voxel(j) != -1) {
	  dcount->voxel(pred->voxel(j))++;
	  j = pred->voxel(j);
	}
      }
    }
    
    /* find leaking points */
    for(i=0;i<N;i++) {
      if (B->voxel(i) == 1) {
	j = i;
	dmax = 0;
	while(pred->voxel(j) != -1) {
	  if (dcount->voxel(j) > dmax) {
	    dmax = dcount->voxel(j);
	    pmax = j;
	  }
	  j = pred->voxel(j);
	}
	k = pmax;
	gmax = this->voxel(k);
	while(pred->voxel(k) != -1) {
	  if ( (dcount->voxel(k) == dmax) && (gmax < this->voxel(k)) ) {
	    pmax = k;
	    gmax = this->voxel(k);
	  }
	  k = pred->voxel(k);
	}

	/*
	if (B->voxel(pmax) != 2)
	  cout << "prune at " << pmax << endl;
	*/
	B->voxel(pmax) = 2; // set leaking point
      }
    }

    /* prune trees */
    seg = new Volume<char>(W,H,D);

    for(i=0;i<N;i++) {
      if (pred->voxel(i) == -1) {
	seg->voxel(i) = 1;
	fifo.push(i);

	while(!fifo.empty()) {
	  j = fifo.front();
	  fifo.pop();
	  q.set( xOf(j), yOf(j), zOf(j) );
	  seg->voxel(j) = 1;

	  for(k=0;k<A6.size();k++) {
	    r = A6.neighbor(q,k);
	    if (valid(r))
	      if ( (pred->voxel(r) == j) && (B->voxel(r) != 2) )
		fifo.push(address(r));
	  }
	}
      }
    }

    delete pred;
    delete dcount;
    delete B;
    return seg;
  }

  Volume<char> * border() {
    Volume<char> *B;
    int i,j;
    B = new Volume<char>(W,H,D);
    if (D>=3)
      for(i=0;i<W;i++)
	for(j=0;j<H;j++)
	  B->voxel(i,j,0) = B->voxel(i,j,D-1) = 1;
    if (H>=3)
      for(i=0;i<W;i++)
	for(j=0;j<D;j++)
	  B->voxel(i,0,j) = B->voxel(i,H-1,j) = 1;
    if (W>=3)
      for(i=0;i<H;i++)
	for(j=0;j<D;j++)
	  B->voxel(0,i,j) = B->voxel(W-1,i,j) = 1;
    return B;
  }

  Volume<T> * isometricInterpolation() {
    float md;

    if (dx==dy && dy==dz) {
      Volume<T> *v;
      int i;
      v = new Volume<T>(W,H,D);
      v->dx = dx;
      v->dy = dy;
      v->dz = dz;
      for(i=0;i<N;i++)
	v->voxel(i) = this->voxel(i);
      return v;
    }

    md = (dx < dy) ? dx : dy;
    md = (dz < md) ? dz : md;
    return(interpolate(md,md,md));
  }

  Volume<T> * interpolate(float ndx, float ndy, float ndz) {
     Volume<T> *dest, *tmp;
     int value;
     Location Q,P,R;
     float walked_dist,dist_PQ;

     if (ndx==dx && ndy==dy && ndz==dz) {
       return(new Volume<T>(this));
     }

     dest = this;

     if (ndx != dx) {
       tmp = new Volume<T>( (int)((float)(dest->W-1)*dest->dx/ndx)+1,
			    dest->H,
			    dest->D);
       for(Q.X=0; Q.X < tmp->W; Q.X++) {
	 for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	   for(Q.Y=0; Q.Y < tmp->H; Q.Y++) {
	     
	     walked_dist = (float)Q.X * ndx;
	     P.X = (int)(walked_dist/dx);
	     P.Y = Q.Y; P.Z = Q.Z;
	     R.X = P.X + 1;
	     if (R.X >= dest->W) R.X = dest->W - 1;
	     R.Y = P.Y; R.Z = P.Z;
	     dist_PQ =  walked_dist - (float)P.X * dx;
	     
	     value = (int)((( dx - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float) dest->voxel(R)) / dx);
	     tmp->voxel(Q) = (T) value;
	   }
       }
       dest = tmp;
     }

     if (ndy != dy) {
       tmp = new Volume<T>(dest->W,
			   (int)(((float)dest->H-1.0) * dy / ndy)+1,
			   dest->D);
       for(Q.Y=0; Q.Y < tmp->H; Q.Y++)
	 for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	   for(Q.X=0; Q.X < tmp->W; Q.X++) {
	     
	     walked_dist = (float)Q.Y * ndy;
	     
	     P.X = Q.X;
	     P.Y = (int)(walked_dist/dy);
	     P.Z = Q.Z;
	     R.X = P.X;
	     R.Y = P.Y + 1;
	     if (R.Y >= dest->H) R.Y = dest->H - 1;
	     R.Z = P.Z;
	     dist_PQ =  walked_dist - (float)P.Y * dy;
	     
	     value = (int)(((dy - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float)(dest->voxel(R))) / dy);
	     tmp->voxel(Q) = (T) value;
	   }
       if (dest != this)
	 delete dest;
       dest=tmp;
     }

     if (ndz != dz) {
       tmp = new Volume<T>(dest->W,
			   dest->H,
			   (int)(((float)dest->D-1.0) * dz / ndz)+1);
       for(Q.Z=0; Q.Z < tmp->D; Q.Z++)
	 for(Q.Y=0; Q.Y < tmp->H; Q.Y++)
	   for(Q.X=0; Q.X < tmp->W; Q.X++) {
	     
	     walked_dist = (float)Q.Z * ndz;
	     
	     P.X = Q.X; P.Y = Q.Y;
	     P.Z = (int)(walked_dist/dz);
	     
	     R.X = P.X; R.Y = P.Y;
	     R.Z = P.Z + 1;
	     if (R.Z >= dest->D) R.Z = dest->D - 1;	     
	     dist_PQ =  walked_dist - (float)P.Z * dz;
	     
	     value = (int)(((dz - dist_PQ)*
			    (float)(dest->voxel(P)) +
			    dist_PQ * (float)(dest->voxel(R))) / dz);
	     tmp->voxel(Q) = (T) value;
	   }
       if (dest != this)
	 delete dest;
       dest=tmp;
     }
          
     dest->dx=ndx;
     dest->dy=ndy;
     dest->dz=ndz;
     return(dest);
  }

  // left/right, anterior/posterior, feet/head
  enum Orientation { sagittal_lr=0, sagittal_rl=1, 
		     coronal_ap=2, coronal_pa=3, 
		     axial_fh=4, axial_hf=5 };

  Volume<T> *changeOrientation(Orientation from, Orientation to) {
    T4 forw[6], rev[6], tmp, t;
    R3 a,b,c,c2;
    int i,x,y,z,nw,nh,nd,x1,y1,z1;
    double ndx,ndy,ndz;
    Volume<T> *out;

    // basic transformations
    forw[0].identity();
    forw[1].yrot(180.0);
    forw[2].yrot(-90.0);   
    forw[3].yrot(90.0);
    forw[4].xrot(90.0);  tmp.yrot(-90.0); forw[4] *= tmp;
    forw[5].xrot(-90.0); tmp.yrot(-90.0); forw[5] *= tmp;
    
    // compute inverse transforms
    for(i=0;i<6;i++) {
      rev[i] = forw[i];
      rev[i].invert();
    }
    
    t = forw[from];
    t *= rev[to];

    // fix translation
    a.set(0,0,0);
    c.set(W,H,D);
    b = t.apply(a);
    c2 = t.apply(c);
    
    x = min((int) b.X,(int) c2.X);
    y = min((int) b.Y,(int) c2.Y);
    z = min((int) b.Z,(int) c2.Z);

    tmp.translate(-x,-y,-z);
    t *= tmp;

    a.set(W,H,D);
    c.set(0,0,0);
    
    b = t.apply(a);
    c = t.apply(c);
    b -= c;

    nw = (int) ceil(fabs(b.X));
    nh = (int) ceil(fabs(b.Y));
    nd = (int) ceil(fabs(b.Z));

    {    
      R3 org(0,0,0), ux(1,0,0), uy(0,1,0), uz(0,0,1);
      org = t.apply(org);
      ux  = t.apply(ux);
      uy  = t.apply(uy);
      uz  = t.apply(uz);
      ux -= org;
      uy -= org;
      uz -= org;
      ndx = dx;
      ndy = dy;
      ndz = dz;
      if (fabs(ux.X) > 0.90) ndx = dx; else if (fabs(ux.Y) > 0.90) ndy = dx; else ndz = dx;
      if (fabs(uy.X) > 0.90) ndx = dy; else if (fabs(uy.Y) > 0.90) ndy = dy; else ndz = dy;
      if (fabs(uz.X) > 0.90) ndx = dz; else if (fabs(uz.Y) > 0.90) ndy = dz; else ndz = dz;
    }
    
    out = new Volume<T>(nw, nh, nd, ndx, ndy, ndz);
    t.invert();
    
    for(z=0;z<nd;z++)
      for(y=0;y<nh;y++)
	for(x=0;x<nw;x++) {
	  a.set(x,y,z);
	  b = t.apply(a);	
	  x1 = (int) b.X;
	  y1 = (int) b.Y;
	  z1 = (int) b.Z;
	  if (valid(x1,y1,z1))
	    out->voxel(x,y,z) = voxel(x1,y1,z1);
	}

    return out;
  }

  void transform(T4 &X) {
    transformNN(X);
  }

  Volume<T> *paddedTransformLI(T4 &X) {
    R3 corner[8],a;
    Volume<T> *tmp;
    int x[2],y[2],z[2];
    int nw,nh,nd,hz;
    int i,j,k,ix,iy,iz,jx,jy,jz;
    float ws,vs,w;
    float sq3;
    T4 A,IX;

    corner[0].set(0,0,0);
    corner[1].set(W-1,0,0);
    corner[2].set(0,H-1,0);
    corner[3].set(0,0,D-1);
    corner[4].set(W-1,H-1,0);
    corner[5].set(W-1,0,D-1);
    corner[6].set(0,H-1,D-1);
    corner[7].set(W-1,H-1,D-1);

    for(i=0;i<8;i++)
      corner[i] = X.apply(corner[i]);

    x[0] = x[1] = (int) (corner[0].X);
    y[0] = y[1] = (int) (corner[0].Y);
    z[0] = z[1] = (int) (corner[0].Z);    

    for(i=0;i<8;i++) {
      if (corner[i].X < x[0]) x[0] = (int) (corner[i].X);
      if (corner[i].X > x[1]) x[1] = (int) (corner[i].X);
      if (corner[i].Y < y[0]) y[0] = (int) (corner[i].Y);
      if (corner[i].Y > y[1]) y[1] = (int) (corner[i].Y);
      if (corner[i].Z < z[0]) z[0] = (int) (corner[i].Z);
      if (corner[i].Z > z[1]) z[1] = (int) (corner[i].Z);
    }

    hz = D/2;
    if (abs(z[1] - hz) > abs(hz - z[0])) {
      z[0] = hz - abs(z[1] - hz);
    } else {
      z[1] = hz + abs(hz - z[0]);
    }

    nw = x[1] - x[0] + 1;
    nh = y[1] - y[0] + 1;
    nd = z[1] - z[0] + 1;

    IX = X;
    A.translate(-x[0],-y[0],-z[0]);
    IX *= A;    
    IX.invert();

    tmp = new Volume<T>(nw,nh,nd,dx,dy,dz);

    sq3 = sqrt(3.0);

    for(k=0;k<nd;k++)
      for(j=0;j<nh;j++)
	for(i=0;i<nw;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);

	  ws = 0.0f;
	  vs = 0.0f;

	  for(iz=0;iz<2;iz++)
	    for(iy=0;iy<2;iy++)
	      for(ix=0;ix<2;ix++) {
		jx = ((int)floor(a.X)) + ix;
		jy = ((int)floor(a.Y)) + iy;
		jz = ((int)floor(a.Z)) + iz;
		if (valid(jx,jy,jz)) {
		  w = sq3 - sqrt( (jx-a.X)*(jx-a.X) +
				  (jy-a.Y)*(jy-a.Y) +
				  (jz-a.Z)*(jz-a.Z) );
		  vs += w * (this->voxel(jx,jy,jz)) ;
		  ws += w;
		}
	      }
	  
	  if (ws != 0.0f) {
	    vs /= ws;
	    tmp->voxel(i,j,k) = (T) vs;
	  }
	}
    return tmp;
  }

  void transformLI(T4 &X) {
    R3 a;
    T4 IX;
    int i,j,k,ix,iy,iz,jx,jy,jz;
    float ws,vs,w;
    Volume<T> *tmp;
    float sq3;

    tmp = new Volume<T>(W,H,D,dx,dy,dz);

    IX = X;
    IX.invert();
    
    sq3 = sqrt(3.0);

    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);

	  ws = 0.0f;
	  vs = 0.0f;

	  for(iz=0;iz<2;iz++)
	    for(iy=0;iy<2;iy++)
	      for(ix=0;ix<2;ix++) {
		jx = ((int)floor(a.X)) + ix;
		jy = ((int)floor(a.Y)) + iy;
		jz = ((int)floor(a.Z)) + iz;
		if (valid(jx,jy,jz)) {
		  w = sq3 - sqrt( (jx-a.X)*(jx-a.X) +
				  (jy-a.Y)*(jy-a.Y) +
				  (jz-a.Z)*(jz-a.Z) );
		  vs += w * (this->voxel(jx,jy,jz)) ;
		  ws += w;
		}
	      }
	  
	  if (ws != 0.0f) {
	    vs /= ws;
	    tmp->voxel(i,j,k) = (T) vs;
	  }
	}

    for(i=0;i<N;i++)
      this->voxel(i) = tmp->voxel(i);
    delete tmp;   
  }

  void transformNN(T4 &X) {
    R3 a;
    T4 IX;
    int i,j,k;
    Volume<T> *tmp;

    tmp = new Volume<T>(W,H,D,dx,dy,dz);

    IX = X;
    IX.invert();
    
    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  a.set(i,j,k);
	  a = IX.apply(a);
	  if (valid(a.X,a.Y,a.Z))
	    tmp->voxel(i,j,k) = this->voxel(a.X,a.Y,a.Z);
	}

    for(i=0;i<N;i++)
      this->voxel(i) = tmp->voxel(i);
    delete tmp;
  }

  Volume<T> * morphGradient3D(float radius) {

    Volume<T> *dest;
    dest = new Volume<T>(W,H,D,dx,dy,dz);
    int i,j,k,ii,jj,kk;
    int rq;
    T vmin, vmax, c;

    rq = (int) (radius*radius);

    for(k=0;k<D;k++)
      for(j=0;j<H;j++)
	for(i=0;i<W;i++) {
	  
	  vmin = vmax = voxel(i,j,k);

	  for(kk=-rq;kk<=rq;kk++)
	    for(jj=-rq;jj<=rq;jj++)
	      for(ii=-rq;ii<=rq;ii++)
		if (ii*ii + jj*jj + kk*kk <= rq)
		  if (valid(i+ii,j+jj,k+kk)) {
		    c = voxel(i+ii,j+jj,k+kk);
		    if (c < vmin) vmin = c; else if (c > vmax) vmax = c;
		  }
	  
	  dest->voxel(i,j,k) = vmax - vmin;
	}
      
    return dest;
  }

 private:
  T *data;
  iterator anyi,ibegin,iend;
  int szPartial, szTotal, szCompressed;

};

#endif
