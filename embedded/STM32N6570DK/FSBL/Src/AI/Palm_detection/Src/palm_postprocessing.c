/*
 * palm_postprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include <math.h>
#include <stddef.h>

#include "palm_postprocessing.h"
#include "pd_anchors.h"

#define PALM_MODEL_INPUT_SIZE 192.0f

bool PALM_FindBestDetection(const PalmNetworkOutput_TypeDef *output, PalmDetection_TypeDef *detection)
{
    if ((output == NULL) || (detection == NULL)) {
        return false;
    }

    uint32_t best_index = 0U;
    float best_score = output->scores[0];

    for (uint32_t i = 1U; i < PALM_DETECTION_COUNT; i++) {
        if (output->scores[i] > best_score) {
            best_score = output->scores[i];
            best_index = i;
        }
    }

    const pd_pp_point_t *anchor = PD_GetAnchor(best_index);

    if (anchor == NULL) {
        return false;
    }

    const float *regression = &output->regressions[best_index * PALM_REGRESSION_SIZE];

    detection->center_x = anchor->x + regression[0] / PALM_MODEL_INPUT_SIZE;
    detection->center_y =anchor->y + regression[1] / PALM_MODEL_INPUT_SIZE;
    detection->width = regression[2] / PALM_MODEL_INPUT_SIZE;
    detection->height = regression[3] / PALM_MODEL_INPUT_SIZE;

    for (uint32_t i = 0U; i < PALM_KEYPOINT_COUNT; i++) {
        detection->keypoints[i][0] = anchor->x + regression[4U + 2U * i] / PALM_MODEL_INPUT_SIZE;

        detection->keypoints[i][1] = anchor->y + regression[5U + 2U * i] / PALM_MODEL_INPUT_SIZE;
    }

    detection->score = best_score;
    detection->probability = 1.0f / (1.0f + expf(-best_score));
    detection->anchor_index = best_index;

    return true;
}

void PALM_UpdateDetectionFilter(PalmDetectionFilter_TypeDef *filter, uint32_t probability_permille)
{
    if (filter == NULL) {
        return;
    }

    if (probability_permille >= PALM_DETECTION_THRESHOLD_PERMILLE) {
        filter->negative_count = 0U;

        if (filter->positive_count < PALM_CONFIRM_FRAME_COUNT) {
            filter->positive_count++;
        }

        if (filter->positive_count >= PALM_CONFIRM_FRAME_COUNT) {
            filter->detected = true;
        }

    } else {
        filter->positive_count = 0U;

        if (filter->negative_count < PALM_CONFIRM_FRAME_COUNT) {
            filter->negative_count++;
        }

        if (filter->negative_count >= PALM_CONFIRM_FRAME_COUNT) {
            filter->detected = false;
        }
    }
}

bool PALM_CreateLandmarkROI(const PalmDetection_TypeDef *detection, HandROI_TypeDef *roi)
{
    if ((detection == NULL) || (roi == NULL)) {
        return false;
    }

    const float shift_y = -0.5f;
    const float scale = 2.6f;

    const float long_side =
        fmaxf(detection->width, detection->height);

    roi->center_x = detection->center_x;
    roi->center_y =
        detection->center_y +
        detection->height * shift_y;

    roi->width = long_side * scale;
    roi->height = long_side * scale;
    roi->rotation = 0.0f;

    const float half_width = roi->width * 0.5f;
    const float half_height = roi->height * 0.5f;

    roi->corners[0][0] = roi->center_x - half_width;
    roi->corners[0][1] = roi->center_y - half_height;

    roi->corners[1][0] = roi->center_x + half_width;
    roi->corners[1][1] = roi->center_y - half_height;

    roi->corners[2][0] = roi->center_x + half_width;
    roi->corners[2][1] = roi->center_y + half_height;

    roi->corners[3][0] = roi->center_x - half_width;
    roi->corners[3][1] = roi->center_y + half_height;

    return true;
}

