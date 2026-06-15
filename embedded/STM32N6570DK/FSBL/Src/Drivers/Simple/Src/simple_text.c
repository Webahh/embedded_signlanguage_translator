#include "simple_text.h"
#include "font_8x16.h"

void LCD_DrawChar(const LCD_LayerConfig *cfg, char c, int16_t x, int16_t y, uint32_t fg_color)
{
    if (c < FONT_8X16_FIRST_CHAR || c > FONT_8X16_LAST_CHAR)
        return;

    if (cfg->pixel_format != LCD_PF_RGB888)
        return;

    int idx = c - FONT_8X16_FIRST_CHAR;
    const uint8_t *glyph = font_8x16[idx];
    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    uint8_t r = (fg_color >> 16) & 0xFF;
    uint8_t g = (fg_color >> 8) & 0xFF;
    uint8_t b = fg_color & 0xFF;

    for (uint8_t row = 0; row < FONT_8X16_HEIGHT; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= height) continue;

        uint8_t bits = glyph[row];
        for (uint8_t col = 0; col < FONT_8X16_WIDTH; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= width) continue;
            if (bits & (1 << (7 - col))) {
                uint32_t off = ((uint32_t)py * buf_width + px) * 3;
                fb[off + 0] = r;
                fb[off + 1] = g;
                fb[off + 2] = b;
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
    if (cfg->pixel_format != LCD_PF_RGB888)
        return;

    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    uint8_t fr = (fg_color >> 16) & 0xFF;
    uint8_t fg = (fg_color >> 8) & 0xFF;
    uint8_t fb_c = fg_color & 0xFF;

    uint8_t br = (bg_color >> 16) & 0xFF;
    uint8_t bg_c = (bg_color >> 8) & 0xFF;
    uint8_t bb = bg_color & 0xFF;

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
                    uint32_t off = ((uint32_t)py * buf_width + px) * 3;
                    if (bits & (1 << (7 - col))) {
                        fb[off + 0] = fr;
                        fb[off + 1] = fg;
                        fb[off + 2] = fb_c;
                    } else {
                        fb[off + 0] = br;
                        fb[off + 1] = bg_c;
                        fb[off + 2] = bb;
                    }
                }
            }
        }
        cx += FONT_8X16_WIDTH;
        str++;
    }
}

void LCD_DrawStringScaled(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint8_t scale)
{
    if (cfg->pixel_format != LCD_PF_RGB888 || scale == 0)
        return;

    if (scale == 1) {
        LCD_DrawString(cfg, str, x, y, fg_color);
        return;
    }

    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    uint8_t r = (fg_color >> 16) & 0xFF;
    uint8_t g = (fg_color >> 8) & 0xFF;
    uint8_t b = fg_color & 0xFF;

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
                            uint32_t off = ((uint32_t)py * buf_width + px) * 3;
                            fb[off + 0] = r;
                            fb[off + 1] = g;
                            fb[off + 2] = b;
                        }
                    }
                }
            }
        }
        cx += FONT_8X16_WIDTH * scale;
        str++;
    }
}
