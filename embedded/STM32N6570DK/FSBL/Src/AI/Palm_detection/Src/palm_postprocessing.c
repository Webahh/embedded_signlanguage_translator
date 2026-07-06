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

#include "palm_postprocessing.h"
#include "pd_anchors.h"

#define PALM_CONFIRM_FRAME_COUNT  2U
#define PALM_LOST_FRAME_COUNT     3U

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

bool PALM_Postprocess(const PalmNetworkOutput_TypeDef *network_output,
					  PalmDetection_TypeDef *detection)
{
    if ((network_output == NULL) ||
        (network_output->scores == NULL) ||
        (network_output->regressions == NULL) ||
        (detection == NULL)) {
        return false;
    }

    PalmCandidate_TypeDef candidates[PALM_PP_MAX_CANDIDATES];
    PalmCandidate_TypeDef filtered[PALM_PP_MAX_CANDIDATES];
    uint32_t candidate_count = 0U;
    uint32_t filtered_count = 0U;

    /*
     * Für conf_threshold = 0.5 ist der Logit-Schwellwert 0.
     * Allgemein:
     *
     * log(conf / (1 - conf))
     */
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
        return false;
    }

    qsort(candidates, candidate_count, sizeof(PalmCandidate_TypeDef), PALM_CompareCandidates);

    /*
     * Non-Maximum-Suppression.
     */
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
        return false;
    }

    /*
     * Nach Sortierung und NMS ist Element 0 die stärkste Box.
     */
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
    return true;
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

bool PALM_CreateLandmarkROI(const PalmDetection_TypeDef *detection, uint32_t frame_width,
							uint32_t frame_height, HandROI_TypeDef *roi)
{
    if ((detection    == NULL) ||
        (roi 	      == NULL) ||
        (frame_width  == 0)    ||
        (frame_height == 0)) {
        return false;
    }

    const float shift_y = -0.5f;
    const float scale = 2.6f;

    const float detection_width_px = detection->width * (float)frame_width;
    const float detection_height_px = detection->height * (float)frame_height;
    const float roi_size_px = fmaxf(detection_width_px, detection_height_px) * scale;

    roi->center_x = detection->center_x;
    roi->center_y = detection->center_y + ((detection_height_px * shift_y) / (float)frame_height);

    roi->width = roi_size_px / (float)frame_width;
    roi->height = roi_size_px / (float)frame_height;
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

