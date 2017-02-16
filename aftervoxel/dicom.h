
#ifndef DICOM_H
#define DICOM_H 1

#include <libip.h>

#define DICOM_FIELD_PATIENT     0
#define DICOM_FIELD_INSTITUTION 1
#define DICOM_FIELD_EQUIP       2
#define DICOM_FIELD_MODALITY    3
#define DICOM_FIELD_STUDYID     4
#define DICOM_FIELD_FILENAME    5
#define DICOM_FIELD_DATE        6
#define DICOM_FIELD_TIME        7
#define DICOM_FIELD_SOPUID      8

typedef struct _dicom_slice {
  int W,H,D;
  char *field[9];
  float xsize, ysize, zsize;

  int   echonumber;
  int   imgnumber;
  int   fakezpos;
  float zpos;
  float thickness;
  float slicedist;
  float origin[3];  

  float scale[2];

  unsigned int imgoffset;
  int bpp;
  int signedness;
  i16_t *image;
  i16_t minval, maxval;
} DicomSlice;

DicomSlice * DicomSliceNew();
void         DicomSliceCopy(DicomSlice *dest, DicomSlice *src);
void         DicomSliceDestroy(DicomSlice *ds);
void         DicomSliceSet(DicomSlice *ds, int i, const char *s);
char *       DicomSliceGet(DicomSlice *ds, int i);
void         DicomSliceLoadImage(DicomSlice *ds);
void         DicomSliceDiscardImage(DicomSlice *ds);
float        DicomSliceDistance(DicomSlice *a, DicomSlice *b);

int          DicomSliceIsDupe(DicomSlice *a,DicomSlice *b);

typedef struct _dicom_field {
  int  group;
  int  key;
  int  len;
  char data[256];
  struct _dicom_field *next;
} DicomField;

DicomField * DFNew();
void         DFKill(DicomField *df);

DicomSlice * ReadDicomSlice(const char *filename);
int          ReadDicomField(FILE *f, DicomField **df);
DicomField * DicomGetField(DicomField *H, int group, int key);
int          ConvInt32(unsigned char *x);
int          ConvInt16(unsigned char *x);
void         PrintDicomField(char *label, DicomField *df);
int          DicomLooksAlphanumeric(unsigned char *s, int len);

#endif
