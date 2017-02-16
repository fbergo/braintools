
#ifndef IVS_ANIMATE_H
#define IVS_ANIMATE_H 1

#include <libip.h>
#include <gtk/gtk.h>

#define ANIM_SWEEP     0 
#define ANIM_ROT_Y     1
#define ANIM_WAVEFRONT 2
#define ANIM_GROWTH    3

void animate(GtkWidget *mainwindow, Scene *scene, int operation, int step);

#endif
