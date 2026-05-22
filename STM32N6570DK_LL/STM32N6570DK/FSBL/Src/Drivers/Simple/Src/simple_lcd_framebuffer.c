/*
 * simple_lcd_framebuffer.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_lcd_framebuffer.h"

uint16_t lcd_framebuffer[LCD_WIDTH * LCD_HEIGHT];

void LCD_Fill(uint16_t color){
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        lcd_framebuffer[i] = color;
    }
}

void LCD_DrawPixel(uint32_t x, uint32_t y, uint16_t color){
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }

    lcd_framebuffer[y * LCD_WIDTH + x] = color;
}


