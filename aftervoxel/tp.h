
#ifndef TP_H
#define TP_H 1

#include <libip.h>

Volume8  * tp_brain_marker_comp(Volume16 *orig);
Volume16 * tp_brain_otsu_enhance(Volume16 *orig);
Volume16 * tp_brain_feature_gradient(Volume16 *orig);
Volume8  * tp_brain_tree_pruning(Volume16 *grad, Volume8 *markers);

#endif
