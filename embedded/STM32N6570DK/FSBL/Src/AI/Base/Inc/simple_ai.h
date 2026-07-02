/*
 * simple_ai.h
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#ifndef SIMPLE_AI_H
#define SIMPLE_AI_H

#include <stdbool.h>

typedef enum {
    AI_STATUS_OK = 0,
    AI_STATUS_NOT_INITIALIZED,
    AI_STATUS_CACHEAXI_ERROR,
    AI_STATUS_INVALID_BUFFER,
    AI_STATUS_RUNTIME_ERROR
} AI_Status_TypeDef;

AI_Status_TypeDef AI_Init(void);
bool AI_IsInitialized(void);

#endif
