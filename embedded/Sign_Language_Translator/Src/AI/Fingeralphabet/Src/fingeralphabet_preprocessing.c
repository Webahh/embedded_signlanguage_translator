/*
 * fingeralphabet_preprocessing.c
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#ifndef AI_PI
#define AI_PI 3.14159265358979323846f
#endif

#include "fingeralphabet.h"

#define FINGERALPHABET_POS_MAX               32767
#define FINGERALPHABET_HAND_FEATURE_COUNT    44
#define FINGERALPHABET_LEFT_OFFSET           0
#define FINGERALPHABET_RIGHT_OFFSET          44

#define FINGERALPHABET_INPUT_SCALE           0.007842f
#define FINGERALPHABET_INPUT_ZERO_POINT      127.0f
#define FINGERALPHABET_HANDEDNESS_THRESHOLD  0.5f

static float FINGERALPHABET_MirrorNormalizedX(float x)
{
    if (x < 0.0f) {
        x = 0.0f;
    }
    else if (x > 1.0f) {
        x = 1.0f;
    }

    return 1.0f - x;
}

static void FINGERALPHABET_SetMissingHand(int16_t features[FINGERALPHABET_INPUT_SIZE], uint32_t offset)
{
    for (uint32_t i = 0U; i < FINGERALPHABET_HAND_FEATURE_COUNT; i++) {
        features[offset + i] = 0;
    }

    features[offset + 42U] = -100;
    features[offset + 43U] = -100;
}

static int16_t FINGERALPHABET_NormalizedToInt16(float value)
{
    if (value < 0.0f) {
        value = 0.0f;
    }
    else if (value > 1.0f) {
        value = 1.0f;
    }

    return (int16_t)(value * (float)FINGERALPHABET_POS_MAX);
}

static int16_t FINGERALPHABET_RelativeToInt16(float value)
{
    const float scaled = value * (float)FINGERALPHABET_POS_MAX;

    if (scaled > 32767.0f) {
        return 32767;
    }

    if (scaled < -32768.0f) {
        return -32768;
    }

    return (int16_t)scaled;
}

static uint8_t FINGERALPHABET_QuantizeFeature(int16_t feature)
{
    const float normalized = (float)feature / (float)FINGERALPHABET_POS_MAX;

    float quantized = normalized / FINGERALPHABET_INPUT_SCALE + FINGERALPHABET_INPUT_ZERO_POINT;

    if (quantized < 0.0f) {
        quantized = 0.0f;
    }
    else if (quantized > 255.0f) {
        quantized = 255.0f;
    }

    return (uint8_t)quantized;
}

AI_Status_TypeDef FINGERALPHABET_Preprocess(const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT],
												   float handedness, uint8_t output[FINGERALPHABET_INPUT_SIZE])
{
    if ((points == NULL) || (output == NULL)) {
        return AI_STATUS_PREPROCESS_ERROR;
    }

    int16_t features[FINGERALPHABET_INPUT_SIZE];

    /* Setting both hands as missing */
    FINGERALPHABET_SetMissingHand(features, FINGERALPHABET_LEFT_OFFSET);
    FINGERALPHABET_SetMissingHand(features, FINGERALPHABET_RIGHT_OFFSET);

    /*
     * left approx. 0.24
     * right approx. 0.93
     */
    const uint32_t hand_offset = (handedness >= FINGERALPHABET_HANDEDNESS_THRESHOLD)
        ? FINGERALPHABET_RIGHT_OFFSET
        : FINGERALPHABET_LEFT_OFFSET;

    const int16_t wrist_x = FINGERALPHABET_NormalizedToInt16(FINGERALPHABET_MirrorNormalizedX(points[0].x));
    const int16_t wrist_y = FINGERALPHABET_NormalizedToInt16(points[0].y);

    /*
     * 21 wrist-relative Landmarks.
     */
    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        const int16_t point_x = FINGERALPHABET_NormalizedToInt16(FINGERALPHABET_MirrorNormalizedX(points[i].x));
        const int16_t point_y = FINGERALPHABET_NormalizedToInt16(points[i].y);

        features[hand_offset + i * 2U + 0U] = (int16_t)(point_x - wrist_x);
        features[hand_offset + i * 2U + 1U] = (int16_t)(point_y - wrist_y);
    }

    /*
     * Joint 21: origin absolute wrist-position.
     */
    features[hand_offset + 42U] = wrist_x;
    features[hand_offset + 43U] = wrist_y;

    /*
     * 88 int16-Features quant to int8.
     */
    for (uint32_t i = 0U; i < FINGERALPHABET_INPUT_SIZE; i++) {
        output[i] = FINGERALPHABET_QuantizeFeature(features[i]);
    }

    return AI_STATUS_OK;
}

AI_Status_TypeDef FINGERALPHABET_PreprocessFromLandmarkOutput(const LandmarkNetworkOutput_TypeDef *landmark_output,
															  uint8_t output[FINGERALPHABET_INPUT_SIZE])
{
    if ((landmark_output == NULL) || (output == NULL)) {
        return AI_STATUS_PREPROCESS_ERROR;
    }

    LandmarkPoint_TypeDef canonical_points[LANDMARK_POINT_COUNT];

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        canonical_points[i].x = landmark_output->landmarks[i * 3U + 0U] / (float)LANDMARK_INPUT_WIDTH;
        canonical_points[i].y = landmark_output->landmarks[i * 3U + 1U] / (float)LANDMARK_INPUT_HEIGHT;
        canonical_points[i].z = landmark_output->landmarks[i * 3U + 2U];
    }
    return FINGERALPHABET_Preprocess(canonical_points, landmark_output->handedness, output);
}
