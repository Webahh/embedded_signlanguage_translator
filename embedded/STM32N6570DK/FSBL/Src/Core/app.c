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
#include "simple_xspi.h"
#include "simple_rifsc.h"
#include "simple_camera.h"
#include "simple_dcmipp.h"
#include "image_bitmap.h"
#include "simple_rcc.h"
#include "simple_ae.h"
#include "tasks.h"

static volatile int lcd_fg_capt_idx = 0;
static volatile int lcd_fg_disp_idx = 1;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe){
    if (pipe == DCMIPP_PIPE1) {
    	AE_OnFrameStats();
        lcd_bg_buffer_disp_idx ^= 1;
        LCD_Layer1Config.fb = (volatile uint8_t *)&lcd_bg_buffer[lcd_bg_buffer_disp_idx];

        LCD_UpdateLayerAddress(&LCD_Layer1Config);

    } else if (pipe == DCMIPP_PIPE2) {
        int next_capt = (lcd_fg_capt_idx + 1) % NN_BUFFER_NB;
        int next_disp = (lcd_fg_disp_idx + 1) % NN_BUFFER_NB;

        DCMIPP_Pipe_UpdateBufAddr(DCMIPP_PIPE2,
            (uint32_t)&lcd_fg_buffer[next_capt]);

        LCD_Layer2Config.fb = lcd_fg_buffer[next_disp];
        LCD_UpdateLayerAddress(&LCD_Layer2Config);

        lcd_fg_capt_idx = next_capt;
        lcd_fg_disp_idx = next_disp;
    }
}

void app_init(){
	Security_Config();
	RCC_BoardClock_Config();
	delay_init();

	GPIO_Config(GPIOG, LED2_PIN, GPIO_default_cfg);

    PSRAM_Init();
    NOR_Init();

    LCD_Init();

    delay_ms(10);

    LCD_ConfigLayer1();
//    LCD_ConfigLayer2();

    delay_ms(10);

    uint32_t error = 0;
    if (CAM_Init(&h_cam) == CAM_OK) {
        if(CAM_DisplayPipe_Start(&h_cam)) {
        	// Error
        	error++;
        }
//        if(CAM_NNPipe_Start(&h_cam)) {
//            // Error
//        	error++;
//        }
    }
    /* --- Scheduler --- */
    SCHEDULER_Init();

	SCHEDULER_AddTask(vLEDTask, "LED", 500);
	//SCHEDULER_AddTask(vBackgroundTask, "BgColor", 20);
	SCHEDULER_AddTask(vAETask, "AETask" , 30);

}

void app_run(){
	SCHEDULER_Run();
}


