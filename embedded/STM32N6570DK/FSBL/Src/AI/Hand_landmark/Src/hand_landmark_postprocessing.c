/*
 * hand_landmark_postprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include "hand_landmark_postprocessing.h"
#include <stddef.h>

void LANDMARK_MapToFrame(
    const LandmarkNetworkOutput_TypeDef *output,
    const HandROI_TypeDef *roi,
    LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT])
{
    if ((output == NULL) ||
        (roi == NULL) ||
        (points == NULL)) {
        return;
    }

    const float roi_left =
        roi->center_x - roi->width * 0.5f;

    const float roi_top =
        roi->center_y - roi->height * 0.5f;

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        const float crop_x =
            output->landmarks[i * 3U + 0U];

        const float crop_y =
            output->landmarks[i * 3U + 1U];

        const float crop_z =
            output->landmarks[i * 3U + 2U];

        points[i].x =
            roi_left +
            (crop_x / (float)LANDMARK_INPUT_WIDTH) *
            roi->width;

        points[i].y =
            roi_top +
            (crop_y / (float)LANDMARK_INPUT_HEIGHT) *
            roi->height;

        points[i].z = crop_z;
    }
}

