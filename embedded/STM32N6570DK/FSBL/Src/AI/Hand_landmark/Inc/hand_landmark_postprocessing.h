/*
 * hand_landmark_postprocessing.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#ifndef HAND_LANDMARK_POSTPROCESSING_H
#define HAND_LANDMARK_POSTPROCESSING_H

#include "hand_landmark.h"
#include "palm_postprocessing.h"

typedef struct {
    float x;
    float y;
    float z;
} LandmarkPoint_TypeDef;

void LANDMARK_MapToFrame(
    const LandmarkNetworkOutput_TypeDef *output,
    const HandROI_TypeDef *roi,
    LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT]);

#endif /* HAND_LANDMARK_POSTPROCESSING_H */
