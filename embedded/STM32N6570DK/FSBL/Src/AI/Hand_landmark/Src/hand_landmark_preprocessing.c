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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "hand_landmark_preprocessing.h"
#include "hand_landmark.h"

#define RGB888_CHANNEL_COUNT 3U

bool LANDMARK_PreprocessROI(const uint8_t *source, uint32_t source_width, uint32_t source_height,
							uint32_t source_stride_bytes, const HandROI_TypeDef *roi, uint8_t *destination)
{
    if ((source == NULL) ||
        (roi == NULL) ||
        (destination == NULL) ||
        (source_width == 0U) ||
        (source_height == 0U) ||
        (source_stride_bytes < (source_width * RGB888_CHANNEL_COUNT)) ||
        (roi->width <= 0.0f) ||
        (roi->height <= 0.0f)) {
        return false;
    }

    const float roi_left = roi->center_x - (roi->width * 0.5f);

    const float roi_top = roi->center_y - (roi->height * 0.5f);

    const float step_x = roi->width / (float)LANDMARK_INPUT_WIDTH;

    const float step_y = roi->height / (float)LANDMARK_INPUT_HEIGHT;

    for (uint32_t dst_y = 0U; dst_y < LANDMARK_INPUT_HEIGHT; dst_y++) {
        const float normalized_y = roi_top + ((float)dst_y + 0.5f) * step_y;
        const float source_y_f = normalized_y * (float)source_height;

        for (uint32_t dst_x = 0U; dst_x < LANDMARK_INPUT_WIDTH;  dst_x++) {
        	const float normalized_x = roi_left + ((float)dst_x + 0.5f) * step_x;
            const float source_x_f = normalized_x * (float)source_width;
            const uint32_t destination_offset = (dst_y * LANDMARK_INPUT_WIDTH + dst_x) * RGB888_CHANNEL_COUNT;

            if ((source_x_f < 0.0f) ||
                (source_y_f < 0.0f) ||
                (source_x_f >= (float)source_width) ||
                (source_y_f >= (float)source_height)) {

                destination[destination_offset + 0U] = 0U;
                destination[destination_offset + 1U] = 0U;
                destination[destination_offset + 2U] = 0U;

                continue;
            }

            const uint32_t source_x = (uint32_t)source_x_f;
            const uint32_t source_y = (uint32_t)source_y_f;
            const uint32_t source_offset = source_y * source_stride_bytes + source_x * RGB888_CHANNEL_COUNT;

            /* Kamerabuffer BGR888 -> Modellinput RGB888 */
            destination[destination_offset + 0U] = source[source_offset + 2U];
            destination[destination_offset + 1U] = source[source_offset + 1U];
            destination[destination_offset + 2U] = source[source_offset + 0U];
        }
    }
    return true;
}


