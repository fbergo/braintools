
#ifndef ORIENT_H
#define ORIENT_H 1

#define ORIENT_SAGGITAL 0
#define ORIENT_CORONAL  1
#define ORIENT_AXIAL    2

#define ORIENT_UPPER     3
#define ORIENT_LOWER     4
#define ORIENT_LEFT      5
#define ORIENT_RIGHT     6
#define ORIENT_ANTERIOR  7
#define ORIENT_POSTERIOR 8

#define ORIENT_UNKNOWN   99

typedef struct _OrientData {
  int main;
  int mainorder[4]; /* N,E,S,W */
  int zlo,zhi;
} OrientData;

void orient_init();
void orient_dialog();


#endif
