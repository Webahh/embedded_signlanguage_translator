/*
 * simple_ai.h
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#ifndef SIMPLE_AI_H
#define SIMPLE_AI_H

#include <stdbool.h>
#include <stdint.h>

#define AI_INPUT_SIZE   88
#define AI_OUTPUT_SIZE  26

#define LANDMARK_INPUT_WIDTH      224U
#define LANDMARK_INPUT_HEIGHT     224U
#define LANDMARK_INPUT_CHANNELS   3U

#define LANDMARK_OUTPUT_COUNT       4U
#define LANDMARK_VECTOR_SIZE       63U

#define PALM_INPUT_WIDTH       	  192U
#define PALM_INPUT_HEIGHT      	  192U
#define PALM_INPUT_CHANNELS      3U

#define PALM_INPUT_ELEMENT_COUNT \
    (PALM_INPUT_WIDTH * \
     PALM_INPUT_HEIGHT * \
     PALM_INPUT_CHANNELS)

#define PALM_INPUT_SIZE \
    PALM_INPUT_ELEMENT_COUNT

#define PALM_DETECTION_COUNT    2016U
#define PALM_REGRESSION_SIZE      18U
#define PALM_REGRESSION_ELEMENT_COUNT \
    (PALM_DETECTION_COUNT * PALM_REGRESSION_SIZE)

#define PALM_SCORE_BUFFER_SIZE \
    (PALM_DETECTION_COUNT * sizeof(float))

#define PALM_REGRESSION_BUFFER_SIZE \
    (PALM_DETECTION_COUNT * \
     PALM_REGRESSION_SIZE * \
     sizeof(float))

typedef struct {
    const float *scores;
    const float *regressions;
    uint32_t detection_count;
    uint32_t regression_size;
} AI_PalmOutput_TypeDef;

typedef struct {
    float scalar_0;
    float vector_0[LANDMARK_VECTOR_SIZE];

    float scalar_1;
    float vector_1[LANDMARK_VECTOR_SIZE];
} AI_LandmarkOutput_TypeDef;

#define LANDMARK_INPUT_SIZE \
    (LANDMARK_INPUT_WIDTH * \
     LANDMARK_INPUT_HEIGHT * \
     LANDMARK_INPUT_CHANNELS)

typedef enum {
    AI_STATUS_OK = 0,
    AI_STATUS_NOT_INITIALIZED,
    AI_STATUS_CACHEAXI_ERROR,
    AI_STATUS_INVALID_INPUT_BUFFER,
    AI_STATUS_INVALID_OUTPUT_BUFFER,
    AI_STATUS_RUNTIME_ERROR
} AI_Status_TypeDef;

typedef struct {
    uint32_t class_index;
    uint8_t score;
    const char *label;
} AI_Result_TypeDef;


AI_Status_TypeDef AI_Init(void);
bool AI_RunLandmark(const uint8_t input[LANDMARK_INPUT_SIZE], AI_LandmarkOutput_TypeDef *output);
bool AI_Run(const uint8_t input[AI_INPUT_SIZE], uint8_t output[AI_OUTPUT_SIZE]);
AI_Result_TypeDef AI_GetResult(const uint8_t output[AI_OUTPUT_SIZE]);
uint32_t AI_GetPrediction(const uint8_t output[AI_OUTPUT_SIZE]);
uint8_t *AI_GetPalmInputBuffer(void);
bool AI_RunPalm(AI_PalmOutput_TypeDef *output);
bool AI_SelfTest(void);

#endif /* SIMPLE_AI_H */
