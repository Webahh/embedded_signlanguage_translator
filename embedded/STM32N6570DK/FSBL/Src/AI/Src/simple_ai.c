/*
 * simple_ai.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#include <string.h>
#include "simple_ai.h"

#include "stm32n6xx_hal.h"
#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"
#include "ll_aton_osal.h"
#include "simple_rcc.h"

extern void NPU0_IRQHandler(void);

#define AI_INPUT_ADDRESS   0x342E0000UL
#define AI_OUTPUT_ADDRESS  0x342E0020UL

static uint8_t *const ai_input = (uint8_t *)AI_INPUT_ADDRESS;
static uint8_t *const ai_output = (uint8_t *)AI_OUTPUT_ADDRESS;

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(fingeralphabet_model);
static CACHEAXI_HandleTypeDef ai_cacheaxi_handle;
static bool ai_initialized = false;
static bool ai_has_run = false;

static const char *const ai_labels[AI_OUTPUT_SIZE] = {
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

AI_Status_TypeDef AI_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_RAMCFGEN;
	(void)RCC->AHB2ENR;

	RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
	(void)RAMCFG_SRAM5_AXI->CR;

	RCC->MEMENR |= 0x17ff;
	(void)RCC->MEMENR;
	RCC->AHB5ENR |= 0xc0183022;		//CAPSULATE SOON IN RCC!!!!
	(void)RCC->AHB5ENR;

    ai_cacheaxi_handle.Instance = CACHEAXI;

    if (HAL_CACHEAXI_Init(&ai_cacheaxi_handle) != HAL_OK) {
        return AI_STATUS_CACHEAXI_ERROR;
    }

    LL_ATON_RT_RuntimeInit();
    LL_ATON_RT_Init_Network(&NN_Instance_fingeralphabet_model);

    ai_initialized = true;

    return AI_STATUS_OK;
}

bool AI_Run(const uint8_t input[AI_INPUT_SIZE], uint8_t output[AI_OUTPUT_SIZE])
{
    LL_ATON_RT_RetValues_t runtime_status;

    if ((input == NULL) || (output == NULL)) {
        return false;
    }

    if (ai_has_run) {
        LL_ATON_RT_Reset_Network(
            &NN_Instance_fingeralphabet_model
        );
    }

    memcpy(ai_input, input, AI_INPUT_SIZE);

    do {
        runtime_status =
            LL_ATON_RT_RunEpochBlock(
                &NN_Instance_fingeralphabet_model
            );

        if (runtime_status == LL_ATON_RT_WFE) {
            LL_ATON_OSAL_WFE();
        }

    } while (runtime_status != LL_ATON_RT_DONE);

    memcpy(output, ai_output, AI_OUTPUT_SIZE);

    ai_has_run = true;

    return true;
}

uint32_t AI_GetPrediction(const uint8_t output[AI_OUTPUT_SIZE])
{
    uint32_t best_index = 0U;
    uint8_t best_value = output[0];

    for (uint32_t i = 1U; i < AI_OUTPUT_SIZE; i++) {
        if (output[i] > best_value) {
            best_value = output[i];
            best_index = i;
        }
    }

    return best_index;
}

AI_Result_TypeDef AI_GetResult(const uint8_t output[AI_OUTPUT_SIZE])
{
    AI_Result_TypeDef result = {
        .class_index = 0U,
        .score = output[0],
        .label = ai_labels[0]
    };

    for (uint32_t i = 1U; i < AI_OUTPUT_SIZE; i++) {
        if (output[i] > result.score) {
            result.class_index = i;
            result.score = output[i];
            result.label = ai_labels[i];
        }
    }

    return result;
}

uint8_t *AI_GetInputBuffer(void)
{
    if (!ai_initialized) {
        return NULL;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return NULL;
    }

    return LL_Buffer_addr_start(info);
}

uint8_t *AI_GetOutputBuffer(void)
{
    if (!ai_initialized) {
        return NULL;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return NULL;
    }

    return LL_Buffer_addr_start(info);
}

uint32_t AI_GetInputSize(void)
{
    if (!ai_initialized) {
        return 0U;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return 0U;
    }

    return LL_Buffer_len(info);
}

uint32_t AI_GetOutputSize(void)
{
    if (!ai_initialized) {
        return 0U;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return 0U;
    }

    return LL_Buffer_len(info);
}
