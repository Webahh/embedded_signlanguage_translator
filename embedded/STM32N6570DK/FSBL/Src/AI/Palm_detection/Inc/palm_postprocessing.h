/*
 * palm_postprocessing.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#ifndef PALM_POSTPROCESSING_H
#define PALM_POSTPROCESSING_H

#include <stdbool.h>
#include <stdint.h>

#include "palm_detection.h"

#define PALM_KEYPOINT_COUNT               7U
#define PALM_DETECTION_THRESHOLD_PERMILLE 500U
#define PALM_CONFIRM_FRAME_COUNT          2U

typedef struct {
    float center_x;
    float center_y;
    float width;
    float height;
    float keypoints[PALM_KEYPOINT_COUNT][2];
    float score;
    float probability;
    uint32_t anchor_index;
} PalmDetection_TypeDef;

typedef struct {
    uint8_t positive_count;
    uint8_t negative_count;
    bool detected;
} PalmDetectionFilter_TypeDef;

bool PALM_FindBestDetection(const PalmNetworkOutput_TypeDef *output, PalmDetection_TypeDef *detection);
void PALM_UpdateDetectionFilter(PalmDetectionFilter_TypeDef *filter, uint32_t probability_permille);

#endif /* PALM_POSTPROCESSING_H */
