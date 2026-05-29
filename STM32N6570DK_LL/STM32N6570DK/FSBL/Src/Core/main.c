#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_lcd_framebuffer.h"
#include "simple_rcc.h"
#include "simple_xspi.h"

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

#define RIF_MASTER_INDEX_LTDC1      10U
#define RIF_MASTER_INDEX_LTDC2      11U
#define RIF_RISC_REG_LTDC_GROUP     3U

#define HAL_RIMC_ATTR_LTDC_VALUE    0x00000310U
#define HAL_RISC_LTDC_GROUP_VALUE   0x000005FCU

static void RIF_Config_LTDC_BareMetal(void){
    RCC->AHB3ENSR |= RCC_AHB3ENSR_RIFSCENS;
    (void)RCC->AHB3ENSR;

    RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_LTDC1] = HAL_RIMC_ATTR_LTDC_VALUE;
    RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_LTDC2] = HAL_RIMC_ATTR_LTDC_VALUE;

    RIFSC->RISC_SECCFGRx[RIF_RISC_REG_LTDC_GROUP]  = HAL_RISC_LTDC_GROUP_VALUE;
    RIFSC->RISC_PRIVCFGRx[RIF_RISC_REG_LTDC_GROUP] = HAL_RISC_LTDC_GROUP_VALUE;

    (void)RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_LTDC1];
    (void)RIFSC->RIMC_ATTRx[RIF_MASTER_INDEX_LTDC2];
    (void)RIFSC->RISC_SECCFGRx[RIF_RISC_REG_LTDC_GROUP];
    (void)RIFSC->RISC_PRIVCFGRx[RIF_RISC_REG_LTDC_GROUP];
}

int main(void){
	RIF_Config_LTDC_BareMetal();

	delay_init();

	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);

    PSRAM_Init();

    LCD_Init();

    delay_ms(10);

    LCD_ConfigLayer1();
    LCD_ConfigLayer2();

    LCD_Fill(LCD_COLOR_GREEN);

    for (uint32_t i = 0; i < LCD_FG_WIDTH * LCD_FG_HEIGHT; i++) {
        lcd_fg_buffer[i] = LCD_COLOR_BLUE;
    }

    uintptr_t bg_addr  = (uintptr_t)lcd_framebuffer;
    uintptr_t bg_start = bg_addr & ~31U;
    uintptr_t bg_size  = (LCD_WIDTH * LCD_HEIGHT * LCD_BYTES_PER_PIXEL + 31U) & ~31U;
    SCB_CleanDCache_by_Addr((uint32_t*)bg_start, bg_size);

    uintptr_t fg_addr  = (uintptr_t)lcd_fg_buffer;
    uintptr_t fg_start = fg_addr & ~31U;
    uintptr_t fg_size  = (LCD_FG_WIDTH * LCD_FG_HEIGHT * LCD_BYTES_PER_PIXEL + 31U) & ~31U;
    SCB_CleanDCache_by_Addr((uint32_t*)fg_start, fg_size);

    delay_ms(10);


    SCHEDULER_Init();

	SCHEDULER_AddTask(vLEDTask, "LED", 500);
	SCHEDULER_AddTask(vBackgroundTask, "BgColor", 20);

	while (1) {
		SCHEDULER_Run();
	}
}

// cant build without
__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
