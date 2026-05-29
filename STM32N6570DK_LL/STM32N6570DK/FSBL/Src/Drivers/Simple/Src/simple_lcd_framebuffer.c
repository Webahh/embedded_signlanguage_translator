/*
 * simple_lcd_framebuffer.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_lcd_framebuffer.h"

volatile uint32_t lcd_bg_buffer[LCD_WIDTH * LCD_HEIGHT] __attribute__((section(".psram_bss"), aligned(32)));

void LCD_Fill(uint32_t color){
	volatile uint32_t* ptr = lcd_framebuffer;

    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        ptr[i] = color;
    }
}

/*
void LCD_DrawPixel(uint32_t x, uint32_t y, uint16_t color){
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }

    lcd_framebuffer[y * LCD_WIDTH + x] = color;
}
*/


