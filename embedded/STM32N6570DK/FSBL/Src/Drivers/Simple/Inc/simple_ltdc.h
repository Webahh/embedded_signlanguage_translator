#ifndef SIMPLE_LTDC_H
#define SIMPLE_LTDC_H

#include <stdint.h>
#include "stm32n657xx.h"

#define LCD_BG_WIDTH  800
#define LCD_BG_HEIGHT 480

#define LCD_FG_WIDTH  16*6
#define LCD_FG_HEIGHT 16*2

#define LCD_BYTES_PER_PIXEL 4U

#define DISPLAY_BUFFER_NB    2
#define DISPLAY_BPP          3

#define NN_DELAY        2
#define NN_BUFFER_NB    (NN_DELAY + 2)
#define NN_BPP          2


#define LCD_COLOR_BLACK  0xFF000000U
#define LCD_COLOR_WHITE  0xFFFFFFFFU
#define LCD_COLOR_RED    0xFFFF0000U
#define LCD_COLOR_GREEN  0xFF00FF00U
#define LCD_COLOR_BLUE   0xFF0000FFU

extern volatile uint8_t lcd_bg_buffer[DISPLAY_BUFFER_NB][LCD_BG_WIDTH * LCD_BG_HEIGHT * DISPLAY_BPP];
extern volatile uint8_t lcd_fg_buffer[NN_BUFFER_NB][LCD_FG_WIDTH * LCD_FG_HEIGHT * NN_BPP];
extern volatile int     lcd_bg_buffer_disp_idx;

typedef enum {
    LCD_PF_ARGB8888 = 0b000,
    LCD_PF_ABGR8888 = 0b001,
    LCD_PF_RGBA8888 = 0b010,
    LCD_PF_BGRA8888 = 0b011,
    LCD_PF_RGB565 	= 0b100,
    LCD_PF_BGR565   = 0b101,
    LCD_PF_RGB888   = 0b110,
    LCD_PF_Flexible = 0b111,
} LCD_PixelFormat;

static inline uint16_t LCD_ARGBtoRGB565(uint32_t argb) {
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8)  & 0xFF;
    uint8_t b = (argb)       & 0xFF;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static inline uint32_t LCD_RGB565toARGB(uint16_t rgb565, uint8_t alpha) {
    uint8_t r = (uint8_t)(((rgb565 >> 11) & 0x1F) << 3);
    uint8_t g = (uint8_t)(((rgb565 >> 5)  & 0x3F) << 2);
    uint8_t b = (uint8_t)(((rgb565)       & 0x1F) << 3);
    return ((uint32_t)alpha << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

typedef struct LCD_LayerConfig {
    LTDC_Layer_TypeDef *regs;               /* LTDC_Layer1 or LTDC_Layer2 */
    volatile void      *fb;                /* framebuffer address (void* for any format) */
    uint16_t            x;                  /* window X offset (pixels)   */
    uint16_t            y;                  /* window Y offset (pixels)   */
    uint16_t            width;              /* displayed width (pixels)   */
    uint16_t            height;             /* displayed height (lines)   */
    uint16_t            buf_width;          /* framebuffer stride (pixels)*/
    LCD_PixelFormat     pixel_format;       /* one of LCD_PF_*            */
    uint8_t             const_alpha;        /* constant alpha value       */
    uint8_t             per_pixel_alpha;    /* 1 = per-pixel, 0 = const   */
    uint32_t            default_color;      /* color outside window       */
    uint8_t             blending_order;     /* 0: background | 1: foreground */
} LCD_LayerConfig;

void LCD_Init(void);
void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b);

void LCD_ConfigLayer(const LCD_LayerConfig *cfg);
void LCD_FillLayer(const LCD_LayerConfig *cfg, uint32_t color);
void LCD_FillLayer2Sides(const LCD_LayerConfig *cfg, uint32_t color1, uint32_t color2);
void LCD_BlitImage(const LCD_LayerConfig *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y);
void LCD_ConfigLayer1(void);
void LCD_ConfigLayer2(void);
void LCD_UpdateLayerAddress(const LCD_LayerConfig *cfg);

#endif
