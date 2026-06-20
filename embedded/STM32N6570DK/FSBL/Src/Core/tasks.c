/*
 * tasks.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <stdio.h>

#include "tasks.h"
#include "simple_gpio.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_ae.h"
#include "config.h"
#include "simple_timer.h"
#include "simple_text.h"

#define LED2_PIN 10
#define BG_NUM_COLORS 3

static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;


void vTestTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now); // MAX:     4294967296
	char str[10];
	sprintf(str, "%d", now);
	LCD_DrawStringBG(&LCD_Layer1Config, str, 100, 100, LCD_COLOR_GREEN, LCD_COLOR_WHITE);
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

    LCD_SetBackgroundColor(r, g, b);
}

void vAETask(void){
	AE_Process(&h_cam);
}
