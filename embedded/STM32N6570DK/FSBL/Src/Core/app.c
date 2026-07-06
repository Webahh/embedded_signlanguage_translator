/*
 * app.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "app.h"
#include "config.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"

#include "simple_ltdc.h"
#include "simple_ltdc_color.h"
#include "simple_ltdc_layer.h"
#include "simple_ltdc_layer_draw.h"

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
#include "ui_callback.h"

extern uint32_t g_pfnVectors[];
UI_Drawer_TypeDef _drawer;
static const char *_mode_labels[] = {"Palm", "Hand", "Sign"};

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

    LTDC_Layer_Layer1_Config();
    LTDC_Layer_Layer2_Config();

    LTDC_Layer_Draw_Fill(&LTDC_Layer1Config, LTDC_LAYER_COLOR_WHITE);

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
	UI_Drawer_Init(&_drawer, (uint8_t *)ltdc_layer_fg_buffer[1], (uint8_t *)ltdc_layer_fg_buffer[0]);

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

	SCHEDULER_Task_add(vTouchTask, "Touch", 20, 2, &task_idx);
	SCHEDULER_Task_add(vLEDTask, "LED", 5000, 1, &task_idx);
	SCHEDULER_Task_add(vAETask, "AETask", 50, 2, &task_idx);
//	SCHEDULER_Task_add(vRecursionTestTask, "Test", 10, 10, &task_idx);
	SCHEDULER_Task_add(vAIPipelineTask,"AIPipeline", 10, 1, &task_idx);
}

void app_run(){
	SCHEDULER_Tasks_run();
}


