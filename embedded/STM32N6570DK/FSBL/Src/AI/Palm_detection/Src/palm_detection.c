/*
 * palm_detection.c
 *
 *  Created on: 29.06.2026
 *      Author: Weber
 */

#include "palm_detection.h"
#include "ai_runtime_internal.h"

#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(palm_detection_model_v3);

static const LL_Buffer_InfoTypeDef *palm_input_info;
static const LL_Buffer_InfoTypeDef *palm_output_info;

static uint8_t *palm_input_buffer;
static float *palm_scores_buffer;
static float *palm_regressions_buffer;

static bool palm_initialized = false;
static bool palm_has_run = false;

AI_Status_TypeDef PALM_Init(void)
{
    LL_ATON_RT_Init_Network(&NN_Instance_palm_detection_model_v3);

    palm_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_palm_detection_model_v3);
    palm_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_palm_detection_model_v3);

    palm_input_buffer = LL_Buffer_addr_start(&palm_input_info[0]);
    palm_scores_buffer = (float *)LL_Buffer_addr_start(&palm_output_info[0]);
    palm_regressions_buffer = (float *)LL_Buffer_addr_start(&palm_output_info[1]);

    if ((palm_input_buffer       == NULL) ||
        (palm_scores_buffer      == NULL) ||
        (palm_regressions_buffer == NULL)) {
        return AI_STATUS_INVALID_BUFFER;
    }

    palm_has_run = false;
    palm_initialized = true;

    return AI_STATUS_OK;
}

uint8_t *PALM_GetInputBuffer(void)
{
    if (!palm_initialized) {
        return NULL;
    }

    return palm_input_buffer;
}

bool PALM_Run(PalmNetworkOutput_TypeDef *output)
{
    if (!palm_initialized ||
        (output == NULL)  ||
        (palm_input_buffer == NULL)) {
        return false;
    }

    if (palm_has_run) {
        LL_ATON_RT_Reset_Network(&NN_Instance_palm_detection_model_v3);
    }

    // DMA2D wrote the input buffer. Discard stale D-cache so NPU reads fresh data
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)palm_input_buffer, PALM_INPUT_SIZE);

    if (!AI_RuntimeRunNetwork(&NN_Instance_palm_detection_model_v3)) {
        return false;
    }

    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)palm_scores_buffer, PALM_SCORE_BUFFER_SIZE);
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)palm_regressions_buffer, PALM_REGRESSION_BUFFER_SIZE);

    output->scores = palm_scores_buffer;
    output->regressions = palm_regressions_buffer;
    output->detection_count = PALM_DETECTION_COUNT;
    output->regression_size = PALM_REGRESSION_SIZE;

    palm_has_run = true;
    return true;
}
