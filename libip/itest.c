#include "libip.h"

int main(int argc, char **argv) {

  CImg *z;

  z = CImgNew(512,512);

  CImgFillRect(z,0,0,512,512,0xffffff);

  CImgDrawCircle(z,256,256,60,0x0000ff);
  CImgDrawCircle(z,256,256,40,0xff8000);
  CImgFillCircle(z,256,256,20,0xff0000);

  CImgFillTriangle(z,10,10,100,100,20,80,0x008000);
  CImgDrawTriangle(z,10+5,10+5,100+5,100+5,20+5,80+5,0x000000);

  CImgDrawArrow(z,10,256,100,256,45,-10,0x800000);

  CImgDrawArrow(z,10,256,100,128,45,-10,0x800000);

  CImgDrawArrow(z,10,256,100,320,45,-10,0x800000);

  CImgWriteP6(z,"test.ppm");
  CImgDestroy(z);

  return 0;

}
