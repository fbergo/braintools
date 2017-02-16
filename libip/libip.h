

#ifndef LIBIP_H
#define LIBIP_H

#ifdef __cplusplus
extern "C" {
#endif

/* memory allocation */
#include <ip_mem.h>

/* array allocation, time measurement */
#include <ip_common.h>

/* fast memory block operations */
#include <ip_optlib.h>

/* packed adjacency relation */
#include <ip_adjacency.h>

/* set data types: Set, Map and BMap */
#include <ip_set.h>

/* two priority queues: Queue and FQueue */ 
#include <ip_queue.h>

/* stack structure */ 
#include <ip_stack.h>

/* color manipulation (RGB<->HSV, RGB<->YCbCr) */
#include <ip_color.h>

/* 2D color image type - CImg */
#include <ip_image.h>

/* 3D volume type and Annotated Volume type */
#include <ip_volume.h>

/* 3D points and vectors, linear transforms, vector operations */
#include <ip_geom.h>

/* task/threaded operations */
#include <ip_task.h>

/* filters over annotated volumes */
#include <ip_avfilter.h>

/* filters over Volume structures */
#include <ip_dfilter.h>

/* ift segmentation algorithms */
#include <ip_iftalgo.h>

/* polynomials */
#include <ip_poly.h>

/* registration and linear system soving */
#include <ip_linsolve.h>

/* view scene */
#include <ip_scene.h>

/* 3d rendering */
#include <ip_visualization.h>

#ifdef __cplusplus
}
#endif


#endif
