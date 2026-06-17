#include "simple_text.h"
#include "font_8x16.h"

static inline int _lcd_bpp(const LCD_LayerConfig *cfg) {
    if (cfg->pixel_format == LCD_PF_Flexible && cfg->flexible_fmt != NULL)
        return cfg->flexible_fmt->bytes_per_pixel;
    switch (cfg->pixel_format) {
        case LCD_PF_RGB565:
        case LCD_PF_BGR565:
            return 2;
        case LCD_PF_RGB888:
            return 3;
        default:
            return 4;
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

    int bpp = _lcd_bpp(cfg);

    uint32_t fg_flex = (cfg->pixel_format == LCD_PF_Flexible && cfg->flexible_fmt != NULL)
                       ? LCD_ARGBtoFlexible(fg_color, cfg->flexible_fmt) : 0;
    uint8_t r = (fg_color >> 16) & 0xFF;
    uint8_t g = (fg_color >> 8) & 0xFF;
    uint8_t b = fg_color & 0xFF;
    uint16_t rgb565 = LCD_ARGBtoRGB565(fg_color);
    uint32_t color32 = fg_color | 0xFF000000;

    for (uint8_t row = 0; row < FONT_8X16_HEIGHT; row++) {
        int16_t py = y + row;
        if (py < 0 || py >= height) continue;

        uint8_t bits = glyph[row];
        for (uint8_t col = 0; col < FONT_8X16_WIDTH; col++) {
            int16_t px = x + col;
            if (px < 0 || px >= width) continue;
            if (bits & (1 << (7 - col))) {
                uint32_t off = ((uint32_t)py * buf_width + px) * bpp;
                if (bpp == 2) {
                    *(volatile uint16_t *)(fb + off) = (cfg->flexible_fmt) ? (uint16_t)fg_flex : rgb565;
                } else if (bpp == 3) {
                    fb[off + 0] = r;
                    fb[off + 1] = g;
                    fb[off + 2] = b;
                } else {
                    *(volatile uint32_t *)(fb + off) = (cfg->flexible_fmt) ? fg_flex : color32;
                }
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

    int bpp = _lcd_bpp(cfg);
    int is_flex = (cfg->pixel_format == LCD_PF_Flexible && cfg->flexible_fmt != NULL);

    uint32_t fg_flex = is_flex ? LCD_ARGBtoFlexible(fg_color, cfg->flexible_fmt) : 0;
    uint32_t bg_flex = is_flex ? LCD_ARGBtoFlexible(bg_color, cfg->flexible_fmt) : 0;

    uint8_t fr = (fg_color >> 16) & 0xFF;
    uint8_t fg = (fg_color >> 8) & 0xFF;
    uint8_t fb_c = fg_color & 0xFF;

    uint8_t br = (bg_color >> 16) & 0xFF;
    uint8_t bg_c = (bg_color >> 8) & 0xFF;
    uint8_t bb = bg_color & 0xFF;

    uint16_t fg_565 = LCD_ARGBtoRGB565(fg_color);
    uint16_t bg_565 = LCD_ARGBtoRGB565(bg_color);
    uint32_t fg_32 = fg_color | 0xFF000000;
    uint32_t bg_32 = bg_color | 0xFF000000;

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
                    uint32_t off = ((uint32_t)py * buf_width + px) * bpp;
                    if (bpp == 2) {
                        uint16_t val = is_flex ? ((bits & (1 << (7 - col))) ? (uint16_t)fg_flex : (uint16_t)bg_flex)
                                               : ((bits & (1 << (7 - col))) ? fg_565 : bg_565);
                        *(volatile uint16_t *)(fb + off) = val;
                    } else if (bpp == 3) {
                        if (bits & (1 << (7 - col))) {
                            fb[off + 0] = fr;
                            fb[off + 1] = fg;
                            fb[off + 2] = fb_c;
                        } else {
                            fb[off + 0] = br;
                            fb[off + 1] = bg_c;
                            fb[off + 2] = bb;
                        }
                    } else {
                        uint32_t val = is_flex ? ((bits & (1 << (7 - col))) ? fg_flex : bg_flex)
                                               : ((bits & (1 << (7 - col))) ? fg_32 : bg_32);
                        *(volatile uint32_t *)(fb + off) = val;
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
    if (scale == 0)
        return;

    if (scale == 1) {
        LCD_DrawString(cfg, str, x, y, fg_color);
        return;
    }

    volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
    uint16_t buf_width = cfg->buf_width;
    uint16_t height = cfg->height;
    uint16_t width = cfg->width;

    int bpp = _lcd_bpp(cfg);
    int is_flex = (cfg->pixel_format == LCD_PF_Flexible && cfg->flexible_fmt != NULL);

    uint32_t fg_flex = is_flex ? LCD_ARGBtoFlexible(fg_color, cfg->flexible_fmt) : 0;
    uint8_t r = (fg_color >> 16) & 0xFF;
    uint8_t g = (fg_color >> 8) & 0xFF;
    uint8_t b = fg_color & 0xFF;
    uint16_t rgb565 = LCD_ARGBtoRGB565(fg_color);
    uint32_t color32 = fg_color | 0xFF000000;

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
                            uint32_t off = ((uint32_t)py * buf_width + px) * bpp;
                            if (bpp == 2) {
                                *(volatile uint16_t *)(fb + off) = is_flex ? (uint16_t)fg_flex : rgb565;
                            } else if (bpp == 3) {
                                fb[off + 0] = r;
                                fb[off + 1] = g;
                                fb[off + 2] = b;
                            } else {
                                *(volatile uint32_t *)(fb + off) = is_flex ? fg_flex : color32;
                            }
                        }
                    }
                }
            }
        }
        cx += FONT_8X16_WIDTH * scale;
        str++;
    }
}
