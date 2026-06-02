#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_lcd_framebuffer.h"
#include "simple_rcc.h"
#include "simple_xspi.h"
#include "simple_rifsc.h"

#define LED2_PIN 10

#define BG_NUM_COLORS 3

static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;

static void vLEDTask(void) {
	GPIO_BSRR_toggle(GPIOG, LED2_PIN);
}

static void vBackgroundTask(void) {
    uint32_t now     = SCHEDULER_GetTick();
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

    LCD_SetBackgroundColor(r, g, b);
}

int main(void){
	Security_Config();

	delay_init();

	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);

//    SCB_EnableDCache();

    PSRAM_Init();
    NOR_Init();

    LCD_Init();

    delay_ms(10);

    LCD_FillLayer2Sides(&LCD_Layer1Config, LCD_COLOR_BLUE, LCD_COLOR_RED);
    LCD_ConfigLayer1();

    delay_ms(10);

    SCHEDULER_Init();

	SCHEDULER_AddTask(vLEDTask, "LED", 500);
	//SCHEDULER_AddTask(vBackgroundTask, "BgColor", 20);

	while (1) {
		SCHEDULER_Run();
	}
}

// cant build without
__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
