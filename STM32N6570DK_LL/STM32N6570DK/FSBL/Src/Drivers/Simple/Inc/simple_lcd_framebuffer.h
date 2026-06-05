/*
 * simple_lcd_framebuffer.h
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#ifndef SIMPLE_LCD_FRAMEBUFFER_H
#define SIMPLE_LCD_FRAMEBUFFER_H

#include <stdint.h>

#define LCD_BG_WIDTH  800
#define LCD_BG_HEIGHT 480

#define LCD_FG_WIDTH  10
#define LCD_FG_HEIGHT 10

#define LCD_BYTES_PER_PIXEL 4U

#define LCD_COLOR_BLACK  0xFF000000U
#define LCD_COLOR_WHITE  0xFFFFFFFFU
#define LCD_COLOR_RED    0xFFFF0000U
#define LCD_COLOR_GREEN  0xFF00FF00U
#define LCD_COLOR_BLUE   0xFF0000FFU

extern volatile uint16_t lcd_bg_buffer[LCD_BG_WIDTH * LCD_BG_HEIGHT];
extern volatile uint32_t lcd_fg_buffer[LCD_FG_WIDTH * LCD_FG_HEIGHT];

#endif /* SIMPLE_LCD_FRAMEBUFFER_H */
