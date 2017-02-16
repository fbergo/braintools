
#ifndef DICOMGUI_H
#define DICOMGUI_H 1

#define ICON_HOME     0
#define ICON_HD       1
#define ICON_CD       2
#define ICON_NET      3
#define ICON_PENDRIVE 4
#define ICON_ZIP      5
#define ICON_MO       6

typedef struct _dicomdev {
  int icon;
  char label[32];
  char path[512];
} DicomDev;

void dicom_init();
void dicom_import_dialog();

GtkWidget * dev_button_new(DicomDev *dev);

#endif
