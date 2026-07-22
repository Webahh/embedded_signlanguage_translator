/*
 * palm_postprocessing.c
 *
 *  Created on: 01.07.2026
 *      Author: Weber
 */

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "palm_detection.h"
#include "pd_anchors.h"
#include "simple_camera.h"

#define PALM_CONFIRM_FRAME_COUNT  2U
#define PALM_LOST_FRAME_COUNT     3U

#ifndef AI_PI
#define AI_PI 3.14159265358979323846f
#endif

typedef struct {
    float probability;
    float center_x;
    float center_y;
    float width;
    float height;
    float keypoints[PALM_KEYPOINT_COUNT][2];
    uint16_t anchor_index;
} PalmCandidate_TypeDef;

static float PALM_Sigmoid(float x)
{
	if (x < -8.0f) return 0.0f;
	if (x > 8.0f) return 1.0f;

	// Pade Approcimation - good for ~1e-3
	float x2 = x * x;
	return (0.5f + 0.25f * x) / (1.0f - 0.25f * x + 0.125f * x2);
}

static int PALM_CompareCandidates(const void *candidate_a, const void *candidate_b)
{
    const PalmCandidate_TypeDef *a = (const PalmCandidate_TypeDef *)candidate_a;
    const PalmCandidate_TypeDef *b = (const PalmCandidate_TypeDef *)candidate_b;

    if (a->probability < b->probability) { return 1; }
    if (a->probability > b->probability) { return -1;}

    return 0;
}

static float PALM_ComputeIoU(const PalmCandidate_TypeDef *a, const PalmCandidate_TypeDef *b)
{
    const float a_left   = a->center_x - a->width  * 0.5f;
    const float a_top    = a->center_y - a->height * 0.5f;
    const float a_right  = a->center_x + a->width  * 0.5f;
    const float a_bottom = a->center_y + a->height * 0.5f;

    const float b_left   = b->center_x - b->width  * 0.5f;
    const float b_top    = b->center_y - b->height * 0.5f;
    const float b_right  = b->center_x + b->width  * 0.5f;
    const float b_bottom = b->center_y + b->height * 0.5f;

    const float intersection_left   = fmaxf(a_left, b_left);
    const float intersection_top    = fmaxf(a_top, b_top);
    const float intersection_right  = fminf(a_right, b_right);
    const float intersection_bottom = fminf(a_bottom, b_bottom);

    const float intersection_width  = fmaxf(intersection_right - intersection_left, 0.0f);
    const float intersection_height = fmaxf(intersection_bottom - intersection_top, 0.0f);
    const float intersection_area   = intersection_width * intersection_height;

    const float area_a = fmaxf(a->width, 0.0f) * fmaxf(a->height, 0.0f);
    const float area_b = fmaxf(b->width, 0.0f) * fmaxf(b->height, 0.0f);
    const float union_area = area_a + area_b - intersection_area;

    if (union_area <= 0.0f) {
        return 0.0f;
    }

    return intersection_area / union_area;
}

static bool PALM_DecodeCandidate(const PalmNetworkOutput_TypeDef *network_output,
								 uint32_t anchor_index, PalmCandidate_TypeDef *candidate)
{
    const pd_pp_point_t *anchor = PD_GetAnchor(anchor_index);

    if ((network_output == NULL) ||
        (candidate == NULL) 	 ||
        (anchor == NULL)) {
        return false;
    }

    const float input_width  = (float)PALM_INPUT_WIDTH;
    const float input_height = (float)PALM_INPUT_HEIGHT;
    const float *regression = &network_output->regressions[anchor_index * PALM_REGRESSION_SIZE];

    candidate->anchor_index = anchor_index;
    candidate->probability = PALM_Sigmoid(network_output->scores[anchor_index]);

    candidate->center_x = (anchor->x * input_width +
    					   regression[PALM_BOX_X_CENTER_INDEX]) /
    					   input_width;

    candidate->center_y = (anchor->y * input_height +
    					   regression[PALM_BOX_Y_CENTER_INDEX]) /
    					   input_height;

    candidate->width  = regression[PALM_BOX_WIDTH_INDEX]  / input_width;
    candidate->height = regression[PALM_BOX_HEIGHT_INDEX] / input_height;

    for (uint32_t i = 0U; i < PALM_KEYPOINT_COUNT; i++) {
        candidate->keypoints[i][0] = (anchor->x * input_width +
        							  regression[PALM_BOX_KEYPOINT_OFFSET + i * 2U + 0U]) /
									  input_width;

        candidate->keypoints[i][1] = (anchor->y * input_height +
        							  regression[PALM_BOX_KEYPOINT_OFFSET + i * 2U + 1U]) /
									  input_height;
    }
    return true;
}

static void PALM_InsertCandidate(PalmCandidate_TypeDef candidates[PALM_PP_MAX_CANDIDATES],
							     uint32_t *candidate_count, const PalmCandidate_TypeDef *candidate)
{
    if (*candidate_count < PALM_PP_MAX_CANDIDATES) {
        candidates[*candidate_count] = *candidate;
        (*candidate_count)++;
        return;
    }

    uint32_t weakest_index = 0U;

    for (uint32_t i = 1U; i < PALM_PP_MAX_CANDIDATES; i++) {

        if (candidates[i].probability < candidates[weakest_index].probability) {
            weakest_index = i;
        }
    }

    if (candidate->probability > candidates[weakest_index].probability) {
        candidates[weakest_index] = *candidate;
    }
}

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

static void ROI_ShiftAndScale(HandROI_TypeDef *roi,
                              float shift_x,
                              float shift_y,
                              float scale_x,
                              float scale_y)
{
    const float c = cosf(roi->rotation);
    const float s = sinf(roi->rotation);

    const float sx =
        roi->width * shift_x * c -
        roi->height * shift_y * s;

    const float sy =
        roi->width * shift_x * s +
        roi->height * shift_y * c;

    roi->center_x += sx;
    roi->center_y += sy;

    const float long_side = fmaxf(roi->width, roi->height);

    roi->width  = long_side * scale_x;
    roi->height = long_side * scale_y;
}

AI_Status_TypeDef PALM_Postprocess(const PalmNetworkOutput_TypeDef *network_output,
					  	  	  	   PalmDetection_TypeDef *detection)
{
    if ((network_output == NULL) ||
        (network_output->scores == NULL) ||
        (network_output->regressions == NULL) ||
        (detection == NULL)) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    PalmCandidate_TypeDef candidates[PALM_PP_MAX_CANDIDATES];
    PalmCandidate_TypeDef filtered[PALM_PP_MAX_CANDIDATES];
    uint32_t candidate_count = 0U;
    uint32_t filtered_count = 0U;

     // log(conf / (1 - conf)) conf Threshold
    const float score_threshold = -logf(1.0f / PALM_PP_CONFIDENCE_THRESHOLD - 1.0f);

    for (uint32_t i = 0U; i < PALM_DETECTION_COUNT; i++) {
        if (network_output->scores[i] < score_threshold) {
            continue;
        }

        PalmCandidate_TypeDef candidate;
        if (!PALM_DecodeCandidate(network_output, i, &candidate)) {
            continue;
        }

        /*
         * delete invalid candidates.
         */
        if ((candidate.width <= 0.0f) 		||
            (candidate.height <= 0.0f) 		||
            (!isfinite(candidate.center_x)) ||
            (!isfinite(candidate.center_y)) ||
            (!isfinite(candidate.width)) 	||
            (!isfinite(candidate.height))) {
            continue;
        }
        PALM_InsertCandidate(candidates, &candidate_count, &candidate);
    }

    if (candidate_count == 0U) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    qsort(candidates, candidate_count, sizeof(PalmCandidate_TypeDef), PALM_CompareCandidates);

    // Non-Maximum-Suppression.
    for (uint32_t i = 0U; i < candidate_count; i++) {
        bool suppressed = false;

        for (uint32_t j = 0U; j < filtered_count; j++) {
            const float iou = PALM_ComputeIoU(&candidates[i], &filtered[j]);
            if (iou >= PALM_PP_IOU_THRESHOLD) {
                suppressed = true;
                break;
            }
        }

        if (!suppressed) {
            filtered[filtered_count] = candidates[i];
            filtered_count++;

            if (filtered_count >= PALM_PP_MAX_CANDIDATES) {
                break;
            }
        }
    }

    if (filtered_count == 0U) {
        return AI_STATUS_POSTPROCESS_ERROR;
    }

    // After sorting and NMS is element 0 the strongest box
    const PalmCandidate_TypeDef *best = &filtered[0];

    detection->probability 	= best->probability;
    detection->center_x 	= best->center_x;
    detection->center_y 	= best->center_y;
    detection->width 		= best->width;
    detection->height 		= best->height;
    detection->anchor_index = best->anchor_index;

    for (uint32_t i = 0U; i < PALM_KEYPOINT_COUNT; i++) {
        detection->keypoints[i][0] = best->keypoints[i][0];
        detection->keypoints[i][1] = best->keypoints[i][1];
    }
    return AI_STATUS_OK;
}

void PALM_UpdateDetectionFilter(PalmDetectionFilter_TypeDef *filter, bool detection_valid)
{
    if (filter == NULL) {
        return;
    }

    if (detection_valid) {
        filter->negative_count = 0U;

        if (filter->positive_count < PALM_CONFIRM_FRAME_COUNT) {
            filter->positive_count++;
        }

        if (filter->positive_count >= PALM_CONFIRM_FRAME_COUNT) {
            filter->detected = true;
        }
    }
    else {
        filter->positive_count = 0U;

        if (filter->negative_count < PALM_LOST_FRAME_COUNT) {
            filter->negative_count++;
        }

        if (filter->negative_count >= PALM_LOST_FRAME_COUNT) {
            filter->detected = false;
        }
    }
}

AI_Status_TypeDef PALM_CreateLandmarkROI(const PalmDetection_TypeDef *detection, uint32_t frame_width,
                                         uint32_t frame_height, HandROI_TypeDef *roi)
{
      if ((detection == NULL) ||
          (roi       == NULL) ||
          (frame_width == 0U) ||
          (frame_height == 0U)) {
          return AI_STATUS_POSTPROCESS_ERROR;
      }

      const float shift_x = 0.0f;
      const float shift_y = -0.5f;
      const float scale   = 2.6f;

      /*
       * Palm detection runs on the NN pipe output (sensor squished to 192x192)
       * The ROI is used on the display pipe output (center-cropped to 800x480)
       *
       * X mapping: both pipes span the full sensor width -> scale by frame_width
       * Y mapping: NN pipe spans sensor height 0..1944, display pipe spans
       *            sensor crop_y..crop_y+crop_height -> compute corrected scale
       */
      const float crop_height = (float)frame_height * (float)CAM_SENSOR_WIDTH / (float)frame_width;
      const float crop_y      = ((float)CAM_SENSOR_HEIGHT - crop_height + 1.0f) * 0.5f;
      const float y_scale     = (float)CAM_SENSOR_HEIGHT * (float)frame_height / crop_height;
      const float y_offset    = -crop_y * (float)frame_height / crop_height;

      roi->center_x = detection->center_x * (float)frame_width;
      roi->center_y = detection->center_y * y_scale + y_offset;
      roi->width    = detection->width    * (float)frame_width;
      roi->height   = detection->height   * y_scale;

      const float x0 = detection->keypoints[0][0] * (float)frame_width;
      const float y0 = detection->keypoints[0][1] * y_scale + y_offset;
      const float x1 = detection->keypoints[2][0] * (float)frame_width;
      const float y1 = detection->keypoints[2][1] * y_scale + y_offset;

      const float dx = x1 - x0;
      const float dy = y1 - y0;

      if (isfinite(dx) && isfinite(dy) && ((dx * dx + dy * dy) > 0.000001f)) {
          roi->rotation = ROI_NormalizeAngle((AI_PI * 0.5f) - atan2f(-dy, dx));
      }
      else {
          roi->rotation = 0.0f;
      }

      ROI_ShiftAndScale(roi, shift_x, shift_y, scale, scale);
      ROI_UpdateRotatedCorners(roi);

      return AI_STATUS_OK;
  }

