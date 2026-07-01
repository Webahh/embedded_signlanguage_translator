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

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(hand_landmark_model_v3);

static const LL_Buffer_InfoTypeDef *landmark_input_info;
static const LL_Buffer_InfoTypeDef *landmark_output_info;

static uint8_t *landmark_input_buffer;
static float *landmark_output_0_buffer;
static float *landmark_output_1_buffer;
static float *landmark_output_2_buffer;
static float *landmark_output_3_buffer;

static bool hand_landmark_initialized = false;
static bool landmark_has_run = false;


AI_Status_TypeDef LANDMARK_Init(void)
{
    LL_ATON_RT_Init_Network(&NN_Instance_hand_landmark_model_v3);

    landmark_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_hand_landmark_model_v3);
    landmark_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_hand_landmark_model_v3);

    landmark_input_buffer = LL_Buffer_addr_start(&landmark_input_info[0]);
    landmark_output_0_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[0]);
    landmark_output_1_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[1]);
    landmark_output_2_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[2]);
    landmark_output_3_buffer = (float *)LL_Buffer_addr_start(&landmark_output_info[3]);

    if ((landmark_input_buffer    == NULL) ||
    	(landmark_output_0_buffer == NULL) ||
		(landmark_output_1_buffer == NULL) ||
		(landmark_output_2_buffer == NULL) ||
		(landmark_output_3_buffer == NULL)){
    	return AI_STATUS_INVALID_BUFFER;
    }

    landmark_has_run = false;
    hand_landmark_initialized = true;

    return AI_STATUS_OK;
}


bool LANDMARK_Run(const uint8_t input[LANDMARK_INPUT_SIZE], LandmarkNetworkOutput_TypeDef *output)
{
	if(!hand_landmark_initialized ||
	  (output == NULL)		      ||
	  (input == NULL)             ||
	  (landmark_input_buffer == NULL)){
		return false;
	}

	if(landmark_has_run){
        LL_ATON_RT_Reset_Network(&NN_Instance_hand_landmark_model_v3);
	}

	memcpy(landmark_input_buffer, input, LANDMARK_INPUT_SIZE);

    if (!AI_RuntimeRunNetwork(&NN_Instance_hand_landmark_model_v3)) {
        return false;
    }

    output->scalar_0 = landmark_output_0_buffer[0];
    memcpy(output->vector_0, landmark_output_1_buffer, sizeof(output->vector_0));
    output->scalar_1 = landmark_output_2_buffer[0];
    memcpy(output->vector_1, landmark_output_3_buffer, sizeof(output->vector_1));

	landmark_has_run = true;
	return true;
}
