/*
 * palm_detection.h
 *
 *  Created on: 29.06.2026
 *      Author: Weber
 */

#ifndef PALM_DETECTION_H
#define PALM_DETECTION_H

#include <stdbool.h>
#include <stdint.h>

#include "simple_ai.h"

#define PALM_INPUT_WIDTH       			192U
#define PALM_INPUT_HEIGHT      			192U
#define PALM_INPUT_CHANNELS      		3U
#define PALM_INPUT_SIZE 				(PALM_INPUT_WIDTH * PALM_INPUT_HEIGHT * PALM_INPUT_CHANNELS)

#define PALM_DETECTION_COUNT  			2016U
#define PALM_REGRESSION_SIZE    		18U
#define PALM_SCORE_BUFFER_SIZE 			(PALM_DETECTION_COUNT * sizeof(float))
#define PALM_REGRESSION_BUFFER_SIZE 	(PALM_DETECTION_COUNT * PALM_REGRESSION_SIZE * sizeof(float))

#define PALM_KEYPOINT_COUNT           	7U
#define PALM_REGRESSION_SIZE         	18U
#define PALM_DETECTION_COUNT       		2016U

#define PALM_PP_CONFIDENCE_THRESHOLD 	0.3f
#define PALM_PP_IOU_THRESHOLD        	0.4f
#define PALM_PP_MAX_CANDIDATES       	20U

#define PALM_BOX_X_CENTER_INDEX       	0U
#define PALM_BOX_Y_CENTER_INDEX       	1U
#define PALM_BOX_WIDTH_INDEX          	2U
#define PALM_BOX_HEIGHT_INDEX         	3U
#define PALM_BOX_KEYPOINT_OFFSET      	4U

typedef struct {
    const float *scores;
    const float *regressions;
    uint32_t detection_count;
    uint32_t regression_size;
} PalmNetworkOutput_TypeDef;

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

AI_Status_TypeDef PALM_Init(void);
AI_Status_TypeDef PALM_Run(PalmNetworkOutput_TypeDef *output);
AI_Status_TypeDef PALM_Postprocess(const PalmNetworkOutput_TypeDef *network_output, PalmDetection_TypeDef *detection);
AI_Status_TypeDef PALM_CreateLandmarkROI(const PalmDetection_TypeDef *detection, uint32_t frame_width,
										 uint32_t frame_height, HandROI_TypeDef *roi);

void PALM_UpdateDetectionFilter(PalmDetectionFilter_TypeDef *filter, bool detection_valid);
uint8_t *PALM_GetInputBuffer(void);

#endif
