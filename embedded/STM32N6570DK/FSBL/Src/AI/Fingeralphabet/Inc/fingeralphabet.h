/*
 * fingeralphabet.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#ifndef FINGERALPHABET_H
#define FINGERALPHABET_H

#include <stdbool.h>
#include <stdint.h>

#include "simple_ai.h"
#include "ai_runtime_internal.h"

#define FINGERALPHABET_INPUT_SIZE  88U
#define FINGERALPHABET_OUTPUT_SIZE 26U

typedef struct {
    uint32_t class_index;
    uint8_t score;
    const char *label;
} FingeralphabetResult_TypeDef;

AI_Status_TypeDef FINGERALPHABET_Init(void);
bool FINGERALPHABET_Run(const uint8_t input[FINGERALPHABET_INPUT_SIZE], uint8_t output[FINGERALPHABET_OUTPUT_SIZE]);
bool FINGERALPHABET_Start(const uint8_t input[FINGERALPHABET_INPUT_SIZE]);
AI_RunStepStatus_TypeDef FINGERALPHABET_RunStep(uint8_t output[FINGERALPHABET_OUTPUT_SIZE]);
FingeralphabetResult_TypeDef FINGERALPHABET_GetResult(const uint8_t output[FINGERALPHABET_OUTPUT_SIZE]);

#endif /* FINGERALPHABET_H */
