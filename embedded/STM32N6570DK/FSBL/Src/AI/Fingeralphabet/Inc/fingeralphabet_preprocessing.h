/*
 * fingeralphabet_preprocessing.h
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#ifndef FINGERALPHABET_PREPROCESSING_H
#define FINGERALPHABET_PREPROCESSING_H

#include <stdbool.h>
#include <stdint.h>

#include "fingeralphabet.h"
#include "hand_landmark.h"
#include "hand_landmark_postprocessing.h"

bool FINGERALPHABET_Preprocess(const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT], float handedness,
							   uint8_t output[FINGERALPHABET_INPUT_SIZE]);

#endif /* FINGERALPHABET_PREPROCESSING_H */
