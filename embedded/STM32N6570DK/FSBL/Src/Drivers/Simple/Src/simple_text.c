#include "simple_text.h"
#include "font_8x16.h"

/* ---------------------------------------------------------------------------
 * private methods
 * --------------------------------------------------------------------------- */
static void LCD_WritePixel(volatile uint8_t *fb, uint32_t off, uint32_t pixel, int bpp) {
    if (bpp == 2) {
        *(volatile uint16_t *)(fb + off) = (uint16_t)pixel;
    } else if (bpp == 3) {
        fb[off + 0] = (uint8_t)(pixel);
        fb[off + 1] = (uint8_t)(pixel >> 8);
        fb[off + 2] = (uint8_t)(pixel >> 16);
    } else {
        *(volatile uint32_t *)(fb + off) = pixel;
    }
}

void LCD_DrawChar(const LCD_LayerConfig *cfg, char c, int16_t x, int16_t y, uint32_t fg_color)
{
    if (c < FONT_8X16_FIRST_CHAR || c > FONT_8X16_LAST_CHAR)
        return;

    int idx = c - FONT_8X16_FIRST_CHAR;
    const uint8_t *glyph = font_8x16[idx];
    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    int bpp = LCD_BytesPerPixel(cfg);
    uint32_t pixel = LCD_ColorToPixel(cfg, fg_color);

    for (uint8_t row = 0; row < FONT_8X16_HEIGHT; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= height) continue;

        uint8_t bits = glyph[row];
        for (uint8_t col = 0; col < FONT_8X16_WIDTH; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= width) continue;
            if (bits & (1 << (7 - col))) {
                LCD_WritePixel(fb, ((uint32_t)py * buf_width + px) * bpp, pixel, bpp);
            }
        }
    }
}

void LCD_DrawString(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color)
{
    int16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += FONT_8X16_HEIGHT;
            str++;
            continue;
        }
        LCD_DrawChar(cfg, *str, cx, y, fg_color);
        cx += FONT_8X16_WIDTH;
        str++;
    }
}

void LCD_DrawStringBG(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint32_t bg_color)
{
    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    int bpp = LCD_BytesPerPixel(cfg);
    uint32_t fg = LCD_ColorToPixel(cfg, fg_color);
    uint32_t bg = LCD_ColorToPixel(cfg, bg_color);

    int16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += FONT_8X16_HEIGHT;
            str++;
            continue;
        }

        if (*str >= FONT_8X16_FIRST_CHAR && *str <= FONT_8X16_LAST_CHAR) {
            int idx = *str - FONT_8X16_FIRST_CHAR;
            const uint8_t *glyph = font_8x16[idx];

            for (uint8_t row = 0; row < FONT_8X16_HEIGHT; row++) {
                int16_t py = y + row;
                if (py < 0 || py >= height) continue;
                uint8_t bits = glyph[row];
                for (uint8_t col = 0; col < FONT_8X16_WIDTH; col++) {
                    int16_t px = cx + col;
                    if (px < 0 || px >= width) continue;
                    uint32_t sel = (bits & (1 << (7 - col))) ? fg : bg;
                    LCD_WritePixel(fb, ((uint32_t)py * buf_width + px) * bpp, sel, bpp);
                }
            }
        }
        cx += FONT_8X16_WIDTH;
        str++;
    }
}

void LCD_DrawStringScaled(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint8_t scale)
{
    if (scale == 0 || scale == 1) {
        LCD_DrawString(cfg, str, x, y, fg_color);
        return;
    }

    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    int bpp = LCD_BytesPerPixel(cfg);
    uint32_t pixel = LCD_ColorToPixel(cfg, fg_color);

    int16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += FONT_8X16_HEIGHT * scale;
            str++;
            continue;
        }

        if (*str >= FONT_8X16_FIRST_CHAR && *str <= FONT_8X16_LAST_CHAR) {
            int idx = *str - FONT_8X16_FIRST_CHAR;
            const uint8_t *glyph = font_8x16[idx];

            for (uint8_t row = 0; row < FONT_8X16_HEIGHT; row++) {
                uint8_t bits = glyph[row];
                for (uint8_t sy = 0; sy < scale; sy++) {
                    int16_t py = y + row * scale + sy;
                    if (py < 0 || py >= height) continue;
                    for (uint8_t col = 0; col < FONT_8X16_WIDTH; col++) {
                        if (!(bits & (1 << (7 - col)))) continue;
                        for (uint8_t sx = 0; sx < scale; sx++) {
                            int16_t px = cx + col * scale + sx;
                            if (px < 0 || px >= width) continue;
                            LCD_WritePixel(fb, ((uint32_t)py * buf_width + px) * bpp, pixel, bpp);
                        }
                    }
                }
            }
        }
        cx += FONT_8X16_WIDTH * scale;
        str++;
    }
}
