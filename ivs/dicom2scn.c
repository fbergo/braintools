/* 

   dicom2scn
   Felipe Bergo - bergo@liv.ic.unicamp.br - 2002-2012

   limitations (mostly hardcoded defaults, easy to change on
                source-code level):

   slice images widths cannot be >4096 (8 bit data)
                                 >2048 (16 bit data)
                                 >1024 (32 bit data)

   individual DICOM file names cannot be longer than 512 characters
   maximum number of slices per volume: 1024

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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#define VERSION "2012.1"
int debug=0;
int negfixmode=2;
int endianflip=0;
int lexical=0;
int autoname=0;

#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

/* this is probably the cheapest dicom decoder ever */

typedef struct _dicom_field {
  int  group;
  int  key;
  int  len;
  char data[128];
  struct _dicom_field *next;
} DicomField;

typedef struct _dicom_vector {
  int nf;
  DicomField *fvec;
} DicomVector;

typedef struct _image_3d {
  int    W,H,D;
  int   *data;
  float  zslice, dx, dy, dz;
  int    seqnum;
  float  thickness;
  char   equip[256];
  char   modality[256];
  char   seriesid[256];
  char   patient[256];
} Image3D;

typedef struct _imglist {
  Image3D *I;
  struct _imglist *next;
} ImageList;


#include "dicomdict.h"

/* data structures */

DicomField * DFNew();
void         DFKill(DicomField *df);

Image3D    * Img3DNew(int w,int h,int d);
void         Img3DKill(Image3D *img);
void         Img3DDump16(FILE *f,Image3D *img);
int          Img3DCmp(Image3D *a, Image3D *b);
int          Img3DMin(Image3D *img);
int          Img3DMax(Image3D *img);

char *argv_prog, *argv_src;

/* single file processing */

Image3D *     ReadDicomSlice(char *filename, FILE *rf, int report);
int           ReadDicomField(FILE *f, DicomField **df);
DicomField *  DicomGetField(DicomField *H, int group, int key);
int           ConvInt32(unsigned char *x);
int           ConvInt16(unsigned char *x);
DicomVector * MakeDicomVector(DicomField *df);
void          DVKill(DicomVector *dv);

DicomField * DVGetFieldFirst(DicomVector *dv, int group, int key);
DicomField * DVGetFieldLast(DicomVector *dv, int group, int key);
DicomField * DVGetFieldNth(DicomVector *dv, int group, int key, int index);
DicomField * DVGetFieldByStackPos(DicomVector *dv, int group, int key, int stackpos);
int          DVFieldCount(DicomVector *dv, int group, int key);

const char * DicomDictQuery(int group, int key);

/* directory sorting and volume assembling */

void         DicomDirectoryConvert(char *inputsample, char *outputscn, 
				   int first,int last, int report);
int          DicomVolumeConvert(char *inputsample, char *outputscn, int report);

void         DicomDirectoryInfo(char *inputsample, int extract);
void         DicomDump(char *inputsample);
void         DicomDumpNew(char *inputsample, int withnames);

void         DicomNameSplit(char *isample, char *odirpath, int odsz, 
			    char *obasename);
int          DicomSeqInfo(int index, Image3D **vol,int first,int last,int extract);
void         DicomExtractSeq(int index,int b,int e);

void         FindSampleFileIfDirectory(char *sample);

/* utilities */
int   IntNormalize(int value,int omin,int omax,int nmin,int nmax);
int   slicenumber(const char *s);
int   looks_alphanumeric(unsigned char *s, int len);
void  autoname_transform(char *name, Image3D *sample);

/* ==================================================== */

int   IntNormalize(int value,int omin,int omax,int nmin,int nmax)
{
  float tmp;
  int   i;
  if ( (omax - omin) == 0) return 0;
  tmp = ((float)(value - omin)) / ((float)(omax - omin));
  i = nmin + (int)(tmp * ((float)(nmax - nmin)));
  return i;
}

DicomField * DFNew() {
  DicomField *df;
  df = (DicomField *) malloc(sizeof(DicomField));
  df->group = df->key = 0;
  df->data[0] = 0;
  df->len = 0;
  df->next = 0;
  return df;
}

void DFKill(DicomField *df) {
  int i,n;
  DicomField *p,**v;

  n=1;
  for(p=df->next;p!=NULL;p=p->next)
    n++;

  v = malloc(sizeof(DicomField *) * n);

  i=0; p=df;
  while(i<n && p!=NULL) {
    v[i] = p;
    p=p->next;
    i++;
  }

  n=i;
  for(i=0;i<n;i++)
    free(v[i]);
  free(v);
}

Image3D    * Img3DNew(int w,int h,int d) {
  Image3D *i;

  i = (Image3D *) malloc(sizeof(Image3D));
  i->W = w;
  i->H = h;
  i->D = d;
  i->data = (int *) malloc(sizeof(int) * w * h * d);
  i->zslice = 0.0;
  i->seqnum = 0;
  i->dx = i->dy = i->dz = 1.0;
  i->equip[0] = 0;
  i->modality[0] = 0;
  i->seriesid[0] = 0;
  i->patient[0] = 0;
  memset(i->data,0,sizeof(int)*w*h*d);
  return i;
}

void         Img3DKill(Image3D *img) {
  free(img->data);
  free(img);
}

int Img3DCmp(Image3D *a, Image3D *b) {
  if (a->W != b->W || a->H != b->H || a->D != b->D)
    return 1;
  if (a->dx != b->dx || a->dy != b->dy || a->dz != b->dz)
    return 2;
  return(memcmp(a->data,b->data,sizeof(int) * a->W * a->H *a->D));
}

int Img3DMin(Image3D *img) {
  int i,j,n,*p;
  j = img->data[0];
  n = img->W * img->H * img->D;
  p = &(img->data[1]);
  for(i=n;i>=0;i--,p++)
    if (*p < j) j = *p;
  return j;
}

int Img3DMax(Image3D *img) {
  int i,j,n,*p;
  j = img->data[0];
  n = img->W * img->H;
  p = &(img->data[1]);
  for(i=n;i>=0;i--,p++)
    if (*p > j) j = *p;
  return j;
}

double robust_atof(char *p) {
  while( (*p) < 33)
    ++p;
  return(atof(p));
}

Image3D * ReadDicomSlice(char *filename, FILE *rf, int report) {
  FILE *f;
  Image3D *I;
  char buf[4096], *p, q;
  DicomField *df, *col, *row, *zpos, *pixeldata, *bits, *spacing, *mfnum, *dz, *series, *patient;
  int i,j,k,a,n,dataoffset, bpp;
  short int *sbuf;
  int       *ibuf;
  DicomField *thickness, *manuf, *model, *institution, *modality, *seq1, *seq2, *seq3, *sc1, *sc2;
  DicomVector *dvec;
  int mf;
  long seekstart=132;
  float scale[2] = {0.0, 1.0};
  float *vadd, *vmul;

  union {
    short int i16;
    char  i8[2];
  } endiantest;

  //  printf("reading %s\n",filename);

  f = fopen(filename,"r");
  if (!f) {
    fprintf(stderr,"dicom2scn: unable to open file %s\n",filename);
    exit(1);
  }

  if (rf && report)
    fprintf(rf,"reading %s\n",filename);

  /* check if NEMA header is present (DICM signature at offset 128) */
  fseek(f,128,SEEK_SET);
  fread(buf,1,4,f);
  buf[4]=0;
  if (strcmp(buf,"DICM")!=0) {
    seekstart = 0;
    fseek(f,0,SEEK_SET); /* no signature */
    if (rf && report) fprintf(rf,"NEMA=n ");
  } else {
    if (rf && report) fprintf(rf,"NEMA=y ");
  }

  /* read all fields */
  df = NULL;
  while( ReadDicomField(f, &df) != 0 ) ;

  // big endian files will turn out a single field with huge length
  if (df!=NULL && df->next==NULL && (df->len & 0xff) == 0) {
    DFKill(df);
    fseek(f,seekstart,SEEK_SET);
    endianflip = 1;
    df = NULL;
    while( ReadDicomField(f, &df) != 0 ) ;
  }

  dvec = MakeDicomVector(df);
  if (dvec == NULL) {
    fprintf(stderr,"error allocating field vector, aborting.\n");
    return NULL;
  }

  //  printf("done A\n");

  patient     = DVGetFieldFirst(dvec, 0x0010, 0x0010);
  col         = DVGetFieldFirst(dvec, 0x0028, 0x0011);
  row         = DVGetFieldFirst(dvec, 0x0028, 0x0010);
  mfnum       = DVGetFieldFirst(dvec, 0x0028, 0x0008);
  zpos        = DVGetFieldFirst(dvec, 0x0020, 0x1041);
  dz          = DVGetFieldFirst(dvec, 0x0018, 0x0088);
  pixeldata   = DVGetFieldLast(dvec, 0x7fe0, 0x0010);
  bits        = DVGetFieldFirst(dvec, 0x0028, 0x0100);
  spacing     = DVGetFieldFirst(dvec, 0x0028, 0x0030);
  thickness   = DVGetFieldFirst(dvec, 0x0018, 0x0050);
  manuf       = DVGetFieldFirst(dvec, 0x0008, 0x0070);
  model       = DVGetFieldFirst(dvec, 0x0008, 0x1090);
  institution = DVGetFieldFirst(dvec, 0x0008, 0x0080);
  modality    = DVGetFieldFirst(dvec, 0x0008, 0x0060);
  seq3        = DVGetFieldFirst(dvec, 0x0018, 0x0024);
  seq2        = DVGetFieldFirst(dvec, 0x0018, 0x1030);
  seq1        = DVGetFieldFirst(dvec, 0x0008, 0x103e);
  sc1         = DVGetFieldFirst(dvec, 0x0028, 0x1052);
  sc2         = DVGetFieldFirst(dvec, 0x0028, 0x1053);
  series      = DVGetFieldFirst(dvec, 0x0020, 0x000E);

  if (rf && report) 
    fprintf(rf,"W=%d\tH=%d\tD=%d\tZ=%.4f\tbits=%d\tthickness=%.4f\tdz=%.4f",
	    col?atoi(col->data):-999,
	    row?atoi(row->data):-999,
	    mfnum?atoi(mfnum->data):1,
	    zpos?atof(zpos->data):-999,
	    bits?atoi(bits->data):-999,
	    spacing?atof(spacing->data):-999,
	    dz?atof(dz->data):-999);

  if (!col || !row || !pixeldata) {
    fprintf(stderr,"dicom2scn: at least one required field is missing in %s.\n", filename);
    return NULL;
  }

  mf = mfnum ? atoi(mfnum->data) : 1;

  I=Img3DNew(atoi(col->data),atoi(row->data),mf);
  if (zpos) {
    //    printf("[%s]\n",zpos->data);
    I->zslice = atof(zpos->data);
  }

  I->thickness = 0.0;
  if (thickness)
    I->thickness = atof(thickness->data);

  strcpy(I->equip,manuf?(manuf->data):"?");
  strcat(I->equip,"/");
  strcat(I->equip,model?(model->data):"?");
  strcat(I->equip,"/");
  strcat(I->equip,institution?(institution->data):"?");

  strcpy(I->modality,modality?(modality->data):"?");

  if (seq1!=NULL) {
    strcat(I->modality,"/");
    strcat(I->modality,seq1->data);
  } else if (seq2!=NULL) {
    strcat(I->modality,"/");
    strcat(I->modality,seq2->data);
  } else if (seq3!=NULL) {
    strcat(I->modality,"/");
    strcat(I->modality,seq3->data);
  }

  strcpy(I->patient,patient?(patient->data):"?");
  strcpy(I->seriesid,series?(series->data):"?");

  if (spacing) {
    printf("spacing=[%s]\n",spacing->data);
    strcpy(buf,spacing->data);
    p=strtok(buf,"\\");
    if (p) {
      I->dx = robust_atof(p);
      p = strtok(0,"\\");
      if (p) I->dy = robust_atof(p);
    }    
  }

  // slice separation for 3D multiframe files
  if (dz && mf>1) {
    I->dz = robust_atof(dz->data);
  } 

  dataoffset = atoi(pixeldata->data);
  fseek(f,dataoffset,SEEK_SET);

  bpp = 16;
  if (bits)
    bpp = atoi(bits->data);

  n = I->W * I->H * I->D;
  switch(bpp) {

  case 8: /* ??? */
    for(i=0;i<n;i+=I->W) {
      fread(buf, 1, I->W, f);
      for(j=0;j<I->W;j++)
	I->data[i+j] = (int) ((unsigned char) (buf[j]));
    }
    break;
  case 16: /* the usual case */
    endiantest.i16 = 0x100;
    sbuf = (short int *) buf;
    for(i=0;i<n;i+=I->W) {
      fread(buf, 2, I->W, f);

      if ((endiantest.i8[0]!=0) ^ endianflip) {
	for(j=0;j<I->W;j++) {
	  q = buf[2*j];
	  buf[2*j] = buf[2*j+1];
	  buf[2*j+1] = q;
	}
      }      

      for(j=0;j<I->W;j++)
	I->data[i+j] = (int) (sbuf[j]);
	
    }
    break;
  case 32: /* ??? */
    endiantest.i16 = 0x100;
    ibuf = (int *) buf;
    for(i=0;i<n;i+=I->W) {
      fread(buf, 4, I->W, f);

      if ((endiantest.i8[0]!=0) ^ endianflip) {
	for(j=0;j<I->W;j++) {
	  q = buf[4*j];
	  buf[4*j] = buf[4*j+3];
	  buf[4*j+3] = q;
	  q = buf[4*j+1];
	  buf[4*j+1] = buf[4*j+2];
	  buf[4*j+2] = q;
	}
      }

      for(j=0;j<I->W;j++)
	I->data[i+j] = ibuf[j];
		
    }
    break;
  }

  // intensity transform

  // single intercept/slope or non-multiframe, use a single intercept,slope pair for all data
  if (mf == 1 || DVFieldCount(dvec,0x0028,0x1052) < I->D || DVFieldCount(dvec,0x0028,0x1053) < I->D || 
      DVFieldCount(dvec,0x0020,0x9057) == 0) {
    if (sc1!=NULL) scale[0] = atof(sc1->data);
    if (sc2!=NULL) scale[1] = atof(sc2->data);
    //printf("normalization by %f, %f\n",scale[0], scale[1]);
    for(i=0;i<n;i++)
      I->data[i] = (int) (scale[0] + scale[1] * I->data[i]);
  } else {

    vadd = (float *) malloc(sizeof(float) * I->D);
    vmul = (float *) malloc(sizeof(float) * I->D);
    for(i=0;i<I->D;i++) {
      sc1 = DVGetFieldByStackPos(dvec,0x0028,0x1052,i);
      sc2 = DVGetFieldByStackPos(dvec,0x0028,0x1053,i);
      vadd[i] = atof(sc1->data);
      vmul[i] = atof(sc2->data);
    }

    for(k=0;k<I->D;k++) {
      //printf("slice %d, intercept=%.4f slope=%.4f\n",k,vadd[k],vmul[k]);
      if (vadd[k] == 0.0f && vmul[k] == 1.0f) continue;

      for(j=0;j<I->H;j++)
	for(i=0;i<I->W;i++) {
	  a = i+j*(I->W)+k*(I->W*I->H);
	  I->data[a] = (int) (vadd[k] + vmul[k] * I->data[a]);
	} // for i
    } // for k
  }

  fclose(f);
  DFKill(df);
  DVKill(dvec);

  if (rf && report) fprintf(rf,"\n");
  return(I);
}

typedef struct _vr {
  char id[3];
  int  maxl;
  int  fixed;
  int  longlen;
} VR;

/* tipos de dados dos campos aceitos pelo DICOM 3 */
VR DicomVR[26] = {
  { "AE",    16, 0 , 0 }, // 0
  { "AS",     4, 1 , 0 },
  { "AT",     4, 1 , 0 }, // 2
  { "CS",    16, 0 , 0 },
  { "DA",     8, 1 , 0 }, // 4
  { "DS",    16, 0 , 0 },
  { "DT",    26, 0 , 0 }, // 6
  { "FL",     4, 1 , 0 },
  { "FD",     8, 1 , 0 }, // 8
  { "IS",    12, 0 , 0 },
  { "LO",    64, 0 , 0 }, // 10
  { "LT", 10240, 0 , 0 },
  { "OB",     0, 0 , 1 }, // 12
  { "OW",     0, 0 , 1 },
  { "PN",    64, 0 , 0 }, // 14
  { "SH",    16, 0 , 0 },
  { "SL",     4, 1 , 0 }, // 16
  { "SQ",     0, 0 , 1 },
  { "SS",     2, 1 , 0 }, // 18
  { "ST",  1024, 0 , 0 },
  { "TM",    16, 0 , 0 }, // 20
  { "UI",    64, 0 , 0 },
  { "UL",     4, 1 , 0 }, // 22
  { "UN",     0, 0 , 1 },
  { "US",     2, 1 , 0 }, // 24
  { "UT",     0, 0 , 1 },
};

int ConvInt32(unsigned char *x) {
  int a,b,c,d;
  if (endianflip) {
    a = (int) (x[3]);
    b = (int) (x[2]);
    c = (int) (x[1]);
    d = (int) (x[0]);
  } else {
    a = (int) (x[0]);
    b = (int) (x[1]);
    c = (int) (x[2]);
    d = (int) (x[3]);
  }
  a = a + 256*b + 65536*c + 16777216*d;
  return a;
}

int ConvInt16(unsigned char *x) {
  int a,b;
  if (endianflip) {
    a = (int) (x[1]);
    b = (int) (x[0]);
  } else {
    a = (int) (x[0]);
    b = (int) (x[1]);
  }
  a = a + 256*b;
  return a;
}

DicomField * DicomGetField(DicomField *H, int group, int key) {
  for(;H;H=H->next)
    if (H->group == group && H->key == key)
      return H;
  return 0;
}

int ReadDicomField(FILE *f, DicomField **df) {
  DicomField *x;
  unsigned char b[128];
  int i;

  if (debug)
    printf("fpos=%d\n",(int)(ftell(f)));

  x = DFNew();
  if (fread(b,1,4,f)!=4) {
    DFKill(x);
    return 0;
  }

  x->group = ConvInt16(b);
  x->key   = ConvInt16(&b[2]);

  if (debug)
    printf("g:k = %.4x:%.4x\n",x->group,x->key);

  if (fread(b,1,4,f)!=4) {
    DFKill(x);
    return 0;
  }

  if (debug)
    printf("hit 1 b=[%c%c]-%x:%x\n",b[0],b[1],b[2],b[3]);

  for(i=0;i<26;i++) {
    if (b[0] == DicomVR[i].id[0] && b[1] == DicomVR[i].id[1]) {

      if (debug)
	printf("VR match: %c%c\n",b[0],b[1]);
      if (DicomVR[i].longlen) {
	if (fread(b,1,4,f)!=4) {
	  DFKill(x);
	  return 0;
	}
	x->len = ConvInt32(b);
	if (debug)
	  printf("longlen=%d (0x%.8x) %d %d %d %d\n",x->len, x->len, b[0],b[1],b[2],b[3]);
      } else {
	x->len = ConvInt16(&b[2]);
	if (debug)
	  printf("shortlen=%d (0x%.4x)\n",x->len,x->len);
      }
      if (x->len > 126) {
	sprintf(x->data,"%d",(int)(ftell(f)));
	if (fseek(f,x->len,SEEK_CUR)!=0) {
	  DFKill(x);
	  return 0;
	}
      } else if (x->len <= 0) {
	// ignore SQs for now
	memset(x->data,0,128);
	x->len = 0;
	if (debug)
	  printf("ignoring SQ\n");
      } else {
	memset(b,0,128);
	if (fread(b,1,x->len,f)!=x->len) {
	  DFKill(x);
	  return 0;
	}
	if (i==16 || i==22) {
	  sprintf(x->data,"%d",(int)(ConvInt32(b)));
	} else if (i==18 || i==24) {
	  sprintf(x->data,"%d",(int)(ConvInt16(b)));
	} else {
	  memset(x->data,0,128);
	  memcpy(x->data,b,x->len);
	}
      }
      x->next = *df;
      *df     = x;
      return 1;
    }
  }

  //  printf("hit 3\n");

  // implicit VLs
  x->len = ConvInt32(&b[0]);
  if (debug)
    printf("LEN = %d\n",x->len);

  if (x->len > 126) {
    sprintf(x->data,"%d",(int)(ftell(f)));
    fseek(f, x->len, SEEK_CUR);
  } else if (x->len <= 0) {
    memset(x->data,0,128);
    x->len = 0;
    if (debug) printf("ignoring empty field\n");
  } else {
    if (fread(b,1,x->len,f)!=x->len) {
      DFKill(x);
      return 0;
    }

    memset(x->data,0,128);
    if ((x->len!=2 && x->len!=4) || looks_alphanumeric(b,x->len)) {
      memcpy(x->data,b,x->len);
    } else {
      if (x->len==2) sprintf(x->data,"%d",ConvInt16(b));
      if (x->len==4) sprintf(x->data,"%d",ConvInt32(b));
    }

  }
  
  x->next = *df;
  *df     = x;

  if (debug)
    printf("hit4\n");

  return 1;
}

int basenamelen = 0;

int slicecmp(const void *a, const void *b) {
  const char *sa, *sb;
  int na,nb;

  sa = (const char *) a;
  sb = (const char *) b;

  na = slicenumber(&sa[basenamelen]);
  nb = slicenumber(&sb[basenamelen]);
  if (na<nb) return -1;
  if (na>nb) return 1;
  return(strcmp(sa,sb));
}

int lexcmp(const void *a, const void *b) {
  const char *sa, *sb;
  sa = (const char *) a;
  sb = (const char *) b;
  return(strcmp(sa,sb));
}

int zcmp(const void *a, const void *b) {
  const Image3D **sa, **sb;
  float na,nb;
  int ma,mb;

  sa = (const Image3D **) a;
  sb = (const Image3D **) b;

  na = (*sa)->zslice;
  nb = (*sb)->zslice;
  ma = (*sa)->seqnum;
  mb = (*sb)->seqnum;
  if (na<nb) return -1;
  if (na>nb) return 1;
  if (ma<mb) return -1;
  if (ma>mb) return 1;
  return 0;
}

// for multi-frame DICOM files only (one volume per file)
int DicomVolumeConvert(char *inputsample, char *outputscn, int report) {
  Image3D *vol;
  FILE *rf=NULL;
  FILE *f;
  int j,k,minval, maxval, om;
  int fixneg=0, fixrange=0;

  if (report)
    rf = fopen("dicom.txt", "w");

  vol = ReadDicomSlice(inputsample, rf, report);

  if (vol==NULL) return 0;
  if (vol->D <= 1) {
    Img3DKill(vol);
    return 0;
  }

  printf("Patient: %s\n",vol->patient);
  printf("Study  : %s\n",vol->modality);
  printf("Equip  : %s\n",vol->equip);

  minval =  100000;
  maxval = -100000;
  k = vol->W * vol->H * vol->D;
  for(j=0;j<k;j++) {
    if (vol->data[j] < minval) minval = vol->data[j];
    if (vol->data[j] > maxval) maxval = vol->data[j];
  }

  if (negfixmode==2 && minval<0) {
    om = 0;
    fixneg = 1;
    k = vol->W * vol->H * vol->D;
    for(j=0;j<k;j++)
      vol->data[j] += -minval;
    om = -minval;
    minval = 0;
    maxval += -minval;
    printf("!! Negative values fixed, %d added to all intensities.\n",om);
  }
  if (negfixmode==1 && minval<0) {
    minval = 0;
    fixneg = 1;
    k = vol->W * vol->H * vol->D;
    for(j=0;j<k;j++)
      if (vol->data[j] < 0) vol->data[j] = 0;
    printf("!! Negative intensities set to 0.\n");
  }

  if (maxval - minval > 32767) {
    fixrange = 1;
    k = vol->W * vol->H * vol->D;
    for(j=0;j<k;j++)
      vol->data[j] = IntNormalize(vol->data[j],minval,maxval,0,32767);
  }

  if (fixrange)
    printf("!! Dynamic range > 32767 fixed by normalization.\n");

  f = fopen(outputscn,"w");
  if (!f) {
    fprintf(stderr,"dicom2scn: cannot open %s for writing.\n",outputscn);
    exit(3);
  }

  fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n16\n",
	  vol->W, vol->H, vol->D,
	  vol->dx, vol->dy, vol->dz);
  Img3DDump16(f, vol);
  fclose(f);

  Img3DKill(vol);

  if (rf && report)
    fclose(rf); 
  return 1;
}

void DicomDirectoryConvert(char *inputsample, char *outputscn, 
			   int first,int last, int report) 
{
  char dirpath[1024], basename[512], tmp[1024];
  char *farray;
  DIR *d;
  struct dirent *de;
  int i, j, k, nfiles = 0;
  Image3D   **vol;
  float mz, nz, dx, dy, dz;
  FILE *f;
  FILE *rf=0;
  int minval, maxval, om;
  int fixneg=0, fixrange=0;

  if (report)
    rf = fopen("dicom.txt", "w");

  DicomNameSplit(inputsample, dirpath, 1024, basename);
  basenamelen = strlen(basename);

  printf("\r reading directory...         ");
  fflush(stdout);

  d = opendir(dirpath);
  if (d==NULL) {
    fprintf(stderr,"dicom2scn: cannot open directory %s\n",dirpath);
    exit(3);
  }

  farray = (char *) malloc(512 * 16000);
  if (!farray) exit(5);
  
  while(( de = readdir(d) ) != 0) {
    if (strncmp(de->d_name, basename, basenamelen)==0) {

      i = slicenumber(&(de->d_name[strlen(basename)]));
      if (i<first || i>last)
	continue;

      memset(&farray[512*nfiles],0,512);      
      strncpy(&farray[512*nfiles],de->d_name,511);
      ++nfiles;
      if (nfiles >= 16000) {
	fprintf(stderr,"dicom2scn: more than 16000 files, skipping some.\n");
	break;
      }
    }
  }
  closedir(d);

  if (nfiles == 0) {
    fprintf(stderr,"dicom naming scheme looks incorrect, trying to convert all files in the directory.\n");
    d = opendir(dirpath);
    while(( de = readdir(d) ) != 0) {
      if (de->d_name[0] == '.') continue;
      // todo: stat and ignore anything that is not a regular file
      memset(&farray[512*nfiles],0,512);      
      strncpy(&farray[512*nfiles],de->d_name,511);
      ++nfiles;
      if (nfiles >= 16000) {
	fprintf(stderr,"dicom2scn: more than 16000 files, skipping some.\n");
	break;
      }
    }
    closedir(d);
  }
  
  if (nfiles == 0) {
    fprintf(stderr,"dicom2scn: unable to create file list from sample.\n");
    exit(3);
  }

  printf("\r numerically sorting slices...           ");
  fflush(stdout);

  /* sort slices according to the filesystem slice # */
  qsort(farray,nfiles,512,lexical?lexcmp:slicecmp);

  vol = (Image3D **) malloc(sizeof(Image3D *) * nfiles );

  /* read each slice's image */
  for(i=0;i<nfiles;i++) {
    sprintf(tmp,"%s/%s",dirpath,&farray[512*i]);
    //printf("file=[%s]\n",tmp);

    printf("\r reading slice %d of %d            ",i+1,nfiles);
    fflush(stdout);

    j = lexical ? (i+1) : atoi(&farray[512*i+strlen(basename)]);

    if (rf && report)
      fprintf(rf,"#=%d ",j);
    vol[i] = ReadDicomSlice(tmp, rf, report);
    if (vol[i]==NULL) { 
      memmove(&farray[512*i],&farray[512*(i+1)],(nfiles-i-1)*512);
      i--; nfiles--;
      continue; 
    }
    vol[i]->seqnum = j;

    for(k=0;k<i-1;k++) {
      if (vol[k]->zslice == vol[i]->zslice) {
	if (Img3DCmp(vol[k], vol[i])==0) {
	  printf("\nFound repeated slice, ignoring it and everything after it.\n");
	  nfiles = i+1;
	  break;
	} else {
	  printf("\nThere is Z overlapping, something is odd here...proceeding anyway.\n");
	}
      }
    }
    
    if (i>0) {
      if (vol[i]->W != vol[i-1]->W ||
	  vol[i]->H != vol[i-1]->H ||
	  vol[i]->D != vol[i-1]->D ||
	  vol[i]->dx != vol[i-1]->dx ||
	  vol[i]->dy != vol[i-1]->dy ||
	  vol[i]->dz != vol[i-1]->dz) {
	fprintf(stderr,"\n** Error: slice parameter (width, height, dx and/or dy) discontinuity from slice %d to slice %d. Stop.\n(try bounding the range with -b and -e, most likely this DICOM sequence contains several volumes at different sizes/modalities)\n\n", j-1, j);
	exit(2);
      }
    }
    
  }

  printf("\r positionally sorting slices...           ");
  fflush(stdout);

  /* sort slices according to the z position */
  qsort(vol, nfiles, sizeof(Image3D *), zcmp);

  /* find out dz */
  mz = nz = 0.0;

  for(i=0;i<nfiles-1;i++) {
    mz += fabs(vol[i]->zslice - vol[i+1]->zslice);
    nz++;
  }

  dx = vol[0]->dx;
  dy = vol[0]->dy;
  dz = mz / nz;

  printf("\r checking bounds...               ");
  fflush(stdout);
  minval =  100000;
  maxval = -100000;
  for(i=0;i<nfiles;i++) {
    k = vol[i]->W * vol[i]->H * vol[i]->D;
    for(j=0;j<k;j++) {
      if (vol[i]->data[j] < minval) minval = vol[i]->data[j];
      if (vol[i]->data[j] > maxval) maxval = vol[i]->data[j];
    }
  }
  printf("\r checking bounds: %d to %d\n",minval,maxval);
  fflush(stdout);

  if (negfixmode==2 && minval<0) {
    om = 0;
    fixneg = 1;
   
    for(i=0;i<nfiles;i++) {
      k = vol[i]->W * vol[i]->H *vol[i]->D;
      for(j=0;j<k;j++)
	vol[i]->data[j] += -minval;
    }
    om = -minval;
    minval = 0;
    maxval += -minval;
    printf("!! Negative values fixed, %d added to all intensities.\n",om);
  }

  if (negfixmode==1 && minval<0) {
    minval = 0;
    fixneg = 1;
    for(i=0;i<nfiles;i++) {
      k = vol[i]->W * vol[i]->H *vol[i]->D;
      for(j=0;j<k;j++)
	if (vol[i]->data[j]<0) vol[i]->data[j] = 0;
    }
    printf("!! Negative intensities set to 0.\n");
  }


  if (maxval - minval > 32767) {
    fixrange = 1;
    printf("\r fixing range...                ");
    fflush(stdout);
    for(i=0;i<nfiles;i++) {
      k = vol[i]->W * vol[i]->H * vol[i]->D;
      for(j=0;j<k;j++)
	vol[i]->data[j] = IntNormalize(vol[i]->data[j],minval,maxval,0,32767);
    }
  }

  if (fixrange)
    printf("\r!! Dynamic range > 32767 fixed by normalization.\n");

  printf("\r writing scn... 0%%               ");
  fflush(stdout);

  if (autoname)
    autoname_transform(outputscn, vol[0]);

  f = fopen(outputscn,"w");
  if (!f) {
    fprintf(stderr,"dicom2scn: cannot open %s for writing.\n",outputscn);
    exit(3);
  }

  fprintf(f,"SCN\n%d %d %d\n%.4f %.4f %.4f\n16\n",
	  vol[0]->W, vol[0]->H, nfiles,
	  dx, dy, dz);

  for(i=0;i<nfiles;i++) {
    printf("\r writing scn... %d%%              ",((i+1)*100)/nfiles);
    fflush(stdout);
    Img3DDump16(f, vol[i]);
  }

  printf("\r clean up                           ");
  fflush(stdout);

  fclose(f);

  for(i=0;i<nfiles;i++)
    Img3DKill(vol[i]);
  free(vol);

  if (rf && report)
    fclose(rf);

  printf("\r                             \r");
  fflush(stdout);
}

/* split full name isample into its directory, basename,
   and disregard the slice number */
void DicomNameSplit(char *isample, 
		    char *odirpath, int odsz, 
		    char *obasename) {
  char *p;
  int j;

  /* if isample has no /, make odirpath the current dir, 
     and obasename <- isample */
  if (!strchr(isample, '/')) {
    getcwd(odirpath, odsz);
    strcpy(obasename, isample);
  } else {
    /* otherwise, copy path to odirpath, rest to obasename */
    strcpy(odirpath, isample);
    p = strrchr(odirpath, '/');
    *p = 0;
    strcpy(obasename, p+1);
  } 

  j = strlen(obasename);

  /* .dcm files */
  /* remove slice#.dcm from obasename */
  if (j>4) {
    if (strcasecmp(&obasename[j-4],".dcm")==0) {
      p = &obasename[j-5];
      while(*p!='.') {
	--p;
	if (p==obasename) break;
      }
      *(p+1)=0;
      return;
    }
  }

  /* non-.dcm */
  /* remove slice number from obasename */
  p = &obasename[strlen(obasename)-1];
  while(*p >= '0' && *p <= '9')
    --p;
  *(p+1) = 0;

  return;
}

void DicomDumpNew(char *inputsample, int withnames) {
  FILE *f;
  DicomField *df;
  DicomVector *dv;
  char buf[256];
  int i;
  const char *fname;
  long seekstart=132;
  f = fopen(inputsample, "r");
  if (f==NULL) {
    fprintf(stderr, "unable to open %s\n",inputsample);
    return;
  }

  /* check if NEMA header is present (DICM signature at offset 128) */
  fseek(f,128,SEEK_SET);
  fread(buf,1,4,f);
  buf[4]=0;
  if (strcmp(buf,"DICM")!=0) {
    seekstart = 0;
    fseek(f,0,SEEK_SET); /* no signature */
  }

  /* read all fields */
  df = NULL;
  while( ReadDicomField(f, &df) != 0 ) ;

  // big endian files will turn out a single field with a huge length
  if (df!=NULL && df->next == NULL && (df->len & 0xff) == 0) {
    endianflip = 1;
    fseek(f,seekstart,SEEK_SET);
    DFKill(df);
    df = NULL;
    while( ReadDicomField(f, &df) != 0 ) ;
  }

  dv = MakeDicomVector(df);
  DFKill(df);

  if (withnames) {
    for(i=0;i<dv->nf;i++) {
      fname = DicomDictQuery(dv->fvec[i].group, dv->fvec[i].key);
      if (fname == NULL) fname = "?";
      printf("%.4x:%.4x {%s} (%d) [%s]\n",
	     dv->fvec[i].group, dv->fvec[i].key, fname, dv->fvec[i].len, dv->fvec[i].data);      
    }
  } else {
    for(i=0;i<dv->nf;i++) {
      printf("%.4x:%.4x (%d) [%s]\n",
	     dv->fvec[i].group, dv->fvec[i].key, dv->fvec[i].len, dv->fvec[i].data);
    }
  }
  fclose(f);
  DVKill(dv);
}

// dump all fields in a single file, in reverse order. Kept for compatibility with software that
// uses dicom2scn and expect this order
void DicomDump(char *inputsample) {
  FILE *f;
  DicomField *df, *dfi;
  char buf[256];
  long seekstart=132;
  f = fopen(inputsample, "r");
  if (f==NULL) {
    fprintf(stderr, "unable to open %s\n",inputsample);
    return;
  }

  /* check if NEMA header is present (DICM signature at offset 128) */
  fseek(f,128,SEEK_SET);
  fread(buf,1,4,f);
  buf[4]=0;
  if (strcmp(buf,"DICM")!=0) {
    seekstart = 0;
    fseek(f,0,SEEK_SET); /* no signature */
  }

  /* read all fields */
  df = NULL;
  while( ReadDicomField(f, &df) != 0 ) ;

  // big endian files will turn out a single field with a huge length
  if (df!=NULL && df->next == NULL && (df->len & 0xff) == 0) {
    endianflip = 1;
    fseek(f,seekstart,SEEK_SET);
    DFKill(df);
    df = NULL;
    while( ReadDicomField(f, &df) != 0 ) ;
  }

  for(dfi=df;dfi!=NULL;dfi=dfi->next) {
    printf("%.4x:%.4x (%d) [%s]\n",
	   dfi->group, dfi->key, dfi->len, dfi->data);
  }
  fclose(f);
}


/* read all dicoms and find out the sequences in there */
void DicomDirectoryInfo(char *inputsample, int extract) {
  char dirpath[1024], basename[512], tmp[1024];
  char *farray;
  DIR *d;
  struct dirent *de;
  int i,j,k, nfiles = 0;
  Image3D **vol;
  int heads[16000], nheads=0;
  int index;

  DicomNameSplit(inputsample, dirpath, 1024, basename);
  basenamelen = strlen(basename);

  printf("\r reading directory...          ");
  fflush(stdout);

  d = opendir(dirpath);
  if (d==NULL) {
    fprintf(stderr,"dicom2scn: cannot open directory %s\n",dirpath);
    exit(3);
  }

  farray = (char *) malloc(512 * 16000);
  if (!farray) exit(5);

  while(( de = readdir(d) ) != 0) {
    if (strncmp(de->d_name, basename, basenamelen)==0) {
      memset(&farray[512*nfiles],0,512);
      strncpy(&farray[512*nfiles],de->d_name,511);
      ++nfiles;
      if (nfiles >= 16000) {
	fprintf(stderr,"dicom2scn: over 16000 files in this directory, some files skipped.\n");
	break;
      }
    }
  }
  closedir(d);

  printf("\r numerically sorting slices...           ");
  fflush(stdout);

  /* sort slices */
  qsort(farray,nfiles,512,lexical?lexcmp:slicecmp);

  /* read slices */
  heads[0] = 0;
  nheads = 1;
  vol = (Image3D **) malloc(sizeof(Image3D *) * nfiles );

  for(i=0;i<nfiles;i++) {
    sprintf(tmp,"%s/%s",dirpath,&farray[512*i]);
    //printf("path=[%s]\n",dirpath);
    //printf("name=[%s]\n",&farray[512*i]);
    //printf("file=[%s]\n",tmp);

    printf("\r reading slice %d of %d            ",i+1,nfiles);
    fflush(stdout);

    j = lexical ? (i+1) : slicenumber(&farray[512*i+basenamelen]);
    
    vol[i] = ReadDicomSlice(tmp, 0, 0);

    if (vol[i]==NULL) {
      memmove(&farray[512*i],&farray[512*(i+1)],512*(nfiles-i-1));
      i--; nfiles--;
      continue;
    }
    vol[i]->seqnum = j;

    for(k=heads[nheads-1];k<i-1;k++) {
      if (vol[k]->zslice == vol[i]->zslice) {
	heads[nheads++] = i;
      }
    }

    if (i>0) {
      if (vol[i]->W != vol[i-1]->W ||
          vol[i]->H != vol[i-1]->H ||
          vol[i]->dx != vol[i-1]->dx ||
          vol[i]->dy != vol[i-1]->dy ||
	  vol[i]->zslice == vol[i-1]->zslice) {
	heads[nheads++] = i;
      }
    }
  }
  heads[nheads] = nfiles;
  
  printf("\r                                   \n");

  index = 0;
  for(i=0;i<nheads;i++)
    index += DicomSeqInfo(index, vol, heads[i], heads[i+1]-1,extract);

  for(i=0;i<nfiles;i++)
    Img3DKill(vol[i]);
  free(vol);
}

int DicomSeqInfo(int index, Image3D **vol, int first, int last,int extract) {
  int i;
  double mdz, cdz, dz, thickness;
  int b,e;
  int minval, maxval;

  b = vol[first]->seqnum;
  e = vol[last]->seqnum;

  if (e < b) return 0;

  /* sort slices according to the z position */
  qsort(&vol[first], last-first+1, sizeof(Image3D *), zcmp);

  mdz = 0.0;
  cdz = 0.0;
  for(i=first;i<last;i++) {
    mdz += fabs(vol[i+1]->zslice - vol[i]->zslice);
    cdz += 1.0;
  }

  dz = 1.0;
  if (cdz > 0.0)
    dz = mdz / cdz;

  mdz = 0.0;
  cdz = 0.0;
  for(i=first;i<=last;i++) {
    mdz += vol[i]->thickness;
    cdz += 1.0;
  }
  thickness = 0.0;
  if (cdz > 0.0)
    thickness = mdz /cdz;

  minval =  2000000;
  maxval = -2000000;
  for(i=first;i<=last;i++) { 
    minval = MIN(minval, Img3DMin(vol[i]));
    maxval = MAX(maxval, Img3DMax(vol[i]));
  }

  printf("[#%d] %dx%dx%d (%.2fx%.2fx%.2f) -- to extract: -b %d -e %d\n",
	 index+1, vol[first]->W, vol[first]->H, last-first+1, 
	 vol[first]->dx,vol[first]->dy,dz,b,e);
  printf("      range: [%d,%d] thickness: %.2f\n",minval,maxval,thickness);
  printf("      modality: [%s] equipment: [%s]\n",
	 vol[first]->modality,
	 vol[first]->equip);

  if (extract)
    DicomExtractSeq(index,b,e);
  printf("\n");

  return 1;
}

void DicomExtractSeq(int index,int b,int e) {
  int devnull,pid;

  char out[128];
  char bv[10],ev[10];

  sprintf(out,"dicom%.4d.scn",index+1);
  printf("      extracting as %s...",out); 
  fflush(stdout);

  devnull = open("/dev/null",O_WRONLY);

  pid = fork();

  if (!pid) {
    close(1);
    close(2);
    dup2(devnull, 1);
    dup2(devnull, 2);
    
    sprintf(bv,"%d",b);
    sprintf(ev,"%d",e);
    execlp(argv_prog,argv_prog,"-b",bv,"-e",ev,argv_src,out,NULL);
    return;

  } else {
    waitpid(pid,0,0);
    printf(" done\n");
  }

  close(devnull);
}

void Img3DDump16(FILE *f,Image3D *img) {
  short int *tmp;
  char *ctmp;
  int i,n,left;

  union {
    short int i16;
    char  i8[2];
  } endiantest;
  
  endiantest.i16 = 0x0100;

  n = (img->W) * (img->H) * (img->D);
  tmp = (short int *) malloc(2*n);
  
  for(i=0;i<n;i++)
    tmp[i] = (short int) img->data[i];

  /* flip endianness to little-endian if needed */
  if (endiantest.i8[0]) {
    for(i=0;i<n;i++)
      tmp[i] = (tmp[i] >>8 ) | ((tmp[i]&0xff)<<8);
  }

  /* write out */
  left = n * 2;
  ctmp = (char *) tmp;
  while(left) {
    i = fwrite(ctmp, 1, left > 16384 ? 16384 : left, f);
    if (i<=0) {
      fprintf(stderr,"dicom2scn: write error.\n");
      exit(4);
    }
    ctmp += i;
    left -= i;
  }

  free(tmp);
}

void  FindSampleFileIfDirectory(char *sample) {
  DIR *d;
  struct dirent *de;
  struct stat s;
  int i,j,k,m,z;
  char *x, emsg[128];
  char tmp[1024],xtmp[1024];

  if (stat(sample, &s)!=0) {
    switch(errno) {
    case ENOENT:
    case ENOTDIR:
      strcpy(emsg, "path or file not found.");
      break;
    case EACCES:
      strcpy(emsg, "permission denied.");
      break;
    default:
      sprintf(emsg, "errno=%d.",errno);
      break;
    }
    fprintf(stderr," unable to stat %s: %s\n",sample,emsg);
    exit(2);
  }
  if (!S_ISDIR(s.st_mode)) return;
  d = opendir(sample);
  if (!d) {
    fprintf(stderr," unable to read directory %s, errno=%d.\n",sample,errno);
    exit(2);
  }

  strcpy(tmp, sample);
  if (tmp[strlen(tmp)-1] != '/')
    strcat(tmp,"/");
  z = 0;

  while(( de = readdir(d) ) != 0) {
    x = de->d_name;
    j = strlen(x);
    k = 0;
    m = 1;

    sprintf(xtmp,"%s%s",tmp,x);
    if (stat(xtmp,&s)!=0) continue;
    if (!S_ISREG(s.st_mode)) continue;

    /* pattern 1: *.dcm */
    if (j>4) {
      if (strcasecmp(&x[j-4],".dcm")==0) {
	printf("[directory given, %s matches naming convention]\n",x);
	strcpy(sample, xtmp);
	z = 1;
	break;
      }
    }

    /* pattern 2: [A-Z]*[0-9]+ */
    for(i=0;i<j;i++) {
      if (k==0) {
	if (x[i] >= '0' && x[i] <= '9') { k=1; continue; }
	if (x[i] >= 'A' && x[i] <= 'z') { continue; }
	m = 0; break;
      }
      if (k==1) {
	if (x[i] < '0' || x[i] > '9') { m=0; break; }
      }
    }
    if (k==1 && m==1) {
      printf("[directory given, %s matches naming convention]\n",x);
      strcpy(sample, xtmp);
      z = 1;
      break;
    }
  }
  closedir(d);
  if (!z) {
    fprintf(stderr,"[directory given, but no suitable DICOM file found in it]\n");
    exit(4);
  }
}

void usage() {
    fprintf(stderr,"\nusage: dicom2scn [options] inputsample [outfile.scn]\n\n");
    fprintf(stderr,"  inputsample is ANY DICOM slice file, the other\n");
    fprintf(stderr,"  slices will be found automagically in the same\n");
    fprintf(stderr,"  directory (proper naming required). If you have a\n");
    fprintf(stderr,"  directory with ONLY DICOM files in it, you can\n");
    fprintf(stderr,"  provide just the directory name too.\n\n");
    fprintf(stderr,"options:\n");
    fprintf(stderr," -b n           start conversion at frame n [1]\n");
    fprintf(stderr," -e n           end conversion at frame n [last]\n");
    fprintf(stderr," -r             generate a dicom.txt report\n");
    fprintf(stderr," -n0            do not fix negative intensity values\n");
    fprintf(stderr," -n1            set negative intensity values to zero\n");
    fprintf(stderr," -n2            add offset to negative intentisites (default)\n");
    fprintf(stderr," -lex           sort DICOM filenames lexically instead of expecting slice numbers\n");
    fprintf(stderr," -p             treat output name as a prefix, and append patient/sequence data to it\n");
    fprintf(stderr," -D             dump DICOM tags of a single file, in direct order\n");
    fprintf(stderr," -DD            dump DICOM tags of a single file, in direct order, with field names\n");
    fprintf(stderr," -d             dump DICOM tags of a single file, in reverse order\n");
    fprintf(stderr," -i             print a description of the DICOM\n");
    fprintf(stderr,"                sequence(s) and exit.\n");
    fprintf(stderr," -xa            extract all volumes in this DICOM\n");
    fprintf(stderr," -h , --help    print this help\n\n");
    exit(1);
}

int main(int argc, char **argv) {
  char dicom[1024], scn[256];
  int first, last, i, j, wantreport, infomode, extract=0, dump=0;

  char *p;
  p = getenv("DEBUG");
  if (p!=NULL)
    if (atoi(p)!=0)
      debug = 1;

  first  = 1; 
  last   = 10000;
  wantreport = 0;
  infomode   = 0;
  j=0;
  strcpy(scn, "dicom.scn");

  argv_prog = argv[0];

  printf("dicom2scn - version %s - (c) 2002-2012 Felipe Bergo - fbergo@gmail.com\n",VERSION);

  for(i=1;i<argc;i++) {
    if (strcmp(argv[i], "-lex")==0) {
      lexical = 1;
      continue;
    }
    if (strcmp(argv[i], "-p")==0) {
      autoname = 1;
      continue;
    }
    if (strcmp(argv[i], "-b")==0) {
      if (i==argc-1) usage();
      first = atoi(argv[++i]);
      continue;
    }
    if (strcmp(argv[i], "-e")==0) {
      if (i==argc-1) usage();
      last = atoi(argv[++i]);
      continue;
    }
    if (strcmp(argv[i], "-r")==0) {
      wantreport = 1;
      continue;
    }
    if (strcmp(argv[i], "-n0")==0) {
      negfixmode = 0; 
      continue;
    }
    if (strcmp(argv[i], "-n1")==0) {
      negfixmode = 1; 
      continue;
    }
    if (strcmp(argv[i], "-n2")==0) {
      negfixmode = 2; 
      continue;
    }
    if (strcmp(argv[i], "-i")==0) {
      infomode = 1;
      extract = 0;
      continue;
    }
    if (strcmp(argv[i], "-d")==0) {
      dump = 1;
      continue;
    }
    if (strcmp(argv[i], "-D")==0) {
      dump = 2;
      continue;
    }
    if (strcmp(argv[i], "-DD")==0) {
      dump = 3;
      continue;
    }
    if (strcmp(argv[i], "-xa")==0) {
      infomode = 1;
      extract = 1;
      continue;
    }
    if (strcmp(argv[i], "-h")==0)
      usage();
    if (strcmp(argv[i], "--help")==0)
      usage();

    switch(j) {
    case 0:  strcpy(dicom, argv[i]); ++j; argv_src = argv[i]; break;
    case 1:  strcpy(scn, argv[i]); ++j; break;
    default: usage();
    }
  }

  if (j==0) usage();

  FindSampleFileIfDirectory(dicom);

  switch(dump) {
  case 1:
    DicomDump(dicom);
    return 0;
  case 2:
    DicomDumpNew(dicom,0);
    return 0;
  case 3:
    DicomDumpNew(dicom,1);
    return 0;
  }

  if (!infomode) {
    if (!DicomVolumeConvert(dicom,scn,wantreport))
      DicomDirectoryConvert(dicom,scn,first,last,wantreport);
  } else
    DicomDirectoryInfo(dicom,extract);
  
  return 0;
}

int slicenumber(const char *s) {
  int value = 0;
  const char *p;
  for(p=s;(*p)!=0;p++)
    if ((*p)>='0' && (*p)<='9')
      value = (10*value) + ((*p)-'0');
    else
      break;
  return value;
}

int looks_alphanumeric(unsigned char *s, int len) {
  int i;
  for(i=0;i<len;i++)
    if (s[i] < 32 || s[i] > 126)
      return 0;
  return 1;
}

DicomVector * MakeDicomVector(DicomField *df) {
  DicomVector *vec;
  int n,i;
  DicomField *p;
  vec = (DicomVector *) malloc(sizeof(DicomVector));

  for(n=0,p=df;p!=NULL;p=p->next)
    n++;

  vec->nf = n;
  vec->fvec = (DicomField *) malloc(sizeof(DicomField) * n);
  
  for(p = df, i = vec->nf - 1; i>=0; p=p->next, i--)
    memcpy(&(vec->fvec[i]), p, sizeof(DicomField));

  return vec;
}

DicomField * DVGetFieldFirst(DicomVector *dv, int group, int key) {
  int i;
  for(i=0;i<dv->nf;i++)
    if (dv->fvec[i].group == group && dv->fvec[i].key == key)
      return(&(dv->fvec[i]));
  return NULL;
}

DicomField * DVGetFieldLast(DicomVector *dv, int group, int key) {
  int i;
  for(i=dv->nf-1;i>=0;i--)
    if (dv->fvec[i].group == group && dv->fvec[i].key == key)
      return(&(dv->fvec[i]));
  return NULL;
}

DicomField * DVGetFieldNth(DicomVector *dv, int group, int key, int index) {
  int i,c=-1;
  for(i=0;i<dv->nf;i++)
    if (dv->fvec[i].group == group && dv->fvec[i].key == key) {
      c++;
      if (c==index)
	return(&(dv->fvec[i]));
    }
  return NULL;
}

DicomField * DVGetFieldByStackPos(DicomVector *dv, int group, int key, int stackpos) {
  int i,c=-1;
  for(i=0;i<dv->nf;i++) {
    if (dv->fvec[i].group == group && dv->fvec[i].key == key && c==stackpos)
	return(&(dv->fvec[i]));
    if (dv->fvec[i].group == 0x0020 && dv->fvec[i].key == 0x9057)
      c++;
  }
  return NULL;
}

int DVFieldCount(DicomVector *dv, int group, int key) {
  int i,c=0;
  for(i=0;i<dv->nf;i++)
    if (dv->fvec[i].group == group && dv->fvec[i].key == key)
      c++;
  return c;  
}

void DVKill(DicomVector *dv) {
  if (dv!=NULL) {
    if (dv->fvec != NULL)
      free(dv->fvec);
    free(dv);
  }
}

// todo: binary search
const char * DicomDictQuery(int group, int key) {
  int i,n;
  n = sizeof(dictionary) / sizeof(DictEntry);
  for(i=0;i<n;i++)
    if (dictionary[i].group == group && dictionary[i].key == key)
      return(dictionary[i].name);
  return NULL;
}

void autoname_transform(char *name, Image3D *sample) {
  char *p;
  char tmpname[512], tmp[512],aux[512];
  int i;
  // remove .scn extension if present
  strcpy(tmpname,name);
  p = strcasestr(tmpname, ".scn"); if (p!=NULL) *p=0;

  sprintf(tmp,"-%s-%s",
	  strlen(sample->patient) ? sample->patient : "X",
	  strlen(sample->modality) ? sample->modality : "X");

  for(i=0;i<strlen(tmp);i++)
    if (tmp[i] == '.' || tmp[i] == '/' || tmp[i]<=' ')
      tmp[i] = '_';

  sprintf(aux,"%s%s.scn",tmpname,tmp);
  i = 0;
  while(access(aux,F_OK) == 0) {
    i++;
    sprintf(aux,"%s%s-%d.scn",tmpname,tmp,i);
  }
  strcpy(name,aux);
}

