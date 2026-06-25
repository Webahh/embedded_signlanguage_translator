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

typedef enum{
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
bool AI_Run(const uint8_t input[AI_INPUT_SIZE], uint8_t output[AI_OUTPUT_SIZE]);
AI_Result_TypeDef AI_GetResult(const uint8_t output[AI_OUTPUT_SIZE]);

uint8_t *AI_GetInputBuffer(void);
uint8_t *AI_GetOutputBuffer(void);

uint32_t AI_GetInputSize(void);
uint32_t AI_GetOutputSize(void);

#endif /* SIMPLE_AI_H */
