/*
 * hand_landmark.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include "hand_landmark.h"
#include "ai_runtime_internal.h"

#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"
#include <string.h>

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(hand_landmark_model_v3);

static const LL_Buffer_InfoTypeDef *landmark_input_info;
static const LL_Buffer_InfoTypeDef *landmark_output_info;

static uint8_t *landmark_input_buffer;
static float *landmark_presence_buffer;
static float *landmark_handedness_buffer;
static float *landmark_image_buffer;
static float *landmark_world_buffer;

static bool hand_landmark_initialized = false;
static bool landmark_has_run = false;

AI_Status_TypeDef LANDMARK_Init(void)
{
    LL_ATON_RT_Init_Network(&NN_Instance_hand_landmark_model_v3);

    landmark_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_hand_landmark_model_v3);
    landmark_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_hand_landmark_model_v3);

    landmark_input_buffer = LL_Buffer_addr_start(&landmark_input_info[0]);
    landmark_handedness_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[0]);
    landmark_world_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[1]); //prob. not needed!
    landmark_presence_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[2]);
    landmark_image_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[3]);

    if ((landmark_input_buffer    == NULL) ||
    	(landmark_presence_buffer == NULL) ||
		(landmark_handedness_buffer == NULL) ||
		(landmark_image_buffer == NULL) ||
		(landmark_world_buffer == NULL)){
    	return AI_STATUS_INVALID_BUFFER;
    }

    landmark_has_run = false;
    hand_landmark_initialized = true;

    return AI_STATUS_OK;
}


uint8_t *LANDMARK_GetInputBuffer(void){
    return landmark_input_buffer;
}

AI_Status_TypeDef LANDMARK_Run(LandmarkNetworkOutput_TypeDef *output)
{
    if (!hand_landmark_initialized ||
        (output == NULL) ||
        (landmark_input_buffer == NULL)) {
        return AI_STATUS_RUNTIME_ERROR;
    }

    if (landmark_has_run) {
        LL_ATON_RT_Reset_Network(&NN_Instance_hand_landmark_model_v3);
    }

    // DMA2D wrote the input buffer. Discard stale D-cache so NPU reads fresh data
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)landmark_input_buffer, LANDMARK_INPUT_SIZE);

    if (!AI_RuntimeRunNetwork(&NN_Instance_hand_landmark_model_v3)) {
        return AI_STATUS_RUNTIME_ERROR;
    }

    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)landmark_handedness_buffer, sizeof(float));
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)landmark_presence_buffer, sizeof(float));
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)landmark_image_buffer, LANDMARK_VALUE_COUNT * sizeof(float));
    LL_ATON_Cache_MCU_Invalidate_Range((uintptr_t)landmark_world_buffer, LANDMARK_VALUE_COUNT * sizeof(float));

    output->presence = landmark_presence_buffer[0];
    output->handedness = landmark_handedness_buffer[0];
    memcpy(output->landmarks, landmark_image_buffer, LANDMARK_VALUE_COUNT * sizeof(float));
    memcpy(output->world_landmarks, landmark_world_buffer, LANDMARK_VALUE_COUNT * sizeof(float));

    landmark_has_run = true;
    return AI_STATUS_OK;
}

