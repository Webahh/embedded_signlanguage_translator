/*
 * simple_lcd_framebuffer.h
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#ifndef SIMPLE_LCD_FRAMEBUFFER_H
#define SIMPLE_LCD_FRAMEBUFFER_H

#include <stdint.h>

#define LCD_WIDTH  400
#define LCD_HEIGHT 240

#define LCD_COLOR_BLACK  0x0000
#define LCD_COLOR_WHITE  0xFFFF
#define LCD_COLOR_RED    0xF800
#define LCD_COLOR_GREEN  0x07E0
#define LCD_COLOR_BLUE   0x001F

#define lcd_framebuffer ((uint16_t*)0x34000000)

void LCD_Fill(uint16_t color);
void LCD_DrawPixel(uint32_t x, uint32_t y, uint16_t color);

#endif /* SIMPLE_LCD_FRAMEBUFFER_H */
