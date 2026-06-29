/*
 * tasks.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <stdio.h>

#include "tasks.h"
#include "app.h"
#include "simple_gpio.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_ae.h"
#include "config.h"
#include "simple_timer.h"
#include "simple_text.h"
#include "simple_touch.h"
#include "ui.h"

#define LED2_PIN 10
#define BG_NUM_COLORS 3

static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;

__attribute__((noinline, optimize("O0"))) // No optimizations for better testing
static int recursion(int n)
{
    volatile uint32_t marker = 0xDEADBEEF;
    volatile uint32_t padding[8];

    padding[0] = marker;

    if (n == 0)
        return padding[0];

    return recursion(n - 1) + 1;
}

void vRecursionTestTask(void) {
	recursion(20);
}

void vSystemTimeTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now); // MAX:     4294967296
	char str[11]; // + '\0'
	snprintf(str, sizeof(str), "%lu", (unsigned long)now);
	TEXT_StringBg_draw(&LTDC_Layer1Config, str, 10, 10, LTDC_COLOR_GREEN, LTDC_COLOR_WHITE);
}

void vLEDTask(void) {
	GPIO_BSRR_toggle(GPIOG, LED2_PIN);
}

void vBackgroundTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now);
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

    LTDC_SetBackgroundColor(r, g, b);
}

void vAETask(void){
	AE_Process(&h_cam);
}

void vTouchTask(void){
	uint8_t pending = 0;
	TOUCH_GetPending(&pending);
    if (pending)
    {
        TOUCH_Data_TypeDef data;
        TOUCH_GetState(NULL, &data);

        int ui = UI_HandleTouch(data.x, data.y, data.pressed);
        UI_Drawer_HandleTouch(&_drawer, data.x, data.y, data.pressed, &LTDC_Layer2Config);
        if (ui != -1)
            UI_DrawAll(&LTDC_Layer2Config);

        if (data.pressed)
        {
            int next_idx = ltdc_bg_buffer_disp_idx ^ 1;
            LTDC_LayerConfig_TypeDef tmp = LTDC_Layer1Config;
            tmp.fb = (void *)&ltdc_bg_buffer[next_idx];
            LTDC_LayerDrawCricle(&tmp, data.x, data.y, 5, LTDC_COLOR_BLUE);
        }
    }
}
