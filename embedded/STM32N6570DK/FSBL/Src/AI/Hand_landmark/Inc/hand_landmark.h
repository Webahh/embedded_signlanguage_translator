/*
 * hand_landmark.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */


#ifndef HAND_LANDMARK_H
#define HAND_LANDMARK_H

#include <stdbool.h>
#include <stdint.h>

#include "simple_ai.h"

#define LANDMARK_INPUT_WIDTH    224U
#define LANDMARK_INPUT_HEIGHT   224U
#define LANDMARK_INPUT_CHANNELS   3U
#define LANDMARK_INPUT_SIZE (LANDMARK_INPUT_WIDTH * LANDMARK_INPUT_HEIGHT * LANDMARK_INPUT_CHANNELS)

typedef struct {
    float scalar_0;
    float vector_0[63];
    float scalar_1;
    float vector_1[63];
} LandmarkNetworkOutput_TypeDef;

AI_Status_TypeDef LANDMARK_Init(void);
bool LANDMARK_Run(const uint8_t input[LANDMARK_INPUT_SIZE], LandmarkNetworkOutput_TypeDef *output);


#endif /* HAND_LANDMARK_H */
