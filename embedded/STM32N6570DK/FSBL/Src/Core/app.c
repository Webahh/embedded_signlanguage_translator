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
#include "palm_detection_logic.h"

extern uint32_t g_pfnVectors[];
static volatile int ltdc_fg_disp_idx = 1;
static volatile uint8_t nn_frame_ready = 0U;
static volatile uint8_t nn_completed_buffer_idx = 0U;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
   	//AE_OnFrameStats();

    if (pipe == DCMIPP_PIPE1) {
        ltdc_bg_buffer_disp_idx ^= 1;
        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_bg_buffer[ltdc_bg_buffer_disp_idx];

        LTDC_UpdateLayerAddress(&LTDC_Layer1Config);

    } else if (pipe == DCMIPP_PIPE2) {
        nn_completed_buffer_idx = (uint8_t)ltdc_fg_disp_idx;
        ltdc_fg_disp_idx ^= 1;

        nn_frame_ready = 1U;
    }
}

static AI_PalmOutput_TypeDef palm_output;
static PalmDetection_TypeDef palm_detection;
static PalmDetectionFilter_TypeDef palm_filter;

static void vPalmTask(void)
{
    if (nn_frame_ready == 0U) {
        return;
    }

    nn_frame_ready = 0U;

    const uint8_t completed_idx = nn_completed_buffer_idx;

    uint8_t *palm_input = AI_GetPalmInputBuffer();

    if (palm_input == NULL) {
        return;
    }

    memcpy(palm_input, (const void *)ltdc_fg_buffer[completed_idx], PALM_INPUT_ELEMENT_COUNT);

    NVIC_DisableIRQ(TIM7_IRQn);

    const bool inference_ok = AI_RunPalm(&palm_output);

    NVIC_EnableIRQ(TIM7_IRQn);

    if (!inference_ok) {
        return;
    }

    if (!PALM_FindBestDetection(&palm_output, &palm_detection)) {
        return;
    }

    const uint32_t probability_permille = (uint32_t)(palm_detection.probability * 1000.0f);

    PALM_UpdateDetectionFilter(&palm_filter,probability_permille);

    DEBUG_PRINTF(
        "Palm p=%u idx=%u detected=%u\r\n",
        probability_permille,
        palm_detection.anchor_index,
        (uint32_t)palm_filter.detected
    );
}

void app_init(){
	SCB->VTOR = (uint32_t)g_pfnVectors;
	RIFSC_Config();
	RCC_config_PWR();
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
//    LTDC_ConfigLayer2();

    LTDC_LayerFill(&LTDC_Layer1Config, LTDC_COLOR_WHITE);
//    LTDC_LayerFill(&LTDC_Layer2Config, LTDC_COLOR_WHITE);

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

    uint8_t found_addrs[128] = {0};
    uint32_t found = 0;
    I2C_Scan(TS_I2C, found_addrs, &found);

    static TOUCH_Handle_TypeDef h_touch;

    uint8_t id[4];

    I2C_Mem_read(I2C2, 0x5d, 0x8140, id, 4);

    if (TOUCH_Probe(&h_touch, TS_I2C) == TOUCH_OK) {
        TOUCH_Init(&h_touch);
    } else {
        DEBUG_PRINTF("Touch: no controller found\r\n");
    }

    if (!AI_SelfTest()) {
        DEBUG_PRINTF("AI self-test failed\r\n");
        while (1) {
        }
    }
    DEBUG_PRINTF("AI self-test successful\r\n");


    /* --- Scheduler --- */
    SCHEDULER_System_init();

	uint8_t task_idx;

	SCHEDULER_Task_add(vTouchTask, "Touch", 1, 1, &task_idx);
	//SCHEDULER_Task_add(vSystemTimeTask, "Display Systemtime", 3, 1, &task_idx);
	SCHEDULER_Task_add(vLEDTask, "LED", 5000, 2, &task_idx);
	SCHEDULER_Task_add(vBackgroundTask, "BgColor", 20, 3, &task_idx);
	SCHEDULER_Task_add(vAETask, "AETask", 10, 4, &task_idx);
//	SCHEDULER_Task_add(vRecursionTestTask, "Test", 10, 10, &task_idx);
	SCHEDULER_Task_add(vPalmTask,"Palm", 250, 4, &task_idx);
}

void app_run(){
	SCHEDULER_Tasks_run();
}


