

#ifndef XPORT_H
#define XPORT_H 1

#include <libip.h>

void xport_view_dialog();
void xport_bulk_dialog();
void xport_volume(int format);

int    CImgWriteJPEG(CImg *src, char *path, int quality);
CImg * CImgReadJPEG(char *path);
CImg * CImgPreviewJPEG(CImg *src,int quality,int gray,int width,int *psize);

int    CImgWritePNG(CImg *src, char *path);

#endif
