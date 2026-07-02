/*
 * app.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <string.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "app.h"
#include "config.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_text.h"
#include "simple_xspi.h"
#include "simple_rifsc.h"
#include "simple_camera.h"
#include "simple_dcmipp.h"
#include "simple_ai.h"
#include "image_bitmap.h"
#include "simple_rcc.h"
#include "simple_ae.h"
#include "simple_touch.h"
#include "tasks.h"
#include "simple_i2c.h"
#include "pd_anchors.h"
#include "palm_detection.h"
#include "fingeralphabet.h"
#include "palm_postprocessing.h"
#include "hand_landmark.h"
#include "hand_landmark_preprocessing.h"
#include "hand_landmark_postprocessing.h"
#include "ui.h"

extern uint32_t g_pfnVectors[];
static volatile int ltdc_fg_disp_idx = 1;
static volatile uint8_t nn_frame_ready = 0U;
static volatile uint8_t nn_completed_buffer_idx = 0U;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
    if (pipe == DCMIPP_PIPE1) {
    	AE_OnFrameStats();
        ltdc_bg_buffer_disp_idx ^= 1;
        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_bg_buffer[ltdc_bg_buffer_disp_idx];
        LTDC_UpdateLayerAddress(&LTDC_Layer1Config);
    } else if (pipe == DCMIPP_PIPE2) {
        nn_completed_buffer_idx = (DCMIPP->P2SR & DCMIPP_P2SR_LSTFRM) ? 1U : 0U;
        nn_frame_ready = 1U;
    }
}

static PalmNetworkOutput_TypeDef palm_output;
static PalmDetection_TypeDef palm_detection;
static PalmDetectionFilter_TypeDef palm_filter;
static HandROI_TypeDef landmark_roi;

static LandmarkNetworkOutput_TypeDef landmark_output;
static uint8_t landmark_preprocessed_input[LANDMARK_INPUT_SIZE] __attribute__((aligned(32)));
static LandmarkPoint_TypeDef landmark_points[LANDMARK_POINT_COUNT];

static void vPalmTask(void)
{
    if (nn_frame_ready == 0U) {
        return;
    }

    nn_frame_ready = 0U;

    const uint8_t completed_idx = nn_completed_buffer_idx;
    const uint8_t camera_buffer_idx = (uint8_t)ltdc_bg_buffer_disp_idx;

    uint8_t *palm_input = PALM_GetInputBuffer();

    if (palm_input == NULL) {
        return;
    }

    memcpy(palm_input, (const void *)ltdc_nn_raw_buffer[completed_idx], PALM_INPUT_SIZE);

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
    	ClearPreviousROI();
    	ClearPreviousLandmarks();
    	return;
    }

    if(!PALM_CreateLandmarkROI(&palm_detection, &landmark_roi)){
    	ClearPreviousROI();
    	ClearPreviousLandmarks();
        return;
    }

    ClearPreviousROI();
    DrawLandmarkROI(&landmark_roi, LTDC_COLOR_GREEN);

    const bool preprocessing_ok = LANDMARK_PreprocessROI((const uint8_t *)ltdc_bg_buffer[camera_buffer_idx],
    													  LTDC_Layer1Config.width, LTDC_Layer1Config.height,
														  LTDC_Layer1Config.buf_width * LANDMARK_INPUT_CHANNELS,
														  &landmark_roi, landmark_preprocessed_input);

    if (!preprocessing_ok) {
        DEBUG_PRINTF("Landmark preprocessing failed\r\n");
        return;
    }
/*
    LTDC_BlitRGB888ToARGB4444(
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

    ClearPreviousLandmarks();
    if (landmark_output.presence >= 0.5f) {
        LANDMARK_MapToFrame(
            &landmark_output,
            &landmark_roi,
            landmark_points
        );
        DrawLandmarks(landmark_points);
    }
}



UI_Drawer_TypeDef _drawer;

static const char *_mode_labels[] = {"Palm", "Hand", "Sign"};

static void _dr_cb_SystemMode(uint8_t idx, uint8_t val, void *ctx) {
	(void)idx;
	(void)ctx;
	DEBUG_PRINTF("[UI] Mode: %u\r\n", val);
}

static void _dr_cb_composite(uint8_t idx, uint8_t val, void *ctx) {
	(void)val;
	UI_Drawer_TypeDef *drawer = (UI_Drawer_TypeDef *)ctx;
	DEBUG_PRINTF("[UI] %s: visible=%u slider=%u\r\n",
		drawer->items[idx].label, drawer->items[idx].composite.visible,
		drawer->items[idx].composite.slider_value);
}

static void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx) {
	static uint8_t systemtime_id;
	(void)idx;
	(void)ctx;
	if (val) {
		SCHEDULER_Task_add(vSystemTimeTask, "Display Systemtime", 3, 1, &systemtime_id);
	} else {
		SCHEDULER_Task_remove(systemtime_id);
		LTDC_LayerDrawRect(&LTDC_Layer2Config, 720, 0, 80, 16, 0x00000000);
	}
}

static void _dr_cb_toggle_SystemInfo(uint8_t idx, uint8_t val, void *ctx) {
	(void)idx;
	(void)ctx;
	if(val){

	} else {

	}

	DEBUG_PRINTF("[UI] SystemInfo: %u\r\n", val);
}

void app_init(){
	SCB->VTOR = (uint32_t)g_pfnVectors;
	RIFSC_Config();
	RCC_config_PWR();
	RCC_config_SleepModeLPEN();
	RCC_BoardClock_Config();
	debug_init(dbg_cfg);
	TIMER_Delay_init();
	DEBUG_PRINTF("Lets debug!\r\n");

	GPIO_Config(GPIOG, LED2_PIN, GPIO_default_cfg);

	XSPI_Status_TypeDef xspi_status = XSPI_ERROR;
	xspi_status = XSPI_PSRAM_init(XSPI_psram_cfg);
	DEBUG_PRINTF("PSRAM INIT Status: %d\r\n", xspi_status);
    xspi_status = XSPI_NOR_init(XSPI_nor_cfg);
    DEBUG_PRINTF("NOR INIT Status: %d\r\n", xspi_status);

    LTDC_Init();

    TIMER_Delay_ms(10);

    LTDC_ConfigLayer1();
    LTDC_ConfigLayer2();

    LTDC_LayerFill(&LTDC_Layer1Config, LTDC_COLOR_WHITE);

    uint32_t error = 0;
    if(CAM_Init(&h_cam) == CAM_OK) {
    	if(CAM_DisplayPipe_Start(&h_cam) != CAM_OK) {
    		error++;
    	}
    	if(CAM_NNPipe_Start(&h_cam) != CAM_OK) {
    		error++;
    	}
    }

    /* --- Touch --- */
    TOUCH_ConfigIO();
    TIMER_Delay_ms(50);

    static TOUCH_Handle_TypeDef h_touch;

    if (TOUCH_Probe(&h_touch, TS_I2C) == TOUCH_OK) {
        TOUCH_Init(&h_touch);
    } else {
        DEBUG_PRINTF("Touch: no controller found\r\n");
    }

    /* --- UI --- */
	UI_Drawer_Init(&_drawer, (uint8_t *)ltdc_fg_buffer[1], (uint8_t *)ltdc_fg_buffer[0]);

	// Mode: 3-segment selector (Palm / Hand / Sign)
	int mode_idx;
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_SELECTOR, "Mode", _dr_cb_SystemMode, NULL, &mode_idx);
	_drawer.items[mode_idx].seg_labels = _mode_labels;
	_drawer.items[mode_idx].seg_count = 3;

	// Composite items: visibility eye + slider per gesture class
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Palm", _dr_cb_composite, &_drawer, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Hand", _dr_cb_composite, &_drawer, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Sign", _dr_cb_composite, &_drawer, NULL);

	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_TOGGLE, "System Time", _dr_cb_toggle_SystemTime, NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_TOGGLE, "System Info", _dr_cb_toggle_SystemInfo, NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_LABEL, "v1.0.0", NULL, NULL, NULL);

	// Pre-render both open/closed buffers
	UI_Drawer_Prepare(&_drawer, &LTDC_Layer2Config);

    /* --- AI --- */
	AI_Status_TypeDef status = AI_Init();

	if (status != AI_STATUS_OK) {
		while (1) {
		}
	}

	/* --- Scheduler --- */
	SCHEDULER_System_init();

	uint8_t task_idx;

	SCHEDULER_Task_add(vTouchTask, "Touch", 1, 2, &task_idx);
	SCHEDULER_Task_add(vLEDTask, "LED", 5000, 2, &task_idx);
	SCHEDULER_Task_add(vBackgroundTask, "BgColor", 20, 3, &task_idx);
	SCHEDULER_Task_add(vAETask, "AETask", 10, 4, &task_idx);
//	SCHEDULER_Task_add(vRecursionTestTask, "Test", 10, 10, &task_idx);
	SCHEDULER_Task_add(vPalmTask,"Palm", 30, 1, &task_idx);
}

void app_run(){
	SCHEDULER_Tasks_run();
}


