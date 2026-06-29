/*
 * app.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <string.h>
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
#include "ui.h"

extern uint32_t g_pfnVectors[];
static volatile int ltdc_fg_disp_idx = 1;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
   	AE_OnFrameStats();

    if (pipe == DCMIPP_PIPE1) {
        ltdc_bg_buffer_disp_idx ^= 1;
        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_bg_buffer[ltdc_bg_buffer_disp_idx];

        LTDC_UpdateLayerAddress(&LTDC_Layer1Config);
    }
}

volatile uint32_t ai_init_before = 0;
volatile uint32_t ai_init_after = 0;
static uint8_t landmark_test_input[LANDMARK_INPUT_SIZE];
static AI_LandmarkOutput_TypeDef landmark_test_output;

UI_Drawer_TypeDef _drawer;

static void _dr_cb_toggle(uint8_t idx, uint8_t val, void *ctx)
{
    (void)idx;
    (void)ctx;
    if (val)
        GPIO_BSRR_set(GPIOG, LED2_PIN);
    else
        GPIO_BSRR_reset(GPIOG, LED2_PIN);
}

static void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx) {
	static uint8_t systemtime_id;
	(void)idx;
	(void)ctx;
	if (val) {
		// Add task
		SCHEDULER_Task_add(vSystemTimeTask, "Display Systemtime", 3, 1, &systemtime_id);
	} else {
		// Remove Task and clean layer
		SCHEDULER_Task_remove(systemtime_id);
		LTDC_LayerDrawRect(&LTDC_Layer2Config, 720, 0, 80, 16, 0x00000000);
	}
}


static void _dr_cb_slider(uint8_t idx, uint8_t val, void *ctx)
{
    (void)idx;
    (void)val;
    (void)ctx;
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

    volatile const uint8_t *weights =
        (volatile const uint8_t *)0x71000000UL;

    volatile uint8_t weight_header[16];

    for (uint32_t i = 0U; i < sizeof(weight_header); i++) {
        weight_header[i] = weights[i];
    }

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
//    	if(CAM_NNPipe_Start(&h_cam) != CAM_OK) {
//    		error++;
//    	}
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
	UI_Init();
	UI_Drawer_Init(&_drawer, (uint8_t *)ltdc_fg_buffer[1], (uint8_t *)ltdc_fg_buffer[0]);

	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_SELECTOR, "Mode", NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Palm", NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Hand", NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_COMPOSITE, "Sign", NULL, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_TOGGLE, "System Time", _dr_cb_toggle_SystemTime, NULL);
	UI_Drawer_AddItem(&_drawer, UI_DRAWER_ITEM_LABEL, "v1.0.0", NULL, NULL);

	UI_DrawAll(&LTDC_Layer2Config);
	UI_Drawer_Prepare(&_drawer, &LTDC_Layer2Config);

    /* --- AI --- */
    AI_Status_TypeDef status = AI_Init();

    if (status != AI_STATUS_OK) {
        while (1) {
        }
    }

    static const uint8_t ai_test_input_b[AI_INPUT_SIZE] = {
        128, 128, 134, 119, 135, 102, 130,  90, 125,  83, 136,
         87, 137,  69, 137,  58, 137,  48, 132,  86, 133,  67,
        133,  55, 134,  45, 128,  87, 128,  70, 128,  60, 129,
         50, 123,  91, 123,  78, 123,  70, 123,  63, 175, 235,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
    };

    static uint8_t ai_output_data[AI_OUTPUT_SIZE];

    static volatile uint32_t predicted_index;
    static volatile uint8_t predicted_score;
    static volatile const char *predicted_label;

    if (!AI_Run(ai_test_input_b, ai_output_data)) {
        while (1) {
        }
    }

    AI_Result_TypeDef result = AI_GetResult(ai_output_data);

    predicted_index = result.class_index;
    predicted_score = result.score;
    predicted_label = result.label;

    for (uint32_t i = 0U; i < LANDMARK_INPUT_SIZE; i++) {
        landmark_test_input[i] = (uint8_t)i;
    }

    bool landmark_copy_ok = AI_RunLandmark(landmark_test_input, &landmark_test_output);

    /* --- Scheduler --- */
    SCHEDULER_System_init();

	uint8_t task_idx;

	SCHEDULER_Task_add(vTouchTask, "Touch", 1, 1, &task_idx);
	SCHEDULER_Task_add(vLEDTask, "LED", 5000, 2, &task_idx);
	SCHEDULER_Task_add(vBackgroundTask, "BgColor", 20, 3, &task_idx);
	SCHEDULER_Task_add(vAETask, "AETask", 10, 4, &task_idx);
//	SCHEDULER_Task_add(vRecursionTestTask, "Test", 10, 10, &task_idx);
}

void app_run(){
	SCHEDULER_Tasks_run();
}


