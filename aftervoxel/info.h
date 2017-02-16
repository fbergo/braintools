
#ifndef INFO_H
#define INFO_H 1

#define GENDER_M 0
#define GENDER_F 1
#define GENDER_U 2

/* these are kept in UTF-8 */
typedef struct _patinfo {
  char name[128];
  char age[16];
  char gender;
  char scandate[32];
  char institution[128];
  char equipment[128];
  char protocol[128];
  
  char *notes;
  int   noteslen;
} PatientInfo;

void info_init();
void info_reset();
void info_dialog();

void volinfo_dialog();

void histogram_dialog();

#endif
