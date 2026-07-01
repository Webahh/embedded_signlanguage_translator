/*
 * hand_landmark_preprocessing.h
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#ifndef HAND_LANDMARK_PREPROCESSING_H
#define HAND_LANDMARK_PREPROCESSING_H

#include "palm_postprocessing.h"

bool LANDMARK_PreprocessROI(const uint8_t *source, uint32_t source_width, uint32_t source_height,
							uint32_t source_stride_bytes, const HandROI_TypeDef *roi, uint8_t *destination);

#endif /* HAND_LANDMARK_PREPROCESSING_H */
