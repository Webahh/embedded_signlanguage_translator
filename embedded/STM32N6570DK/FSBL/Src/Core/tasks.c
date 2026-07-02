/*
 * tasks.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <stdio.h>
#include <string.h>

#include "tasks.h"
#include "app.h"
#include "simple_gpio.h"
#include "simple_scheduler.h"

#include "simple_ltdc.h"
#include "simple_ltdc_layer.h"
#include "simple_ltdc_color.h"
#include "simple_ltdc_layer_draw.h"

#include "simple_ae.h"
#include "config.h"
#include "simple_timer.h"
#include "simple_text.h"
#include "simple_touch.h"
#include "hand_landmark.h"
#include "hand_landmark_preprocessing.h"
#include "hand_landmark_postprocessing.h"
#include "ui.h"

#define LED2_PIN 10
#define BG_NUM_COLORS 3

static volatile int ltdc_fg_disp_idx = 1;
static volatile uint8_t nn_frame_ready = 0U;
static volatile uint8_t nn_completed_buffer_idx = 0U;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
    if (pipe == DCMIPP_PIPE1) {
    	AE_OnFrameStats();
        ltdc_layer_bg_buffer_disp_idx ^= 1;
        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[ltdc_layer_bg_buffer_disp_idx];
        LTDC_Layer_Address_Set(&LTDC_Layer1Config);
    } else if (pipe == DCMIPP_PIPE2) {
        nn_completed_buffer_idx = (DCMIPP->P2SR & DCMIPP_P2SR_LSTFRM) ? 1U : 0U;
        nn_frame_ready = 1U;
    }
}

static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;

__attribute__((noinline, optimize("O0"))) // No optimizations for better testing
static int recursion(int n)
{
    volatile uint32_t marker = 0xDEADBEEF;
    volatile uint32_t padding[8];

    padding[0] = marker;

    if (n == 0)
        return padding[0];

    return recursion(n - 1) + 1;
}

void vRecursionTestTask(void) {
	recursion(20);
}

void vSystemTimeTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now); // MAX:     4294967296
	char str[11]; // + '\0'
	snprintf(str, sizeof(str), "%lu", (unsigned long)now);
	TEXT_StringBg_draw(&LTDC_Layer1Config, str, 720, 0, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
}

void vLEDTask(void) {
	GPIO_BSRR_toggle(GPIOG, LED2_PIN);
}

void vBackgroundTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now);
    uint32_t elapsed = now - bg_blend_start;

    if (elapsed >= 1000) {
        bg_seg_idx = (bg_seg_idx + 1) % BG_NUM_COLORS;
        bg_blend_start = now;
        elapsed = 0;
    }

    uint8_t next_idx = (bg_seg_idx + 1) % BG_NUM_COLORS;
    uint32_t t       = elapsed;

    uint8_t r = (uint8_t)(((uint32_t)bg_colors[bg_seg_idx][0] * (1000 - t) + (uint32_t)bg_colors[next_idx][0] * t) / 1000);
    uint8_t g = (uint8_t)(((uint32_t)bg_colors[bg_seg_idx][1] * (1000 - t) + (uint32_t)bg_colors[next_idx][1] * t) / 1000);
    uint8_t b = (uint8_t)(((uint32_t)bg_colors[bg_seg_idx][2] * (1000 - t) + (uint32_t)bg_colors[next_idx][2] * t) / 1000);

    LTDC_BackgroundColor_Set(r, g, b);
}

void vAETask(void){
	AE_Process(&h_cam);
}

// Poll touch, dispatch to drawer, draw a blue dot on press
void vTouchTask(void){
	uint8_t pending = 0;
	TOUCH_GetPending(&pending);
    if (pending)
    {
        TOUCH_Data_TypeDef data;
        TOUCH_GetState(NULL, &data);

        // Route touch events to drawer (toggle, slider, selector, composite)
        UI_Drawer_HandleTouch(&_drawer, data.x, data.y, data.pressed,
            &LTDC_Layer2Config, NULL);

        // Paint touch feedback dot on the camera layer
        if (data.pressed)
        {
            int next_idx = ltdc_layer_bg_buffer_disp_idx ^ 1;
            LTDC_Layer_Config_TypeDef tmp = LTDC_Layer1Config;
            tmp.fb = (void *)&ltdc_layer_bg_buffer[next_idx];
            LTDC_Layer_Draw_Circle(&tmp, data.x, data.y, 5, LTDC_LAYER_COLOR_BLUE);
        }
    }
}


static PalmNetworkOutput_TypeDef palm_output;
static PalmDetection_TypeDef palm_detection;
static PalmDetectionFilter_TypeDef palm_filter;
static HandROI_TypeDef landmark_roi;

static LandmarkNetworkOutput_TypeDef landmark_output;
static uint8_t landmark_preprocessed_input[LANDMARK_INPUT_SIZE] __attribute__((aligned(32)));
static LandmarkPoint_TypeDef landmark_points[LANDMARK_POINT_COUNT];
void vAIPipelineTask(void){
	if (nn_frame_ready == 0U) {
	        return;
	    }

	    nn_frame_ready = 0U;

	    const uint8_t completed_idx = nn_completed_buffer_idx;
	    const uint8_t camera_buffer_idx = (uint8_t)ltdc_layer_bg_buffer_disp_idx;

	    uint8_t *palm_input = PALM_GetInputBuffer();

	    if (palm_input == NULL) {
	        return;
	    }

	    memcpy(palm_input, (const void *)ltdc_layer_nn_raw_buffer[completed_idx], PALM_INPUT_SIZE);

	    //NVIC_DisableIRQ(TIM7_IRQn);
	    const bool palm_inference_ok = PALM_Run(&palm_output);
	    //NVIC_EnableIRQ(TIM7_IRQn);

	    if (!palm_inference_ok) {
	        return;
	    }

	    if (!PALM_FindBestDetection(&palm_output, &palm_detection)) {
	        return;
	    }

	    const uint32_t probability_permille = (uint32_t)(palm_detection.probability * 1000.0f);
	    PALM_UpdateDetectionFilter(&palm_filter,probability_permille);

	    if (!palm_filter.detected) {
	    	LTDC_Layer_Draw_ROIClearPrevious();
	    	LTDC_Layer_Draw_LandmarksClearPrevious();
	    	return;
	    }

	    if(!PALM_CreateLandmarkROI(&palm_detection, LTDC_Layer1Config.width, LTDC_Layer1Config.height, &landmark_roi)){
	    	LTDC_Layer_Draw_ROIClearPrevious();
	    	LTDC_Layer_Draw_LandmarksClearPrevious();
	    	return;
	    }

    	LTDC_Layer_Draw_ROIClearPrevious();
    	LTDC_Layer_Draw_ROILandmark(&landmark_roi, LTDC_LAYER_COLOR_GREEN);

	    const bool preprocessing_ok = LANDMARK_PreprocessROI((const uint8_t *)ltdc_layer_bg_buffer[camera_buffer_idx],
	    													  LTDC_Layer1Config.width, LTDC_Layer1Config.height,
															  LTDC_Layer1Config.buf_width * LANDMARK_INPUT_CHANNELS,
															  &landmark_roi, landmark_preprocessed_input);

	    if (!preprocessing_ok) {
	        DEBUG_PRINTF("Landmark preprocessing failed\r\n");
	        return;
	    }

	    /*LTDC_BlitRGB888ToARGB4444(
	        &LTDC_Layer2Config,
	        landmark_preprocessed_input,
	        LANDMARK_INPUT_WIDTH,
	        LANDMARK_INPUT_HEIGHT,
	        0U,
	        200U
	    );
	    */

	    //NVIC_DisableIRQ(TIM7_IRQn);
	    const bool landmark_inference_ok = LANDMARK_Run(landmark_preprocessed_input, &landmark_output);
	    //NVIC_EnableIRQ(TIM7_IRQn);

	    if (!landmark_inference_ok) {
	        DEBUG_PRINTF("Landmark inference failed\r\n");
	        return;
	    }

	    LTDC_Layer_Draw_LandmarksClearPrevious();
	    if (landmark_output.presence >= 0.5f) {
	        LANDMARK_MapToFrame(
	            &landmark_output,
	            &landmark_roi,
	            landmark_points
	        );
	        LTDC_Layer_Draw_Landmarks(landmark_points);
	    }
}
