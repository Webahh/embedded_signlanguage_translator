/*
 * hand_landmark_preprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

/*
 * hand_landmark_preprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "hand_landmark.h"

#define RGB888_CHANNEL_COUNT 3U

AI_Status_TypeDef LANDMARK_PreprocessROI(const uint8_t *source, uint32_t source_width, uint32_t source_height,
                                         uint32_t source_stride_bytes, const HandROI_TypeDef *roi, uint8_t *destination)
{
    if ((source == NULL)        ||
        (roi == NULL)           ||
        (destination == NULL)   ||
        (source_width == 0U)    ||
        (source_height == 0U)   ||
        (source_stride_bytes < (source_width * RGB888_CHANNEL_COUNT)) ||
        (roi->width <= 0.0f)    ||
        (roi->height <= 0.0f)) {
        return AI_STATUS_PREPROCESS_ERROR;
    }

    const float c = cosf(roi->rotation);
    const float s = sinf(roi->rotation);

    const float roi_cx_px = roi->center_x;
    const float roi_cy_px = roi->center_y;
    const float roi_w_px  = roi->width;
    const float roi_h_px  = roi->height;

    for (uint32_t dst_y = 0U; dst_y < LANDMARK_INPUT_HEIGHT; dst_y++) {
        const float norm_y = ((float)dst_y / (float)(LANDMARK_INPUT_HEIGHT - 1U)) - 0.5f;

        for (uint32_t dst_x = 0U; dst_x < LANDMARK_INPUT_WIDTH; dst_x++) {
            const float norm_x = ((float)dst_x / (float)(LANDMARK_INPUT_WIDTH - 1U)) - 0.5f;

            const float local_x_px = norm_x * roi_w_px;
            const float local_y_px = norm_y * roi_h_px;

            const float source_x_f = roi_cx_px + local_x_px * c - local_y_px * s;
            const float source_y_f = roi_cy_px + local_x_px * s + local_y_px * c;

            const uint32_t destination_offset = (dst_y * LANDMARK_INPUT_WIDTH + dst_x) * RGB888_CHANNEL_COUNT;

            if ((source_x_f < 0.0f) ||
                (source_y_f < 0.0f) ||
                (source_x_f >= (float)(source_width - 1U)) ||
                (source_y_f >= (float)(source_height - 1U))) {

                destination[destination_offset + 0U] = 0U;
                destination[destination_offset + 1U] = 0U;
                destination[destination_offset + 2U] = 0U;
                continue;
            }

            const uint32_t x0 = (uint32_t)source_x_f;
            const uint32_t y0 = (uint32_t)source_y_f;
            const uint32_t x1 = x0 + 1U;
            const uint32_t y1 = y0 + 1U;

            const float fx = source_x_f - (float)x0;
            const float fy = source_y_f - (float)y0;

            const float w00 = (1.0f - fx) * (1.0f - fy);
            const float w10 = fx * (1.0f - fy);
            const float w01 = (1.0f - fx) * fy;
            const float w11 = fx * fy;

            const uint32_t off00 = y0 * source_stride_bytes + x0 * RGB888_CHANNEL_COUNT;
            const uint32_t off10 = y0 * source_stride_bytes + x1 * RGB888_CHANNEL_COUNT;
            const uint32_t off01 = y1 * source_stride_bytes + x0 * RGB888_CHANNEL_COUNT;
            const uint32_t off11 = y1 * source_stride_bytes + x1 * RGB888_CHANNEL_COUNT;

            /*
             * Source is BGR888, Modelinput is RGB888.
             */
            for (uint32_t ch = 0U; ch < 3U; ch++) {

                const uint32_t src_ch =
                    (ch == 0U) ? 2U :
                    (ch == 1U) ? 1U :
                                 0U;

                const float value =
                    (float)source[off00 + src_ch] * w00 +
                    (float)source[off10 + src_ch] * w10 +
                    (float)source[off01 + src_ch] * w01 +
                    (float)source[off11 + src_ch] * w11;

                float clamped = value;

                if (clamped < 0.0f) {
                    clamped = 0.0f;
                }
                else if (clamped > 255.0f) {
                    clamped = 255.0f;
                }

                destination[destination_offset + ch] = (uint8_t)(clamped + 0.5f);
            }
        }
    }
    return AI_STATUS_OK;
}


