/*
 * hand_landmark_postprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include <math.h>
#include <stddef.h>

#include "hand_landmark.h"

#ifndef AI_PI
#define AI_PI 3.14159265358979323846f
#endif

static float ROI_NormalizeAngle(float angle)
{
    while (angle >= AI_PI) {
        angle -= 2.0f * AI_PI;
    }

    while (angle < -AI_PI) {
        angle += 2.0f * AI_PI;
    }

    return angle;
}

static void ROI_UpdateRotatedCorners(HandROI_TypeDef *roi)
{
    const float half_width  = roi->width  * 0.5f;
    const float half_height = roi->height * 0.5f;

    const float c = cosf(roi->rotation);
    const float s = sinf(roi->rotation);

    const float local[4][2] = {
        { -half_width, -half_height },
        {  half_width, -half_height },
        {  half_width,  half_height },
        { -half_width,  half_height }
    };

    for (uint32_t i = 0U; i < 4U; i++) {
        const float x = local[i][0];
        const float y = local[i][1];

        roi->corners[i][0] = roi->center_x + x * c - y * s;
        roi->corners[i][1] = roi->center_y + x * s + y * c;
    }
}

static void ROI_ShiftAndScale(HandROI_TypeDef *roi, float shift_x,
							  float shift_y, float scale_x, float scale_y)
{
    const float c = cosf(roi->rotation);
    const float s = sinf(roi->rotation);

    const float sx = roi->width * shift_x * c - roi->height * shift_y * s;
    const float sy = roi->width * shift_x * s + roi->height * shift_y * c;

    roi->center_x += sx;
    roi->center_y += sy;

    const float long_side = fmaxf(roi->width, roi->height);

    roi->width  = long_side * scale_x;
    roi->height = long_side * scale_y;
}

AI_Status_TypeDef LANDMARK_MapToFrame(const LandmarkNetworkOutput_TypeDef *output, const HandROI_TypeDef *roi,
                                      uint32_t frame_width, uint32_t frame_height, LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT])
{
    if ((output == NULL)      ||
        (roi == NULL)         ||
        (points == NULL)      ||
        (frame_width == 0U)   ||
        (frame_height == 0U)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    const float c = cosf(roi->rotation);
    const float s = sinf(roi->rotation);

    const float roi_cx_px = roi->center_x;
    const float roi_cy_px = roi->center_y;
    const float roi_w_px  = roi->width;
    const float roi_h_px  = roi->height;

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {

        const float crop_x = output->landmarks[i * 3U + 0U];
        const float crop_y = output->landmarks[i * 3U + 1U];
        const float crop_z = output->landmarks[i * 3U + 2U];

        const float lm_x = (crop_x / (float)LANDMARK_INPUT_WIDTH)  - 0.5f;
        const float lm_y = (crop_y / (float)LANDMARK_INPUT_HEIGHT) - 0.5f;

        const float local_x_px = lm_x * roi_w_px;
        const float local_y_px = lm_y * roi_h_px;

        const float frame_x_px = roi_cx_px + local_x_px * c - local_y_px * s;
        const float frame_y_px = roi_cy_px + local_x_px * s + local_y_px * c;

        points[i].x = frame_x_px / (float)frame_width;
        points[i].y = frame_y_px / (float)frame_height;
        points[i].z = crop_z;
    }

    return AI_STATUS_OK;
}

AI_Status_TypeDef LANDMARK_UpdateROIFromNetworkOutput(const LandmarkNetworkOutput_TypeDef *output,
													  const HandROI_TypeDef *current_roi, HandROI_TypeDef *next_roi)
{
    if ((output == NULL) 	  ||
        (current_roi == NULL) ||
        (next_roi == NULL)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    LandmarkPoint_TypeDef decoded[LANDMARK_POINT_COUNT];

    const float c = cosf(current_roi->rotation);
    const float s = sinf(current_roi->rotation);

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        const float crop_x = output->landmarks[i * 3U + 0U] / (float)LANDMARK_INPUT_WIDTH;
        const float crop_y = output->landmarks[i * 3U + 1U] / (float)LANDMARK_INPUT_HEIGHT;
        const float local_x = (crop_x - 0.5f) * current_roi->width;
        const float local_y = (crop_y - 0.5f) * current_roi->height;

        decoded[i].x = current_roi->center_x + local_x * c - local_y * s;
        decoded[i].y = current_roi->center_y + local_x * s + local_y * c;
        decoded[i].z = output->landmarks[i * 3U + 2U];
    }

    const float palm_center_x =
        (decoded[5].x +
         decoded[9].x +
         decoded[13].x +
         decoded[17].x) * 0.25f;

    const float palm_center_y =
        (decoded[5].y +
         decoded[9].y +
         decoded[13].y +
         decoded[17].y) * 0.25f;

    const float dx = palm_center_x - decoded[0].x;
    const float dy = palm_center_y - decoded[0].y;

    if (isfinite(dx) && isfinite(dy) && ((dx * dx + dy * dy) > 0.000001f)) {
        next_roi->rotation = ROI_NormalizeAngle((AI_PI * 0.5f) - atan2f(-dy, dx));
    }
    else {
        next_roi->rotation = current_roi->rotation;
    }

    float min_x =  1000000.0f;
    float min_y =  1000000.0f;
    float max_x = -1000000.0f;
    float max_y = -1000000.0f;

    for (uint32_t n = 0U; n < LANDMARK_POINT_COUNT; n++) {
        if (!isfinite(decoded[n].x) || !isfinite(decoded[n].y)) {
            return AI_STATUS_POSTPROCESS_ERROR;
        }

        if (decoded[n].x < min_x) { min_x = decoded[n].x; }
        if (decoded[n].x > max_x) { max_x = decoded[n].x; }
        if (decoded[n].y < min_y) { min_y = decoded[n].y; }
        if (decoded[n].y > max_y) { max_y = decoded[n].y; }
    }

    next_roi->center_x = (max_x + min_x) * 0.5f;
    next_roi->center_y = (max_y + min_y) * 0.5f;
    next_roi->width    = max_x - min_x;
    next_roi->height   = max_y - min_y;

    if ((next_roi->width <= 0.0f) || (next_roi->height <= 0.0f)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    ROI_ShiftAndScale(next_roi, 0.0f, -0.1f, 2.0f, 2.0f);
    ROI_UpdateRotatedCorners(next_roi);

    return AI_STATUS_OK;
}

