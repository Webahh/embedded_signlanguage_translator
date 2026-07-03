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

#define PALM_KEYPOINT_COUNT           	7U
#define PALM_REGRESSION_SIZE         	18U
#define PALM_DETECTION_COUNT       		2016U

#define PALM_PP_CONFIDENCE_THRESHOLD 	0.5f
#define PALM_PP_IOU_THRESHOLD        	0.4f
#define PALM_PP_MAX_CANDIDATES       	20U

#define PALM_BOX_X_CENTER_INDEX       	0U
#define PALM_BOX_Y_CENTER_INDEX       	1U
#define PALM_BOX_WIDTH_INDEX          	2U
#define PALM_BOX_HEIGHT_INDEX         	3U
#define PALM_BOX_KEYPOINT_OFFSET      	4U

typedef struct {
    float center_x;
    float center_y;
    float width;
    float height;
    float keypoints[PALM_KEYPOINT_COUNT][2];
    float score;
    float probability;
    uint16_t anchor_index;
} PalmDetection_TypeDef;

typedef struct {
    uint8_t positive_count;
    uint8_t negative_count;
    bool detected;
} PalmDetectionFilter_TypeDef;

typedef struct {
    float center_x;
    float center_y;
    float width;
    float height;
    float rotation;

    float corners[4][2]; // top left, top right, bottom right, bottom left
} HandROI_TypeDef;

bool PALM_Postprocess(const PalmNetworkOutput_TypeDef *network_output, PalmDetection_TypeDef *detection);
void PALM_UpdateDetectionFilter(PalmDetectionFilter_TypeDef *filter, bool detection_valid);
bool PALM_CreateLandmarkROI(const PalmDetection_TypeDef *detection, uint32_t frame_width,
							uint32_t frame_height, HandROI_TypeDef *roi);

#endif /* PALM_POSTPROCESSING_H */
