#include <libip.h>

typedef struct _pointrtwxfgsd {
  int x,y,z;
} PT;

#define ASSIGN(a,b,c,d) a.x=b; a.y=c; a.z=d
#define XPND(a) a.x,a.y,a.z

int main(int argc, char **argv) {
  double R[12],T[16];
  int i,j;

  double P[4], Q[4];

  PT a,b,c,d,e,f,g,h;

  ASSIGN(a,16, 16, 0);
  ASSIGN(b,100,12, 1);
  ASSIGN(c,153,17, 2);
  ASSIGN(d,158,55, 3);
  ASSIGN(e,9  ,174,3);
  ASSIGN(f,7  ,136,2);
  ASSIGN(g,159,175,1);
  ASSIGN(h,143,177,0);

  SolveRegistration(R,
		    XPND(a),XPND(b),XPND(c),XPND(d),
		    XPND(e),XPND(f),XPND(g),XPND(h));
  RTransform(T,R);

  for(i=0;i<12;i++)
    printf("%.4f ",R[i]);
  printf("\n");

  printf("homog trans = \n");
  for(i=0;i<4;i++) {
    for(j=0;j<4;j++) {
      printf("%.4f ",T[4*i+j]);
    }
    printf("\n");
  }

  P[0] = b.x; P[1] = b.y; P[2] = b.z; P[3] = 1;
  htrans(Q,P,T);
  printf("trans = %.4f %.4f %.4f %.4f\n",Q[0],Q[1],Q[2],Q[3]);

  P[0] = d.x; P[1] = d.y; P[2] = d.z; P[3] = 1;
  htrans(Q,P,T);
  printf("trans = %.4f %.4f %.4f %.4f\n",Q[0],Q[1],Q[2],Q[3]);

  P[0] = f.x; P[1] = f.y; P[2] = f.z; P[3] = 1;
  htrans(Q,P,T);
  printf("trans = %.4f %.4f %.4f %.4f\n",Q[0],Q[1],Q[2],Q[3]);

  P[0] = h.x; P[1] = h.y; P[2] = h.z; P[3] = 1;
  htrans(Q,P,T);
  printf("trans = %.4f %.4f %.4f %.4f\n",Q[0],Q[1],Q[2],Q[3]);

  return 0;
}
