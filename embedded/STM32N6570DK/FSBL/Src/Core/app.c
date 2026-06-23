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
#include "image_bitmap.h"
#include "simple_rcc.h"
#include "simple_ae.h"
#include "tasks.h"

extern uint32_t g_pfnVectors[];
static volatile int ltdc_fg_disp_idx = 1;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
    if (pipe == DCMIPP_PIPE1) {
    	AE_OnFrameStats();
        ltdc_bg_buffer_disp_idx ^= 1;
        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_bg_buffer[ltdc_bg_buffer_disp_idx];

        LTDC_UpdateLayerAddress(&LTDC_Layer1Config);

    } else if (pipe == DCMIPP_PIPE2) {
        ltdc_fg_disp_idx ^= 1;
        LTDC_Layer2Config.fb = ltdc_fg_buffer[ltdc_fg_disp_idx];
        LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
    }
}

void app_init(){
	SCB->VTOR = (uint32_t)g_pfnVectors;
	RIFSC_Config();
	RCC_config_PWR();
	RCC_BoardClock_Config();
	debug_init(dbg_cfg);
	delay_init();
	DEBUG_PRINTF("Lets debug!\r\n");

	GPIO_Config(GPIOG, LED2_PIN, GPIO_default_cfg);

    XSPI_PSRAM_init(XSPI_psram_cfg);
    XSPI_NOR_init(XSPI_nor_cfg);

    LTDC_Init();

    TIMER_Delay_ms(10);

    LTDC_ConfigLayer1();
    LTDC_ConfigLayer2();

    LTDC_FillLayer(&LTDC_Layer1Config, LTDC_COLOR_WHITE);
    LTDC_FillLayer(&LTDC_Layer2Config, LTDC_COLOR_WHITE);

    uint32_t error = 0;
    if(CAM_Init(&h_cam) == CAM_OK) {
    	if(CAM_DisplayPipe_Start(&h_cam) != CAM_OK) {
    		error++;
    	}
//    	if(CAM_NNPipe_Start(&h_cam) != CAM_OK) {
//    		error++;
//    	}
    }

    /* --- Scheduler --- */
    SCHEDULER_System_init();

	uint8_t task_idx;

	SCHEDULER_Task_add(vSystemTimeTask, "Display Systemtime", 3, 1, &task_idx);
	SCHEDULER_Task_add(vLEDTask, "LED", 5000, 2, &task_idx);
	SCHEDULER_Task_add(vBackgroundTask, "BgColor", 20, 3, &task_idx);
	SCHEDULER_Task_add(vAETask, "AETask", 10, 4, &task_idx);
//	SCHEDULER_Task_add(vRecursionTestTask, "Test", 10, 10, &task_idx);
}

void app_run(){
	SCHEDULER_Tasks_run();
}


