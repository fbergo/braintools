
#ifndef AFTERVOXEL_H
#define AFTERVOXEL_H 1

//#define TODEBUG 1

// everything and the kitchen sink

#include "about.h"
#include "app.h"
#include "bg.h"
#include "contour.h"
#include "dicom.h"
#include "dicomgui.h"
#include "fio.h"
#include "icons.h"
#include "info.h"
#include "labels.h"
#include "measure.h"
#include "msgbox.h"
#include "orient.h"
#include "render.h"
#include "roi.h"
#include "segment.h"
#include "session.h"
#include "tp.h"
#include "undo.h"
#include "util.h"
#include "view2d.h"
#include "view3d.h"
#include "volumetry.h"
#include "xport.h"

// export globals

#ifndef SOURCE_APP_C
#include <gtk/gtk.h>
extern GtkWidget    *mw, *cw;
extern Resources    res;
extern VolData      voldata;
extern ViewState    view;
extern GUI          gui;
extern NotifyData   notify;
extern LabelControl labels;
#endif

#ifndef SOURCE_VIEW2D_C
extern View2D v2d;
#endif

#ifndef SOURCE_VIEW3D_C
extern View3D v3d;
extern OctantCube ocube;
#endif

#ifndef SOURCE_INFO_C
extern PatientInfo patinfo;
#endif

#ifndef SOURCE_SESSION_C
extern AftervoxelSession session;
#endif

#ifndef SOURCE_ROI_C
extern ROIdata roi;
#endif

#ifndef SOURCE_ORIENT_C
extern OrientData orient;
extern const char OriKeys[10];
#endif

#endif
