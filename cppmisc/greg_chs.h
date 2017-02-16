
#ifndef GREG_CHS_H
#define GREG_CHS_H 1

#include "brain_imaging.h"

Volume<int> * chs_register1(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_register1x(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_register2(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_register2c(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_register(Volume<int> *a, Volume<int> *b,vector<T4> &tvec);
void          chs(Volume<int> *B, vector<int> &S, T4 &T, vector<T4> &tvec, Volume<int> *mov);
double        chs_eval(Volume<int> *B, vector<int> &S, Volume<int> *mov, T4 &T);

Volume<int> * chs_miregister1(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_miregister1x(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_miregister2(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_miregister2c(Volume<int> *a, Volume<int> *b);
Volume<int> * chs_miregister(Volume<int> *a, Volume<int> *b,vector<T4> &tvec);
void          mi_chs(Volume<int> *a, Volume<int> *b, T4 &T, vector<T4> &tvec);
double        mi_chs_eval(Volume<int> *a, Volume<int> *b, T4 &T);

Volume<int> * mat_register(Volume<int> *a, Volume<int> *b,T4 &mat);

#endif

