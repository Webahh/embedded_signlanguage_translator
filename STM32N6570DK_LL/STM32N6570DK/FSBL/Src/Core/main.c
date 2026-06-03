#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_lcd_framebuffer.h"
#include "simple_rcc.h"
#include "simple_xspi.h"
#include "simple_rifsc.h"
#include "simple_camera.h"
#include "simple_i2c.h"

#define LED2_PIN 10

#define BG_NUM_COLORS 3

static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;

static CAM_Handle h_cam;

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

    LCD_FillLayer(&LCD_Layer1Config, 0b0000011111100000);
    LCD_ConfigLayer1();

    delay_ms(10);

    /* --- Camera power-up sequence --- */
    /* CAM_PWR_EN: PD2 = high to enable camera power */
    GPIO_Config(GPIOD, 2, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_BSRR_set(GPIOD, 2);

    delay_ms(1);

    /* CAM_NRST: PC8 pulse low then high to release reset */
    GPIO_Config(GPIOC, 8, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_BSRR_reset(GPIOC, 8);
    delay_ms(1);
    GPIO_BSRR_set(GPIOC, 8);
    delay_ms(10);

    /* --- Camera (IMX335) initialization --- */
    /* I2C1: PH9=SCL, PC1=SDA (AF4, open-drain, pull-up) */
    GPIO_Config(GPIOH, 9, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_PUPD_UP, GPIO_I2C);
    GPIO_Config(GPIOC, 1, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_PUPD_UP, GPIO_I2C);
    /* I2C1 @ 400 kHz (PCLK1=64MHz, PRESC=0, SCLL=83, SCLH=75, SDADEL=0, SCLDEL=11) */
    I2C_Config(I2C1, 0, 0x00B04B53);

    if (CAM_Init(&h_cam, I2C1, 0) == CAM_OK) {
        /* Reconfigure LTDC layer1 to RGB565 to match camera output */
        LCD_Layer1Config.pixel_format  = LCD_PF_RGB565;
        LCD_Layer1Config.buf_width     = CAM_DISPLAY_WIDTH;
        LCD_Layer1Config.width         = CAM_DISPLAY_WIDTH;
        LCD_Layer1Config.height        = CAM_DISPLAY_HEIGHT;
        LCD_Layer1Config.const_alpha   = 0xFF;
        LCD_Layer1Config.per_pixel_alpha = 0;
        LCD_Layer1Config.default_color = 0;
        LCD_Layer1Config.blendingOrder = 0;
        LCD_Layer1Config.fb            = (volatile uint32_t *)h_cam.display_buf0;
        LCD_ConfigLayer1();

        CAM_DisplayPipe_Start(&h_cam);
    }

    /* --- Scheduler --- */
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
