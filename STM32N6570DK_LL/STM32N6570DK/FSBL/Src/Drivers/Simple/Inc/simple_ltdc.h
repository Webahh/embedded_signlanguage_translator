#ifndef SIMPLE_LTDC_H
#define SIMPLE_LTDC_H

#include <stdint.h>
#include "stm32n657xx.h"

#define LTDC_PIXEL_FORMAT_RGB565  2U

typedef struct {
    uint16_t width;
    uint16_t height;

    uint16_t hsync;
    uint16_t hbp;
    uint16_t hfp;

    uint16_t vsync;
    uint16_t vbp;
    uint16_t vfp;

    uint32_t framebuffer;
    uint32_t pixel_format;
} LTDC_ConfigTypeDef;

void LCD_Init(void);
void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b);
void LCD_ConfigLayer1(void);

#endif /* SIMPLE_LTDC_H */
