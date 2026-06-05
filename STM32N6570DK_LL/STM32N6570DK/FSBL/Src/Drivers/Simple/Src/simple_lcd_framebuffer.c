/*
 * simple_lcd_framebuffer.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_lcd_framebuffer.h"

volatile uint32_t lcd_bg_buffer[LCD_BG_WIDTH * LCD_BG_HEIGHT] __attribute__((section(".psram_bss"), aligned(32)));
volatile uint32_t lcd_fg_buffer[LCD_FG_WIDTH * LCD_FG_HEIGHT] __attribute__((section(".psram_bss"), aligned(32)));



