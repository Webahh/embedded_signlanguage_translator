#ifndef PD_ANCHORS_H
#define PD_ANCHORS_H

#include <stdint.h>

#define PD_ANCHOR_COUNT 2016U

typedef struct {
    float x;
    float y;
} pd_pp_point_t;

const pd_pp_point_t *PD_GetAnchor(uint32_t index);

#endif
