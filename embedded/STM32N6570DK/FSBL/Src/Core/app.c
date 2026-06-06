/*
 * app.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include "app.h"
#include "config.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_xspi.h"
#include "simple_rifsc.h"
#include "simple_camera.h"
#include "image_bitmap.h"
#include "simple_rcc.h"
#include "tasks.h"

void app_init(){
	Security_Config();
	RCC_SystemClock_Config();
	delay_init();

	GPIO_Config(GPIOG, LED2_PIN, GPIO_default_cfg);

    PSRAM_Init();
    NOR_Init();

    LCD_Init();

    delay_ms(10);

    LCD_ConfigLayer1();

    delay_ms(10);

    uint32_t error = 0;
    if (CAM_Init(&h_cam, 0) == CAM_OK) {
        if(CAM_DisplayPipe_Start(&h_cam)) {
        	// Error
        	error++;
        }
//        CAM_NNPipe_Start(&h_cam);
    }

    /* --- Scheduler --- */
    SCHEDULER_Init();

	SCHEDULER_AddTask(vLEDTask, "LED", 500);
	SCHEDULER_AddTask(vBackgroundTask, "BgColor", 20);
}

void app_run(){
	SCHEDULER_Run();
}


