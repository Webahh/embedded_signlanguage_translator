/*
 * simple_ai.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#include <string.h>

#include "simple_ai.h"
#include "simple_rcc.h"

#include "stm32n6xx_hal.h"
#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"
#include "ll_aton_osal.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(fingeralphabet_model_v2);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(hand_landmark_model_v2);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(palm_detection_model_v2);

static CACHEAXI_HandleTypeDef ai_cacheaxi_handle;
static bool ai_initialized = false;
static bool ai_fingeralphabet_has_run = false;
static bool ai_landmark_has_run = false;
static bool ai_palm_has_run = false;


static const LL_Buffer_InfoTypeDef *fingeralphabet_input_info  = NULL;
static const LL_Buffer_InfoTypeDef *fingeralphabet_output_info = NULL;
static uint8_t *ai_input_buffer  = NULL;
static uint8_t *ai_output_buffer = NULL;


static const LL_Buffer_InfoTypeDef *landmark_input_info  = NULL;
static const LL_Buffer_InfoTypeDef *landmark_output_info = NULL;
static uint8_t *landmark_input_buffer  = NULL;
static float *landmark_output_0_buffer = NULL;
static float *landmark_output_1_buffer = NULL;
static float *landmark_output_2_buffer = NULL;
static float *landmark_output_3_buffer = NULL;


static const LL_Buffer_InfoTypeDef *palm_input_info  = NULL;
static const LL_Buffer_InfoTypeDef *palm_output_info = NULL;
static float *palm_input_buffer    = NULL;
static float *palm_output_0_buffer = NULL;
static float *palm_output_1_buffer = NULL;

/* -------------------------------------------------------------------------- */
/* Fingeralphabet labels                                                      */
/* -------------------------------------------------------------------------- */

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


static void AI_EnableNpuRam(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_RAMCFGEN;
    (void)RCC->AHB2ENR;

    RAMCFG_SRAM3_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM3_AXI->CR;

    RAMCFG_SRAM4_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM4_AXI->CR;

    RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM5_AXI->CR;

    RAMCFG_SRAM6_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM6_AXI->CR;

    RCC->MEMENR |= 0x17FFU;
    (void)RCC->MEMENR;

    RCC->AHB5ENR |= 0xC0183022U;
    (void)RCC->AHB5ENR;
}


static AI_Status_TypeDef AI_InitFingeralphabetBuffers(void)
{
    fingeralphabet_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model_v2);
    fingeralphabet_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model_v2);

    if ((fingeralphabet_input_info == NULL) ||
        (fingeralphabet_input_info[0].name == NULL)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((fingeralphabet_output_info == NULL) ||
        (fingeralphabet_output_info[0].name == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    /*
     * Exactly one input and one output are expected.
     */
    if (fingeralphabet_input_info[1].name != NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if (fingeralphabet_output_info[1].name != NULL) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&fingeralphabet_input_info[0]) != AI_INPUT_SIZE) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if (LL_Buffer_len(&fingeralphabet_output_info[0]) != AI_OUTPUT_SIZE) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if ((fingeralphabet_input_info[0].type != DataType_UINT8) ||
        (fingeralphabet_input_info[0].nbits != 8U)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((fingeralphabet_output_info[0].type != DataType_UINT8) ||
        (fingeralphabet_output_info[0].nbits != 8U)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    ai_input_buffer =
        LL_Buffer_addr_start(&fingeralphabet_input_info[0]);

    ai_output_buffer =
        LL_Buffer_addr_start(&fingeralphabet_output_info[0]);

    if (ai_input_buffer == NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if (ai_output_buffer == NULL) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    return AI_STATUS_OK;
}


static AI_Status_TypeDef AI_InitLandmarkBuffers(void)
{
    landmark_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_hand_landmark_model_v2);
    landmark_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_hand_landmark_model_v2);

    if ((landmark_input_info == NULL) ||
        (landmark_input_info[0].name == NULL)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((landmark_output_info == NULL) ||
        (landmark_output_info[0].name == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    /*
     * Exactly one landmark input is expected.
     */
    if (landmark_input_info[1].name != NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if (LL_Buffer_len(&landmark_input_info[0]) !=
        LANDMARK_INPUT_SIZE) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((landmark_input_info[0].type != DataType_UINT8) ||
        (landmark_input_info[0].nbits != 8U)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    /*
     * Exactly four outputs are expected.
     */
    for (uint32_t i = 0U;
         i < LANDMARK_OUTPUT_COUNT;
         i++) {

        if (landmark_output_info[i].name == NULL) {
            return AI_STATUS_INVALID_OUTPUT_BUFFER;
        }

        if ((landmark_output_info[i].type != DataType_FLOAT) ||
            (landmark_output_info[i].nbits != 32U)) {
            return AI_STATUS_INVALID_OUTPUT_BUFFER;
        }
    }

    if (landmark_output_info[LANDMARK_OUTPUT_COUNT].name != NULL) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&landmark_output_info[0]) !=
        sizeof(float)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&landmark_output_info[1]) !=
        (LANDMARK_VECTOR_SIZE * sizeof(float))) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&landmark_output_info[2]) !=
        sizeof(float)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&landmark_output_info[3]) !=
        (LANDMARK_VECTOR_SIZE * sizeof(float))) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    landmark_input_buffer =
        LL_Buffer_addr_start(&landmark_input_info[0]);

    landmark_output_0_buffer =
        (float *)LL_Buffer_addr_start(
            &landmark_output_info[0]
        );

    landmark_output_1_buffer =
        (float *)LL_Buffer_addr_start(
            &landmark_output_info[1]
        );

    landmark_output_2_buffer =
        (float *)LL_Buffer_addr_start(
            &landmark_output_info[2]
        );

    landmark_output_3_buffer =
        (float *)LL_Buffer_addr_start(
            &landmark_output_info[3]
        );

    if (landmark_input_buffer == NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((landmark_output_0_buffer == NULL) ||
        (landmark_output_1_buffer == NULL) ||
        (landmark_output_2_buffer == NULL) ||
        (landmark_output_3_buffer == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    return AI_STATUS_OK;
}


static bool AI_RunNetwork(NN_Instance_TypeDef *network)
{
    LL_ATON_RT_RetValues_t runtime_status;

    if (network == NULL) {
        return false;
    }

    do {
        runtime_status =
            LL_ATON_RT_RunEpochBlock(network);

        if (runtime_status == LL_ATON_RT_WFE) {
            LL_ATON_OSAL_WFE();
        }

    } while (runtime_status != LL_ATON_RT_DONE);

    return true;
}


/* -------------------------------------------------------------------------- */
/* Public initialization                                                      */
/* -------------------------------------------------------------------------- */

AI_Status_TypeDef AI_Init(void)
{
    AI_Status_TypeDef status;

    AI_EnableNpuRam();

    /*
     * Initialize CACHEAXI.
     */
    ai_cacheaxi_handle.Instance = CACHEAXI;

    if (HAL_CACHEAXI_Init(&ai_cacheaxi_handle) != HAL_OK) {
        return AI_STATUS_CACHEAXI_ERROR;
    }

    /*
     * Initialize the Neural-ART runtime once.
     */
    LL_ATON_RT_RuntimeInit();
    LL_ATON_RT_Init_Network(&NN_Instance_fingeralphabet_model_v2);
    LL_ATON_RT_Init_Network(&NN_Instance_hand_landmark_model_v2);

    status = AI_InitFingeralphabetBuffers();

    if (status != AI_STATUS_OK) {
        return status;
    }

    status = AI_InitLandmarkBuffers();

    if (status != AI_STATUS_OK) {
        return status;
    }

    ai_fingeralphabet_has_run = false;
    ai_landmark_has_run = false;
    ai_initialized = true;

    return AI_STATUS_OK;
}


/* -------------------------------------------------------------------------- */
/* Landmark inference                                                         */
/* -------------------------------------------------------------------------- */

bool AI_RunLandmark(const uint8_t input[LANDMARK_INPUT_SIZE], AI_LandmarkOutput_TypeDef *output)
{
    if (!ai_initialized ||
        (input == NULL) ||
        (output == NULL) ||
        (landmark_input_buffer == NULL)) {
        return false;
    }

    /*
     * Nicht vor dem ersten Lauf zurücksetzen.
     */
    if (ai_landmark_has_run) {
        LL_ATON_RT_Reset_Network(&NN_Instance_hand_landmark_model_v2);
    }

    memcpy(landmark_input_buffer, input, LANDMARK_INPUT_SIZE);

    if (!AI_RunNetwork(&NN_Instance_hand_landmark_model_v2)) {
        return false;
    }

    output->scalar_0 = landmark_output_0_buffer[0];

    memcpy(output->vector_0, landmark_output_1_buffer, sizeof(output->vector_0));

    output->scalar_1 = landmark_output_2_buffer[0];

    memcpy(output->vector_1, landmark_output_3_buffer, sizeof(output->vector_1));

    ai_landmark_has_run = true;

    return true;
}


/* -------------------------------------------------------------------------- */
/* Fingeralphabet inference                                                   */
/* -------------------------------------------------------------------------- */

bool AI_Run(const uint8_t input[AI_INPUT_SIZE], uint8_t output[AI_OUTPUT_SIZE])
{
    if (!ai_initialized ||
        (input == NULL) ||
        (output == NULL) ||
        (ai_input_buffer == NULL) ||
        (ai_output_buffer == NULL)) {
        return false;
    }

    if (ai_fingeralphabet_has_run) {
        LL_ATON_RT_Reset_Network(&NN_Instance_fingeralphabet_model_v2);
    }

    memcpy(ai_input_buffer, input, AI_INPUT_SIZE);

    if (!AI_RunNetwork(&NN_Instance_fingeralphabet_model_v2)) {
        return false;
    }

    memcpy(output, ai_output_buffer, AI_OUTPUT_SIZE);

    ai_fingeralphabet_has_run = true;

    return true;
}


/* -------------------------------------------------------------------------- */
/* Fingeralphabet result evaluation                                           */
/* -------------------------------------------------------------------------- */

uint32_t AI_GetPrediction(const uint8_t output[AI_OUTPUT_SIZE])
{
    uint32_t best_index = 0U;
    uint8_t best_value;

    if (output == NULL) {
        return 0U;
    }

    best_value = output[0];

    for (uint32_t i = 1U;
         i < AI_OUTPUT_SIZE;
         i++) {

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
        .score = 0U,
        .label = ai_labels[0]
    };

    if (output == NULL) {
        return result;
    }

    result.score = output[0];

    for (uint32_t i = 1U;
         i < AI_OUTPUT_SIZE;
         i++) {

        if (output[i] > result.score) {
            result.class_index = i;
            result.score = output[i];
            result.label = ai_labels[i];
        }
    }

    return result;
}
