#ifndef SIMPLE_TEXT_H
#define SIMPLE_TEXT_H

#include <stdint.h>
#include <stddef.h>
#include "simple_ltdc.h"

#define TEXT_COLOR_WHITE   0x00FFFFFFU
#define TEXT_COLOR_BLACK   0x00000000U
#define TEXT_COLOR_RED     0x00FF0000U
#define TEXT_COLOR_GREEN   0x0000FF00U
#define TEXT_COLOR_BLUE    0x000000FFU
#define TEXT_COLOR_YELLOW  0x00FFFF00U
#define TEXT_COLOR_CYAN    0x0000FFFFU
#define TEXT_COLOR_MAGENTA 0x00FF00FFU

void LCD_DrawChar(const LCD_LayerConfig *cfg, char c, int16_t x, int16_t y, uint32_t fg_color);
void LCD_DrawString(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color);
void LCD_DrawStringBG(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint32_t bg_color);
void LCD_DrawStringScaled(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint8_t scale);

#endif
