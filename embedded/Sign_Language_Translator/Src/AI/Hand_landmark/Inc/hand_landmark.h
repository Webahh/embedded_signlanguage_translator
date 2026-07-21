/*
 * hand_landmark.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */


#ifndef HAND_LANDMARK_H
#define HAND_LANDMARK_H

#include <stdbool.h>
#include <stdint.h>

#include "simple_ai.h"
#include "palm_detection.h"

#define LANDMARK_INPUT_WIDTH      224U
#define LANDMARK_INPUT_HEIGHT     224U
#define LANDMARK_INPUT_CHANNELS   3U
#define LANDMARK_INPUT_SIZE 	  (LANDMARK_INPUT_WIDTH * LANDMARK_INPUT_HEIGHT * LANDMARK_INPUT_CHANNELS)

#define LANDMARK_POINT_COUNT      21U
#define LANDMARK_VALUE_COUNT      63U

typedef struct {
    float presence;
    float handedness;
    float landmarks[LANDMARK_VALUE_COUNT];
    float world_landmarks[LANDMARK_VALUE_COUNT];
} LandmarkNetworkOutput_TypeDef;

typedef struct {
    float x;
    float y;
    float z;
} LandmarkPoint_TypeDef;

AI_Status_TypeDef LANDMARK_Init(void);
AI_Status_TypeDef LANDMARK_Run(LandmarkNetworkOutput_TypeDef *output);
AI_Status_TypeDef LANDMARK_MapToFrame(const LandmarkNetworkOutput_TypeDef *output, const HandROI_TypeDef *roi,
                                      uint32_t frame_width, uint32_t frame_height, LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT]);

AI_Status_TypeDef LANDMARK_PreprocessROI(const uint8_t *source, uint32_t source_width, uint32_t source_height,
										 uint32_t source_stride_bytes, const HandROI_TypeDef *roi, uint8_t *destination);

AI_Status_TypeDef LANDMARK_UpdateROIFromNetworkOutput(const LandmarkNetworkOutput_TypeDef *output, const HandROI_TypeDef *current_roi,
													  HandROI_TypeDef *next_roi);

uint8_t *LANDMARK_GetInputBuffer(void);

#endif /* HAND_LANDMARK_H */
