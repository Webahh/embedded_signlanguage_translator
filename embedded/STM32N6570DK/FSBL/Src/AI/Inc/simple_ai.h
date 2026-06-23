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

typedef enum
{
    AI_STATUS_OK = 0,
    AI_STATUS_NOT_INITIALIZED,
    AI_STATUS_CACHEAXI_ERROR,
    AI_STATUS_INVALID_INPUT_BUFFER,
    AI_STATUS_INVALID_OUTPUT_BUFFER,
    AI_STATUS_RUNTIME_ERROR
} AI_Status_TypeDef;

AI_Status_TypeDef AI_Init(void);

uint8_t *AI_GetInputBuffer(void);
uint8_t *AI_GetOutputBuffer(void);

uint32_t AI_GetInputSize(void);
uint32_t AI_GetOutputSize(void);

#endif /* SIMPLE_AI_H */
