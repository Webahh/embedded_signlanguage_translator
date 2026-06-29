/*
 * simple_ai.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#include <string.h>

#include "simple_ai.h"
#include "simple_rcc.h"
#include "npu_cache.h"

#include "stm32n6xx_hal.h"
#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"
#include "ll_aton_osal.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(fingeralphabet_model_v3);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(hand_landmark_model_v3);
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(palm_detection_model_v3);

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

volatile uint32_t palm_debug_output_len_0 = 0U;
volatile uint32_t palm_debug_output_len_1 = 0U;

volatile uint32_t palm_debug_output_type_0 = 0U;
volatile uint32_t palm_debug_output_type_1 = 0U;

volatile uint32_t palm_debug_output_nbits_0 = 0U;
volatile uint32_t palm_debug_output_nbits_1 = 0U;

volatile uintptr_t palm_debug_output_addr_0 = 0U;
volatile uintptr_t palm_debug_output_addr_1 = 0U;

static const LL_Buffer_InfoTypeDef *palm_input_info  = NULL;
static const LL_Buffer_InfoTypeDef *palm_output_info = NULL;
static uint8_t *palm_input_buffer = NULL;
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
	npu_cache_enable();

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
    fingeralphabet_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model_v3);
    fingeralphabet_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model_v3);

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
    landmark_input_info = LL_ATON_Input_Buffers_Info(&NN_Instance_hand_landmark_model_v3);
    landmark_output_info = LL_ATON_Output_Buffers_Info(&NN_Instance_hand_landmark_model_v3);


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


static AI_Status_TypeDef AI_InitPalmBuffers(void)
{
    palm_input_info =
        LL_ATON_Input_Buffers_Info(
            &NN_Instance_palm_detection_model_v3
        );

    palm_output_info =
        LL_ATON_Output_Buffers_Info(
            &NN_Instance_palm_detection_model_v3
        );

    if ((palm_input_info == NULL) ||
        (palm_input_info[0].name == NULL)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((palm_output_info == NULL) ||
        (palm_output_info[0].name == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    /*
     * Exactly one input is expected.
     */
    if (palm_input_info[1].name != NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if (LL_Buffer_len(&palm_input_info[0]) !=
        PALM_INPUT_SIZE) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((palm_input_info[0].type != DataType_UINT8) ||
        (palm_input_info[0].nbits != 8U)) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    /*
     * Exactly two outputs are expected.
     */
    if ((palm_output_info[0].name == NULL) ||
        (palm_output_info[1].name == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (palm_output_info[2].name != NULL) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    for (uint32_t i = 0U; i < 2U; i++) {
        if ((palm_output_info[i].type != DataType_FLOAT) ||
            (palm_output_info[i].nbits != 32U)) {
            return AI_STATUS_INVALID_OUTPUT_BUFFER;
        }
    }

    if (LL_Buffer_len(&palm_output_info[0]) !=
        PALM_SCORE_BUFFER_SIZE) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    if (LL_Buffer_len(&palm_output_info[1]) !=
        PALM_REGRESSION_BUFFER_SIZE) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    palm_input_buffer =
        LL_Buffer_addr_start(
            &palm_input_info[0]
        );

    palm_output_0_buffer =
        (float *)LL_Buffer_addr_start(
            &palm_output_info[0]
        );

    palm_output_1_buffer =
        (float *)LL_Buffer_addr_start(
            &palm_output_info[1]
        );

    if (palm_input_buffer == NULL) {
        return AI_STATUS_INVALID_INPUT_BUFFER;
    }

    if ((palm_output_0_buffer == NULL) ||
        (palm_output_1_buffer == NULL)) {
        return AI_STATUS_INVALID_OUTPUT_BUFFER;
    }

    palm_debug_output_len_0 =
        LL_Buffer_len(&palm_output_info[0]);

    palm_debug_output_len_1 =
        LL_Buffer_len(&palm_output_info[1]);

    palm_debug_output_type_0 =
        palm_output_info[0].type;

    palm_debug_output_type_1 =
        palm_output_info[1].type;

    palm_debug_output_nbits_0 =
        palm_output_info[0].nbits;

    palm_debug_output_nbits_1 =
        palm_output_info[1].nbits;

    palm_debug_output_addr_0 =
        (uintptr_t)LL_Buffer_addr_start(
            &palm_output_info[0]
        );

    palm_debug_output_addr_1 =
        (uintptr_t)LL_Buffer_addr_start(
            &palm_output_info[1]
        );

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
    LL_ATON_RT_Init_Network(&NN_Instance_fingeralphabet_model_v3);
    LL_ATON_RT_Init_Network(&NN_Instance_hand_landmark_model_v3);
    LL_ATON_RT_Init_Network(&NN_Instance_palm_detection_model_v3);

    status = AI_InitFingeralphabetBuffers();

    if (status != AI_STATUS_OK) {
        return status;
    }

    status = AI_InitLandmarkBuffers();

    if (status != AI_STATUS_OK) {
        return status;
    }

    status = AI_InitPalmBuffers();

    if (status != AI_STATUS_OK) {
        return status;
    }

    ai_fingeralphabet_has_run = false;
    ai_landmark_has_run = false;
    ai_palm_has_run = false;

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
        LL_ATON_RT_Reset_Network(&NN_Instance_hand_landmark_model_v3);
    }

    memcpy(landmark_input_buffer, input, LANDMARK_INPUT_SIZE);

    if (!AI_RunNetwork(&NN_Instance_hand_landmark_model_v3)) {
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
        LL_ATON_RT_Reset_Network(&NN_Instance_fingeralphabet_model_v3);
    }

    memcpy(ai_input_buffer, input, AI_INPUT_SIZE);

    if (!AI_RunNetwork(&NN_Instance_fingeralphabet_model_v3)) {
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

uint8_t *AI_GetPalmInputBuffer(void)
{
    if (!ai_initialized) {
        return NULL;
    }

    return palm_input_buffer;
}


bool AI_RunPalm(AI_PalmOutput_TypeDef *output)
{
    if (!ai_initialized ||
        (output == NULL) ||
        (palm_input_buffer == NULL) ||
        (palm_output_0_buffer == NULL) ||
        (palm_output_1_buffer == NULL)) {
        return false;
    }

    if (ai_palm_has_run) {
        LL_ATON_RT_Reset_Network(
            &NN_Instance_palm_detection_model_v3
        );
    }

    LL_ATON_Cache_MCU_Clean_Range(
        (uintptr_t)palm_input_buffer,
        PALM_INPUT_SIZE
    );

    if (!AI_RunNetwork(
            &NN_Instance_palm_detection_model_v3)) {
        return false;
    }

    /*
     * Die Outputs wurden von der NPU beschrieben.
     * Vor dem Lesen durch die CPU Cachezeilen verwerfen.
     */
    LL_ATON_Cache_MCU_Invalidate_Range(
        (uintptr_t)palm_output_0_buffer,
        PALM_SCORE_BUFFER_SIZE
    );

    LL_ATON_Cache_MCU_Invalidate_Range(
        (uintptr_t)palm_output_1_buffer,
        PALM_REGRESSION_BUFFER_SIZE
    );

    output->scores = palm_output_0_buffer;
    output->regressions = palm_output_1_buffer;
    output->detection_count = PALM_DETECTION_COUNT;
    output->regression_size = PALM_REGRESSION_SIZE;

    ai_palm_has_run = true;

    return true;
}


/* -------------------------------------------------------------------------- */
/* DEBUG TEST                                       */
/* -------------------------------------------------------------------------- */

typedef enum {
    AI_TEST_STAGE_NONE = 0,
    AI_TEST_STAGE_INIT,
    AI_TEST_STAGE_PALM,
    AI_TEST_STAGE_FINGERALPHABET,
    AI_TEST_STAGE_LANDMARK,
    AI_TEST_STAGE_DONE,
    AI_TEST_STAGE_ERROR
} AI_TestStage_TypeDef;

static volatile AI_TestStage_TypeDef ai_test_stage = AI_TEST_STAGE_NONE;

/* Palm debug values */
static volatile bool palm_test_ok = false;
static volatile float palm_score_0 = 0.0f;
static volatile float palm_score_min = 0.0f;
static volatile float palm_score_max = 0.0f;
static volatile uint32_t palm_score_max_index = 0U;
static volatile uint32_t palm_finite_score_count = 0U;
static volatile uint32_t palm_nan_score_count = 0U;

static volatile float palm_regression_0 = 0.0f;
static volatile float palm_regression_1 = 0.0f;
static volatile float palm_regression_min = 0.0f;
static volatile float palm_regression_max = 0.0f;
static volatile uint32_t palm_finite_regression_count = 0U;

/* Landmark debug values */
static uint8_t landmark_test_input[LANDMARK_INPUT_SIZE];
static AI_LandmarkOutput_TypeDef landmark_test_output;

static volatile bool landmark_test_ok = false;
static volatile uint32_t landmark_finite_count = 0U;
static volatile uint32_t landmark_nan_count = 0U;
static volatile float landmark_min = 0.0f;
static volatile float landmark_max = 0.0f;

static volatile uint32_t predicted_index = 0U;
static volatile uint8_t predicted_score = 0U;
static volatile const char *predicted_label = NULL;

static volatile float landmark_scalar_0 = 0.0f;
static volatile float landmark_scalar_1 = 0.0f;

static volatile float landmark_vector_0_first = 0.0f;
static volatile float landmark_vector_1_first = 0.0f;

static AI_PalmOutput_TypeDef palm_test_output;
static volatile AI_Status_TypeDef ai_status_debug =
    AI_STATUS_NOT_INITIALIZED;

bool AI_SelfTest(void)
{
    /* ---------------------------------------------------------
     * 1. AI Runtime und Netzwerke initialisieren
     * --------------------------------------------------------- */
    ai_test_stage = AI_TEST_STAGE_INIT;

    ai_status_debug = AI_Init();

    if (ai_status_debug != AI_STATUS_OK) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    /* ---------------------------------------------------------
     * 2. Palm-Detection-Modell testen
     * --------------------------------------------------------- */
    ai_test_stage = AI_TEST_STAGE_PALM;

    uint8_t *palm_input = AI_GetPalmInputBuffer();

    if (palm_input == NULL) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    memset(
        palm_input,
        128,
        PALM_INPUT_ELEMENT_COUNT
    );

    palm_test_ok = AI_RunPalm(&palm_test_output);

    if (!palm_test_ok) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    palm_score_0 = palm_test_output.scores[0];
    palm_score_min = palm_test_output.scores[0];
    palm_score_max = palm_test_output.scores[0];
    palm_score_max_index = 0U;
    palm_finite_score_count = 0U;
    palm_nan_score_count = 0U;

    for (uint32_t i = 0U; i < PALM_DETECTION_COUNT; i++) {
        float value = palm_test_output.scores[i];

        if (isfinite(value)) {
            palm_finite_score_count++;

            if (value < palm_score_min) {
                palm_score_min = value;
            }

            if (value > palm_score_max) {
                palm_score_max = value;
                palm_score_max_index = i;
            }
        } else {
            palm_nan_score_count++;
        }
    }

    palm_regression_0 = palm_test_output.regressions[0];
    palm_regression_1 = palm_test_output.regressions[1];
    palm_regression_min = palm_test_output.regressions[0];
    palm_regression_max = palm_test_output.regressions[0];
    palm_finite_regression_count = 0U;

    for (uint32_t i = 0U;
         i < PALM_REGRESSION_ELEMENT_COUNT;
         i++) {

        float value = palm_test_output.regressions[i];

        if (!isfinite(value)) {
            continue;
        }

        palm_finite_regression_count++;

        if (value < palm_regression_min) {
            palm_regression_min = value;
        }

        if (value > palm_regression_max) {
            palm_regression_max = value;
        }
    }

    if ((palm_finite_score_count == 0U) ||
        (palm_finite_regression_count == 0U)) {

        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    /* ---------------------------------------------------------
     * 3. Fingeralphabet-Modell testen
     * --------------------------------------------------------- */
    ai_test_stage = AI_TEST_STAGE_FINGERALPHABET;

    static const uint8_t ai_test_input[AI_INPUT_SIZE] = {
        128, 128, 134, 119, 135, 102, 130,  90, 125,  83, 136,
         87, 137,  69, 137,  58, 137,  48, 132,  86, 133,  67,
        133,  55, 134,  45, 128,  87, 128,  70, 128,  60, 129,
         50, 123,  91, 123,  78, 123,  70, 123,  63, 175, 235,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
    };

    static uint8_t ai_output[AI_OUTPUT_SIZE];

    if (!AI_Run(ai_test_input, ai_output)) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    AI_Result_TypeDef result = AI_GetResult(ai_output);

    predicted_index = result.class_index;
    predicted_score = result.score;
    predicted_label = result.label;

    /* ---------------------------------------------------------
     * 4. Landmark-Modell testen
     * --------------------------------------------------------- */
    ai_test_stage = AI_TEST_STAGE_LANDMARK;

    memset(
        landmark_test_input,
        128,
        LANDMARK_INPUT_SIZE
    );

    landmark_test_ok = AI_RunLandmark(
        landmark_test_input,
        &landmark_test_output
    );

    if (!landmark_test_ok) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    landmark_scalar_0 = landmark_test_output.scalar_0;
    landmark_scalar_1 = landmark_test_output.scalar_1;
    landmark_vector_0_first = landmark_test_output.vector_0[0];
    landmark_vector_1_first = landmark_test_output.vector_1[0];

    landmark_min = landmark_test_output.vector_0[0];
    landmark_max = landmark_test_output.vector_0[0];

    landmark_finite_count = 0U;
    landmark_nan_count = 0U;

    for (uint32_t i = 0U; i < LANDMARK_VECTOR_SIZE; i++) {
        float value = landmark_test_output.vector_0[i];

        if (isfinite(value)) {
            landmark_finite_count++;

            if (value < landmark_min) {
                landmark_min = value;
            }

            if (value > landmark_max) {
                landmark_max = value;
            }
        } else {
            landmark_nan_count++;
        }
    }

    for (uint32_t i = 0U; i < LANDMARK_VECTOR_SIZE; i++) {
        float value = landmark_test_output.vector_1[i];

        if (isfinite(value)) {
            landmark_finite_count++;

            if (value < landmark_min) {
                landmark_min = value;
            }

            if (value > landmark_max) {
                landmark_max = value;
            }
        } else {
            landmark_nan_count++;
        }
    }

    if (isfinite(landmark_test_output.scalar_0)) {
        landmark_finite_count++;
    } else {
        landmark_nan_count++;
    }

    if (isfinite(landmark_test_output.scalar_1)) {
        landmark_finite_count++;
    } else {
        landmark_nan_count++;
    }

    if (landmark_finite_count == 0U) {
        ai_test_stage = AI_TEST_STAGE_ERROR;
        return false;
    }

    ai_test_stage = AI_TEST_STAGE_DONE;
    return true;
}
