#ifndef SIMPLE_LTDC_H
#define SIMPLE_LTDC_H

#include <stdint.h>
#include "stm32n657xx.h"

typedef enum {
    LCD_PF_ARGB8888 = 0,
    LCD_PF_RGB888   = 1,
    LCD_PF_RGB565   = 2,
    LCD_PF_ARGB1555 = 3,
    LCD_PF_ARGB4444 = 4,
    LCD_PF_L8       = 5,
    LCD_PF_AL44     = 6,
    LCD_PF_AL88     = 7,
} LCD_PixelFormat;

typedef struct {
    LTDC_Layer_TypeDef *regs;          		/* LTDC_Layer1 or LTDC_Layer2 */
    volatile uint32_t  *fb;            		/* framebuffer address        */
    uint16_t            x;             		/* window X offset (pixels)   */
    uint16_t            y;             		/* window Y offset (pixels)   */
    uint16_t            width;         		/* displayed width (pixels)   */
    uint16_t            height;        		/* displayed height (lines)   */
    uint16_t            buf_width;     		/* framebuffer stride (pixels)*/
    LCD_PixelFormat     pixel_format;  		/* one of LCD_PF_*            */
    uint8_t             const_alpha;   		/* C = α*Cs + (1-α)*Cd        */
    uint8_t             per_pixel_alpha;	/* 1 = per-pixel, 0 = const   */
    uint32_t            default_color; 		/* color outside window (A/R/G/B)*/
} LCD_LayerConfig;

void LCD_Init(void);
void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b);

extern LCD_LayerConfig LCD_Layer1Config;
extern LCD_LayerConfig LCD_Layer2Config;

void LCD_ConfigLayer(const LCD_LayerConfig *cfg);
void LCD_FillLayer(const LCD_LayerConfig *cfg, uint32_t color);
void LCD_FillLayer2Sides(const LCD_LayerConfig *cfg, uint32_t color1, uint32_t color2);
void LCD_ConfigLayer1(void);
void LCD_ConfigLayer2(void);

#endif
