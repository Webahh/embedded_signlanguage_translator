/*
 * hand_landmark_postprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include <math.h>
#include <stddef.h>

#include "hand_landmark.h"

#define LANDMARK_TRACKING_ROI_SCALE       1.8f
#define LANDMARK_TRACKING_ROI_SHIFT_Y    -0.05f
#define LANDMARK_TRACKING_ROI_SMOOTHING   0.35f
#define LANDMARK_TRACKING_MIN_SIZE_PX     32.0f

static void LANDMARK_UpdateROICorners(HandROI_TypeDef *roi)
{
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
}

AI_Status_TypeDef LANDMARK_MapToFrame(const LandmarkNetworkOutput_TypeDef *output, const HandROI_TypeDef *roi,
									  LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT])
{
    if ((output == NULL) ||
        (roi == NULL) 	 ||
        (points == NULL)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    const float roi_left = roi->center_x - roi->width * 0.5f;
    const float roi_top  = roi->center_y - roi->height * 0.5f;

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        const float crop_x = output->landmarks[i * 3U + 0U];
        const float crop_y = output->landmarks[i * 3U + 1U];
        const float crop_z = output->landmarks[i * 3U + 2U];

        points[i].x = roi_left + (crop_x / (float)LANDMARK_INPUT_WIDTH)  * roi->width;
        points[i].y = roi_top  + (crop_y / (float)LANDMARK_INPUT_HEIGHT) * roi->height;
        points[i].z = crop_z;
    }

    return AI_STATUS_OK;
}

AI_Status_TypeDef LANDMARK_UpdateROI(const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT], uint32_t frame_width,
					    uint32_t frame_height, HandROI_TypeDef *roi)
{
    if ((points == NULL) ||
        (roi == NULL) ||
        (frame_width == 0U) ||
        (frame_height == 0U)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    float min_x = 1.0f;
    float min_y = 1.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        const float x = points[i].x;
        const float y = points[i].y;

        if (!isfinite(x) || !isfinite(y)) {
            return AI_STATUS_POSTPROCESS_ERROR;
        }

        if (x < min_x) {
            min_x = x;
        }

        if (x > max_x) {
            max_x = x;
        }

        if (y < min_y) {
            min_y = y;
        }

        if (y > max_y) {
            max_y = y;
        }
    }

    const float box_width_px = (max_x - min_x) * (float)frame_width;
    const float box_height_px = (max_y - min_y) * (float)frame_height;
    float target_size_px = fmaxf(box_width_px, box_height_px) * LANDMARK_TRACKING_ROI_SCALE;

    const float previous_size_px = roi->width * (float)frame_width;
    const float minimum_size_from_previous = previous_size_px * 0.92f;

    if (target_size_px < minimum_size_from_previous) {
        target_size_px = minimum_size_from_previous;
    }

    if (target_size_px < LANDMARK_TRACKING_MIN_SIZE_PX) {
        target_size_px = LANDMARK_TRACKING_MIN_SIZE_PX;
    }

    const float maximum_size_from_previous = previous_size_px * 1.20f;

    if (target_size_px > maximum_size_from_previous) {
        target_size_px = maximum_size_from_previous;
    }

    float target_center_x = (min_x + max_x) * 0.5f;
    float target_center_y = (min_y + max_y) * 0.5f;

    target_center_y += (target_size_px * LANDMARK_TRACKING_ROI_SHIFT_Y) / (float)frame_height;

    const float target_width  = target_size_px / (float)frame_width;
    const float target_height = target_size_px / (float)frame_height;

    const float alpha = LANDMARK_TRACKING_ROI_SMOOTHING;

    roi->center_x += alpha * (target_center_x - roi->center_x);
    roi->center_y += alpha * (target_center_y - roi->center_y);
    roi->width    += alpha * (target_width - roi->width);
    roi->height   += alpha * (target_height - roi->height);
    roi->rotation = 0.0f;

    LANDMARK_UpdateROICorners(roi);

    return AI_STATUS_OK;
}

