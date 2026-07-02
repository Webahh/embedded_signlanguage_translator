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

#define PALM_INPUT_WIDTH       192U
#define PALM_INPUT_HEIGHT      192U
#define PALM_INPUT_CHANNELS      3U

#define PALM_INPUT_SIZE (PALM_INPUT_WIDTH * PALM_INPUT_HEIGHT * PALM_INPUT_CHANNELS)

#define PALM_DETECTION_COUNT  2016U
#define PALM_REGRESSION_SIZE    18U

#define PALM_SCORE_BUFFER_SIZE (PALM_DETECTION_COUNT * sizeof(float))
#define PALM_REGRESSION_BUFFER_SIZE (PALM_DETECTION_COUNT * PALM_REGRESSION_SIZE * sizeof(float))

typedef struct {
    const float *scores;
    const float *regressions;
    uint32_t detection_count;
    uint32_t regression_size;
} PalmNetworkOutput_TypeDef;

AI_Status_TypeDef PALM_Init(void);

uint8_t *PALM_GetInputBuffer(void);

bool PALM_Run(PalmNetworkOutput_TypeDef *output);

#endif
