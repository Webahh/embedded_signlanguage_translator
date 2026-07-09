/*
 * fingeralphabet.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include "fingeralphabet.h"
#include "ai_runtime_internal.h"

#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(fingeralphabet_model_v3);

static const LL_Buffer_InfoTypeDef *fingeralphabet_input_info;
static const LL_Buffer_InfoTypeDef *fingeralphabet_output_info;

static uint8_t *fingeralphabet_input_buffer;
static uint8_t *fingeralphabet_output_buffer;

static bool fingeralphabet_initialized = false;
static bool fingeralphabet_has_run = false;
static bool fingeralphabet_running = false;

static const char *const ai_labels[FINGERALPHABET_OUTPUT_SIZE] = {
    "NONE",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "SCH"
};

AI_Status_TypeDef FINGERALPHABET_Init(void)
{
	LL_ATON_RT_Init_Network(&NN_Instance_fingeralphabet_model_v3);

    fingeralphabet_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model_v3);
    fingeralphabet_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model_v3);

    fingeralphabet_input_buffer = LL_Buffer_addr_start(&fingeralphabet_input_info[0]);
    fingeralphabet_output_buffer = LL_Buffer_addr_start(&fingeralphabet_output_info[0]);

    if((fingeralphabet_input_buffer  == NULL) ||
       (fingeralphabet_output_buffer == NULL)) {
    	return AI_STATUS_INVALID_BUFFER;
    }

    fingeralphabet_has_run = false;
	fingeralphabet_initialized = true;

	return AI_STATUS_OK;
}

AI_Status_TypeDef FINGERALPHABET_Start(const uint8_t input[FINGERALPHABET_INPUT_SIZE])
{
    if (!fingeralphabet_initialized 	      ||
        (input == NULL) 					  ||
        (fingeralphabet_input_buffer == NULL) ||
        fingeralphabet_running) {
        return AI_STATUS_RUNTIME_ERROR;
    }

    if (fingeralphabet_has_run) {
        LL_ATON_RT_Reset_Network(&NN_Instance_fingeralphabet_model_v3);
    }

    memcpy(fingeralphabet_input_buffer, input, FINGERALPHABET_INPUT_SIZE);
    SCB_CleanDCache_by_Addr((uint32_t *)fingeralphabet_input_buffer, FINGERALPHABET_INPUT_SIZE);

    fingeralphabet_running = true;
    return AI_STATUS_OK;
}

AI_RunStepStatus_TypeDef FINGERALPHABET_RunStep(uint8_t output[FINGERALPHABET_OUTPUT_SIZE])
{
    if (!fingeralphabet_initialized ||
        !fingeralphabet_running 	||
        (output == NULL)) {
        return AI_RUN_ERROR;
    }

    const AI_RunStepStatus_TypeDef status = AI_RuntimeRunNetworkStep(&NN_Instance_fingeralphabet_model_v3);

    if (status == AI_RUN_BUSY) {
        return AI_RUN_BUSY;
    }

    if (status == AI_RUN_ERROR) {
        fingeralphabet_running = false;
        return AI_RUN_ERROR;
    }

    SCB_InvalidateDCache_by_Addr((uint32_t *)fingeralphabet_output_buffer, FINGERALPHABET_OUTPUT_SIZE);
    memcpy(output, fingeralphabet_output_buffer, FINGERALPHABET_OUTPUT_SIZE);



    fingeralphabet_running = false;
    fingeralphabet_has_run = true;
    return AI_RUN_DONE;
}

FingeralphabetResult_TypeDef FINGERALPHABET_GetResult(const uint8_t output[FINGERALPHABET_OUTPUT_SIZE])
{
    FingeralphabetResult_TypeDef result = {
        .class_index = 0U,
        .score = 0U,
        .label = ai_labels[0]
    };

    if (output == NULL) {
        return result;
    }

    result.score = output[0];

    for (uint32_t i = 1U; i < FINGERALPHABET_OUTPUT_SIZE; i++) {
        if (output[i] > result.score) {
            result.class_index = i;
            result.score = output[i];
            result.label = ai_labels[i];
        }
    }

    return result;
}
