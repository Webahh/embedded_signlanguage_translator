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
#include "tasks.h"

static int is_cache_enabled(void)
{
    return (SCB->CCR & SCB_CCR_DC_Msk) != 0;
}

#define CACHE_OP(__op__) do { \
    if (is_cache_enabled()) { \
        __op__; \
    } \
} while (0)

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1) {
        int next_capt = (lcd_bg_buffer_capt_idx + 1) % DISPLAY_BUFFER_NB;
        int next_disp = (lcd_bg_buffer_disp_idx + 1) % DISPLAY_BUFFER_NB;

        CACHE_OP(SCB_InvalidateDCache_by_Addr(
            (void *)&lcd_bg_buffer[lcd_bg_buffer_capt_idx],
            sizeof(lcd_bg_buffer[0])));

        DCMIPP_Pipe_UpdateBufAddr(DCMIPP_PIPE1,
            (uint32_t)&lcd_bg_buffer[next_capt]);

        LCD_Layer1Config.fb = (volatile uint8_t *)&lcd_bg_buffer[next_disp];
        LCD_UpdateLayerAddress(&LCD_Layer1Config);

        lcd_bg_buffer_capt_idx = next_capt;
        lcd_bg_buffer_disp_idx = next_disp;
    }
}

void app_init(){
	Security_Config();
	RCC_SystemClock_Config();
	{
	    static const RCC_PLL_ConfigTypeDef pll_config[4] = {
	        { .CFGR1 = 0x201900, .CFGR2 = 0x0,          .CFGR3 = 0x49000005 },
	        { .CFGR1 = 0x807D00, .CFGR2 = 0x0,          .CFGR3 = 0x49000005 },
	        { .CFGR1 = 0x80E100, .CFGR2 = 0x0,          .CFGR3 = 0x4A000005 },
	        { .CFGR1 = 0x80E100, .CFGR2 = 0x0,          .CFGR3 = 0x76000005 },
	    };
	RCC_config_PLLs(pll_config);
	}
	{
	    static const RCC_IC_ConfigTypeDef ic_config[20] = {
	    	[14] = { .CFGR = 0x20000000 },	/* IC15: ?, div 0 */
	        [15] = { .CFGR = 0x30010000 },  /* IC16: PLL4, div 2 */
	        [16] = { .CFGR = 0x10020000 },  /* IC17: PLL1, div 6 */
	        [17] = { .CFGR = 0x00270000 },  /* IC18: PLL1, div 40 */
	    };
	    RCC_config_ICs(ic_config);
	}
	delay_init();

	GPIO_Config(GPIOG, LED2_PIN, GPIO_default_cfg);

    PSRAM_Init();
    NOR_Init();

    LCD_Init();

    delay_ms(10);

    CACHE_OP(SCB_CleanInvalidateDCache_by_Addr(
        (void *)lcd_bg_buffer, sizeof(lcd_bg_buffer)));

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


