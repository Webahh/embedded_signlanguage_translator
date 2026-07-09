/**
 * @file simple_ltdc_layer_draw.c
 * @author Groß
 * @date 02.07.2026
 * @brief Drawing primitives implementation for LTDC layers
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "simple_ltdc_layer_draw.h"

#include "simple_ltdc_layer.h"
#include "simple_ltdc_color.h"
#include "config.h"

#include "palm_postprocessing.h"

// ====================================
// Private Functions
// ====================================

static void _plot4(volatile uint16_t *fb, int16_t cx, int16_t cy,
                   int16_t x, int16_t y, uint16_t pixel,
                   uint16_t w, uint16_t h, uint16_t stride)
{
    int16_t px, py;
    px = cx + x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx - x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx + x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx - x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
}

static void _plot4_3(volatile uint8_t *fb, int16_t cx, int16_t cy,
                     int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b,
                     uint16_t w, uint16_t h, uint16_t stride)
{
    int16_t px, py; uint32_t off;
    px = cx + x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) { off = (py * stride + px) * 3; fb[off+0]=r; fb[off+1]=g; fb[off+2]=b; }
    px = cx - x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) { off = (py * stride + px) * 3; fb[off+0]=r; fb[off+1]=g; fb[off+2]=b; }
    px = cx + x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) { off = (py * stride + px) * 3; fb[off+0]=r; fb[off+1]=g; fb[off+2]=b; }
    px = cx - x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) { off = (py * stride + px) * 3; fb[off+0]=r; fb[off+1]=g; fb[off+2]=b; }
}

static void _plot4_32(volatile uint32_t *fb, int16_t cx, int16_t cy,
                      int16_t x, int16_t y, uint32_t pixel,
                      uint16_t w, uint16_t h, uint16_t stride)
{
    int16_t px, py;
    px = cx + x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx - x; py = cy + y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx + x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
    px = cx - x; py = cy - y; if (px >= 0 && px < w && py >= 0 && py < h) fb[py * stride + px] = pixel;
}


// ====================================
// API
// ====================================

void LTDC_Layer_Draw_Fill(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color){
    uint32_t pixel;
    LTDC_Layer_ColorToPixel(cfg, color, &pixel);
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);
    uint32_t n = (uint32_t)cfg->buf_width * cfg->height;

    if (bpp == 2) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        for (uint32_t i = 0; i < n; i++)
            fb[i] = (uint16_t)pixel;
    } else if (bpp == 3) {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        for (uint32_t i = 0; i < n; i++) {
            fb[i * 3 + 0] = (uint8_t)(pixel);
            fb[i * 3 + 1] = (uint8_t)(pixel >> 8);
            fb[i * 3 + 2] = (uint8_t)(pixel >> 16);
        }
    } else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        for (uint32_t i = 0; i < n; i++)
            fb[i] = pixel;
    }
}

void LTDC_Layer_Draw_Fill_2Sides(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color1, uint32_t color2) {
    uint32_t p1;
    LTDC_Layer_ColorToPixel(cfg, color1, &p1);
    uint32_t p2;
    LTDC_Layer_ColorToPixel(cfg, color2, &p2);
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);
    uint32_t half = cfg->width / 2;

    if (bpp == 2) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        for (uint32_t y = 0; y < cfg->height; y++)
            for (uint32_t x = 0; x < cfg->width; x++)
                fb[y * cfg->width + x] = (uint16_t)((x < half) ? p1 : p2);
    } else if (bpp == 3) {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        for (uint32_t y = 0; y < cfg->height; y++)
            for (uint32_t x = 0; x < cfg->width; x++) {
                uint32_t pixel = (x < half) ? p1 : p2;
                uint32_t off = (y * cfg->buf_width + x) * 3;
                fb[off + 0] = (uint8_t)(pixel);
                fb[off + 1] = (uint8_t)(pixel >> 8);
                fb[off + 2] = (uint8_t)(pixel >> 16);
            }
    } else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        for (uint32_t y = 0; y < cfg->height; y++)
            for (uint32_t x = 0; x < cfg->width; x++)
                fb[y * cfg->width + x] = (x < half) ? p1 : p2;
    }
}

void LTDC_Layer_Draw_Circle(const LTDC_Layer_Config_TypeDef *cfg, uint16_t pos_x, uint16_t pos_y, uint16_t radius, uint32_t color)
{
    uint32_t pixel;
    LTDC_Layer_ColorToPixel(cfg, color, &pixel);
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);

    int16_t x = radius;
    int16_t y = 0;
    int16_t p = 1 - (int16_t)radius;

    if (bpp == 2)
    {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        while (y <= x)
        {
            _plot4(fb, pos_x, pos_y, x, y, (uint16_t)pixel, cfg->width, cfg->height, cfg->buf_width);
            if (x != y)
                _plot4(fb, pos_x, pos_y, y, x, (uint16_t)pixel, cfg->width, cfg->height, cfg->buf_width);
            y++;
            if (p <= 0)
                p += 2 * y + 1;
            else { x--; p += 2 * (y - x) + 1; }
        }
    }
    else if (bpp == 3)
    {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        uint8_t r = (uint8_t)(pixel);
        uint8_t g = (uint8_t)(pixel >> 8);
        uint8_t b = (uint8_t)(pixel >> 16);
        while (y <= x)
        {
            _plot4_3(fb, pos_x, pos_y, x, y, r, g, b, cfg->width, cfg->height, cfg->buf_width);
            if (x != y)
                _plot4_3(fb, pos_x, pos_y, y, x, r, g, b, cfg->width, cfg->height, cfg->buf_width);
            y++;
            if (p <= 0)
                p += 2 * y + 1;
            else { x--; p += 2 * (y - x) + 1; }
        }
    }
    else
    {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        while (y <= x)
        {
            _plot4_32(fb, pos_x, pos_y, x, y, pixel, cfg->width, cfg->height, cfg->buf_width);
            if (x != y)
                _plot4_32(fb, pos_x, pos_y, y, x, pixel, cfg->width, cfg->height, cfg->buf_width);
            y++;
            if (p <= 0)
                p += 2 * y + 1;
            else { x--; p += 2 * (y - x) + 1; }
        }
    }
}

void LTDC_Layer_Draw_BlitImage(const LTDC_Layer_Config_TypeDef *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y) {
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);

    if (bpp == 2) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        const uint16_t *src = (const uint16_t *)img;
        for (uint16_t y = 0; y < img_h && (dst_y + y) < cfg->height; y++)
            for (uint16_t x = 0; x < img_w && (dst_x + x) < cfg->width; x++)
                fb[(dst_y + y) * cfg->buf_width + dst_x + x] = src[y * img_w + x];
    } else if (bpp == 3) {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        const uint8_t *src = (const uint8_t *)img;
        for (uint16_t y = 0; y < img_h && (dst_y + y) < cfg->height; y++)
            for (uint16_t x = 0; x < img_w && (dst_x + x) < cfg->width; x++) {
                uint32_t di = ((dst_y + y) * cfg->buf_width + dst_x + x) * 3;
                uint32_t si = (y * img_w + x) * 3;
                fb[di + 0] = src[si + 0];
                fb[di + 1] = src[si + 1];
                fb[di + 2] = src[si + 2];
            }
    } else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        const uint32_t *src = (const uint32_t *)img;
        for (uint16_t y = 0; y < img_h && (dst_y + y) < cfg->height; y++)
            for (uint16_t x = 0; x < img_w && (dst_x + x) < cfg->width; x++)
                fb[(dst_y + y) * cfg->buf_width + dst_x + x] = src[y * img_w + x];
    }
}

void LTDC_Layer_Draw_Rect(const LTDC_Layer_Config_TypeDef *cfg,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    uint32_t pixel;
    LTDC_Layer_ColorToPixel(cfg, color, &pixel);
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);

    if (bpp == 2)
    {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        for (uint16_t row = 0; row < h && (y + row) < cfg->height; row++)
            for (uint16_t col = 0; col < w && (x + col) < cfg->width; col++)
                fb[(y + row) * cfg->buf_width + x + col] = (uint16_t)pixel;
    }
    else if (bpp == 3)
    {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        uint8_t r = (uint8_t)(pixel);
        uint8_t g = (uint8_t)(pixel >> 8);
        uint8_t b = (uint8_t)(pixel >> 16);
        for (uint16_t row = 0; row < h && (y + row) < cfg->height; row++)
            for (uint16_t col = 0; col < w && (x + col) < cfg->width; col++)
            {
                uint32_t off = ((y + row) * cfg->buf_width + x + col) * 3;
                fb[off + 0] = r;
                fb[off + 1] = g;
                fb[off + 2] = b;
            }
    }
    else
    {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        for (uint16_t row = 0; row < h && (y + row) < cfg->height; row++)
            for (uint16_t col = 0; col < w && (x + col) < cfg->width; col++)
                fb[(y + row) * cfg->buf_width + x + col] = pixel;
    }
}

void LTDC_Layer_Draw_RectBorder(const LTDC_Layer_Config_TypeDef *cfg,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    LTDC_Layer_Draw_Rect(cfg, x, y, w, 1, color);
    if (h > 1)
        LTDC_Layer_Draw_Rect(cfg, x, y + h - 1, w, 1, color);
    if (w > 1)
    {
        LTDC_Layer_Draw_Rect(cfg, x, y + 1, 1, h - 2, color);
        LTDC_Layer_Draw_Rect(cfg, x + w - 1, y + 1, 1, h - 2, color);
    }
}

// ====================================
// Line / Landmark Drawing
// ====================================

#define _LANDMARK_DRAW_RADIUS          4U
#define _LANDMARK_LINE_THICKNESS       0
#define _LANDMARK_MAX_LINE_LEN_SQ      (180 * 180)

typedef struct {
    uint8_t a;
    uint8_t b;
} LandmarkConnection_TypeDef;

static const LandmarkConnection_TypeDef _hand_connections[] = {
    /* Thumb */
    {0, 1}, {1, 2}, {2, 3}, {3, 4},

    /* Index */
    {0, 5}, {5, 6}, {6, 7}, {7, 8},

    /* Middle */
    {0, 9}, {9, 10}, {10, 11}, {11, 12},

    /* Ring */
    {0, 13}, {13, 14}, {14, 15}, {15, 16},

    /* Pinky */
    {0, 17}, {17, 18}, {18, 19}, {19, 20},

    /* Palm */
    {5, 9}, {9, 13}, {13, 17}
};

#define _HAND_CONNECTION_COUNT \
    ((uint32_t)(sizeof(_hand_connections) / sizeof(_hand_connections[0])))

static bool _landmarks_drawn = false;

static uint16_t _previous_landmark_x[LANDMARK_POINT_COUNT];
static uint16_t _previous_landmark_y[LANDMARK_POINT_COUNT];
static uint8_t  _previous_landmark_valid[LANDMARK_POINT_COUNT];

static uint16_t _previous_line_x0[_HAND_CONNECTION_COUNT];
static uint16_t _previous_line_y0[_HAND_CONNECTION_COUNT];
static uint16_t _previous_line_x1[_HAND_CONNECTION_COUNT];
static uint16_t _previous_line_y1[_HAND_CONNECTION_COUNT];
static uint8_t  _previous_line_valid[_HAND_CONNECTION_COUNT];

static bool _roi_drawn = false;
static uint16_t _previous_roi_x;
static uint16_t _previous_roi_y;
static uint16_t _previous_roi_w;
static uint16_t _previous_roi_h;

static int32_t _abs_i32(int32_t v)
{
    return (v < 0) ? -v : v;
}

static uint8_t _isLinePlausible(
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1
)
{
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t dist_sq = dx * dx + dy * dy;

    return (dist_sq <= _LANDMARK_MAX_LINE_LEN_SQ) ? 1U : 0U;
}

static void _drawPixel(
    const LTDC_Layer_Config_TypeDef *cfg,
    uint16_t x,
    uint16_t y,
    uint32_t color
)
{
    if ((cfg == NULL) || (cfg->fb == NULL)) {
        return;
    }

    if ((x >= cfg->width) || (y >= cfg->height)) {
        return;
    }

    uint32_t pixel;
    LTDC_Layer_ColorToPixel(cfg, color, &pixel);

    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);

    if (bpp == 2) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        fb[(uint32_t)y * cfg->buf_width + x] = (uint16_t)pixel;
    }
    else if (bpp == 3) {
        volatile uint8_t *fb = (volatile uint8_t *)cfg->fb;
        uint32_t off = ((uint32_t)y * cfg->buf_width + x) * 3U;

        fb[off + 0U] = (uint8_t)(pixel);
        fb[off + 1U] = (uint8_t)(pixel >> 8U);
        fb[off + 2U] = (uint8_t)(pixel >> 16U);
    }
    else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        fb[(uint32_t)y * cfg->buf_width + x] = pixel;
    }
}

static void _drawLine(
    const LTDC_Layer_Config_TypeDef *cfg,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
)
{
    if ((cfg == NULL) || (cfg->fb == NULL)) {
        return;
    }

    int32_t dx = _abs_i32(x1 - x0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -_abs_i32(y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    while (1) {
        if ((x0 >= 0) &&
            (y0 >= 0) &&
            (x0 < (int32_t)cfg->width) &&
            (y0 < (int32_t)cfg->height)) {

            _drawPixel(
                cfg,
                (uint16_t)x0,
                (uint16_t)y0,
                color
            );
        }

        if ((x0 == x1) && (y0 == y1)) {
            break;
        }

        int32_t e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void _drawLineThick(
    const LTDC_Layer_Config_TypeDef *cfg,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
)
{
    for (int32_t ox = -_LANDMARK_LINE_THICKNESS;
         ox <= _LANDMARK_LINE_THICKNESS;
         ox++) {

        for (int32_t oy = -_LANDMARK_LINE_THICKNESS;
             oy <= _LANDMARK_LINE_THICKNESS;
             oy++) {

            _drawLine(
                cfg,
                x0 + ox,
                y0 + oy,
                x1 + ox,
                y1 + oy,
                color
            );
        }
    }
}

// ====================================
// Image Conversion
// ====================================

void LTDC_BlitRGB888ToARGB4444(
    const LTDC_Layer_Config_TypeDef *cfg,
    const uint8_t *source,
    uint16_t source_width,
    uint16_t source_height,
    uint16_t destination_x,
    uint16_t destination_y)
{
    if ((cfg == NULL) ||
        (cfg->fb == NULL) ||
        (source == NULL)) {
        return;
    }

    volatile uint16_t *framebuffer =
        (volatile uint16_t *)cfg->fb;

    for (uint16_t y = 0U; y < source_height; y++) {
        const uint32_t target_y =
            (uint32_t)destination_y + y;

        if (target_y >= cfg->height) {
            break;
        }

        for (uint16_t x = 0U; x < source_width; x++) {
            const uint32_t target_x =
                (uint32_t)destination_x + x;

            if (target_x >= cfg->width) {
                break;
            }

            const uint32_t source_offset =
                ((uint32_t)y * source_width + x) * 3U;

            const uint8_t red =
                source[source_offset + 0U];

            const uint8_t green =
                source[source_offset + 1U];

            const uint8_t blue =
                source[source_offset + 2U];

            const uint16_t pixel =
                (uint16_t)(
                    (0xFU << 12U) |
                    ((uint16_t)(red   >> 4U) << 8U) |
                    ((uint16_t)(green >> 4U) << 4U) |
                    ((uint16_t)(blue  >> 4U))
                );

            framebuffer[
                target_y * cfg->buf_width + target_x
            ] = pixel;
        }
    }
}

// ====================================
// Landmark Drawing
// ====================================

void LTDC_Layer_Draw_Landmarks(const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT])
{
    if (points == NULL) {
        return;
    }

    bool point_drawn = false;

    int32_t px[LANDMARK_POINT_COUNT];
    int32_t py[LANDMARK_POINT_COUNT];
    uint8_t valid[LANDMARK_POINT_COUNT];

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        px[i] =
            (int32_t)(points[i].x *
                      (float)LTDC_Layer2Config.width);

        py[i] =
            (int32_t)(points[i].y *
                      (float)LTDC_Layer2Config.height);

        valid[i] = 0U;
        _previous_landmark_valid[i] = 0U;

        if ((px[i] < 0) ||
            (py[i] < 0) ||
            (px[i] >= (int32_t)LTDC_Layer2Config.width) ||
            (py[i] >= (int32_t)LTDC_Layer2Config.height)) {
            continue;
        }

        valid[i] = 1U;
    }

    for (uint32_t i = 0U; i < _HAND_CONNECTION_COUNT; i++) {
        const uint32_t a = _hand_connections[i].a;
        const uint32_t b = _hand_connections[i].b;

        _previous_line_valid[i] = 0U;

        if ((a >= LANDMARK_POINT_COUNT) ||
            (b >= LANDMARK_POINT_COUNT)) {
            continue;
        }

        if ((valid[a] == 0U) ||
            (valid[b] == 0U)) {
            continue;
        }

        if (!_isLinePlausible(px[a], py[a], px[b], py[b])) {
            continue;
        }

        _drawLineThick(
            &LTDC_Layer2Config,
            px[a],
            py[a],
            px[b],
            py[b],
            LTDC_LAYER_COLOR_GREEN
        );

        _previous_line_x0[i] = (uint16_t)px[a];
        _previous_line_y0[i] = (uint16_t)py[a];
        _previous_line_x1[i] = (uint16_t)px[b];
        _previous_line_y1[i] = (uint16_t)py[b];
        _previous_line_valid[i] = 1U;
    }

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        if (valid[i] == 0U) {
            continue;
        }

        LTDC_Layer_Draw_Circle(
            &LTDC_Layer2Config,
            (uint16_t)px[i],
            (uint16_t)py[i],
            _LANDMARK_DRAW_RADIUS,
            LTDC_LAYER_COLOR_RED
        );

        _previous_landmark_x[i] = (uint16_t)px[i];
        _previous_landmark_y[i] = (uint16_t)py[i];
        _previous_landmark_valid[i] = 1U;

        point_drawn = true;
    }

    _landmarks_drawn = point_drawn;
}

void LTDC_Layer_Draw_LandmarksClearPrevious(void)
{
    if (!_landmarks_drawn) {
        return;
    }

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        if (_previous_landmark_valid[i] == 0U) {
            continue;
        }

        LTDC_Layer_Draw_Circle(
            &LTDC_Layer2Config,
            _previous_landmark_x[i],
            _previous_landmark_y[i],
            _LANDMARK_DRAW_RADIUS,
            0x00000000U
        );

        _previous_landmark_valid[i] = 0U;
    }

    for (uint32_t i = 0U; i < _HAND_CONNECTION_COUNT; i++) {
        if (_previous_line_valid[i] == 0U) {
            continue;
        }

        _drawLineThick(
            &LTDC_Layer2Config,
            _previous_line_x0[i],
            _previous_line_y0[i],
            _previous_line_x1[i],
            _previous_line_y1[i],
            0x00000000U
        );

        _previous_line_valid[i] = 0U;
    }

    _landmarks_drawn = false;
}

// ====================================
// ROI Drawing
// ====================================

void LTDC_Layer_Draw_ROILandmark(const HandROI_TypeDef *roi, uint32_t color)
{
    if (roi == NULL) {
        return;
    }

    int32_t x0 =
        (int32_t)(roi->corners[0][0] *
                  LTDC_Layer2Config.width);

    int32_t y0 =
        (int32_t)(roi->corners[0][1] *
                  LTDC_Layer2Config.height);

    int32_t x1 =
        (int32_t)(roi->corners[2][0] *
                  LTDC_Layer2Config.width);

    int32_t y1 =
        (int32_t)(roi->corners[2][1] *
                  LTDC_Layer2Config.height);

    if (x0 < 0) {
        x0 = 0;
    }

    if (y0 < 0) {
        y0 = 0;
    }

    if (x1 >= (int32_t)LTDC_Layer2Config.width) {
        x1 = (int32_t)LTDC_Layer2Config.width - 1;
    }

    if (y1 >= (int32_t)LTDC_Layer2Config.height) {
        y1 = (int32_t)LTDC_Layer2Config.height - 1;
    }

    if ((x1 <= x0) || (y1 <= y0)) {
        return;
    }

    LTDC_Layer_Draw_RectBorder(
        &LTDC_Layer2Config,
        (uint16_t)x0,
        (uint16_t)y0,
        (uint16_t)(x1 - x0),
        (uint16_t)(y1 - y0),
        color
    );

    _previous_roi_x = (uint16_t)x0;
    _previous_roi_y = (uint16_t)y0;
    _previous_roi_w = (uint16_t)(x1 - x0);
    _previous_roi_h = (uint16_t)(y1 - y0);

    _roi_drawn = true;
}

void LTDC_Layer_Draw_ROIClearPrevious(void)
{
    if (!_roi_drawn) {
        return;
    }

    LTDC_Layer_Draw_RectBorder(
        &LTDC_Layer2Config,
        _previous_roi_x,
        _previous_roi_y,
        _previous_roi_w,
        _previous_roi_h,
        0x00000000U
    );

    _roi_drawn = false;
}

// ====================================
// Direct Drawing (no tracking/clearing)
// ====================================

void LTDC_Layer_Draw_LandmarksDirect(const LTDC_Layer_Config_TypeDef *cfg, const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT])
{
    if (cfg == NULL || points == NULL) {
        return;
    }

    int32_t px[LANDMARK_POINT_COUNT];
    int32_t py[LANDMARK_POINT_COUNT];
    uint8_t valid[LANDMARK_POINT_COUNT];

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        px[i] = (int32_t)(points[i].x * (float)cfg->width);
        py[i] = (int32_t)(points[i].y * (float)cfg->height);
        valid[i] = 1U;

        if (px[i] < 0 || py[i] < 0 || px[i] >= (int32_t)cfg->width || py[i] >= (int32_t)cfg->height) {
            valid[i] = 0U;
        }
    }

    for (uint32_t i = 0U; i < _HAND_CONNECTION_COUNT; i++) {
        const uint32_t a = _hand_connections[i].a;
        const uint32_t b = _hand_connections[i].b;

        if (a >= LANDMARK_POINT_COUNT || b >= LANDMARK_POINT_COUNT) continue;
        if (valid[a] == 0U || valid[b] == 0U) continue;
        if (!_isLinePlausible(px[a], py[a], px[b], py[b])) continue;

        _drawLineThick(cfg, px[a], py[a], px[b], py[b], LTDC_LAYER_COLOR_GREEN);
    }

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        if (valid[i] == 0U) continue;

        LTDC_Layer_Draw_Circle(cfg, (uint16_t)px[i], (uint16_t)py[i], _LANDMARK_DRAW_RADIUS, LTDC_LAYER_COLOR_RED);
    }
}

void LTDC_Layer_Draw_ROIDirect(const LTDC_Layer_Config_TypeDef *cfg, const HandROI_TypeDef *roi, uint32_t color)
{
    if (cfg == NULL || roi == NULL) {
        return;
    }

    int32_t x0 = (int32_t)(roi->corners[0][0] * cfg->width);
    int32_t y0 = (int32_t)(roi->corners[0][1] * cfg->height);
    int32_t x1 = (int32_t)(roi->corners[2][0] * cfg->width);
    int32_t y1 = (int32_t)(roi->corners[2][1] * cfg->height);

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)cfg->width) x1 = (int32_t)cfg->width - 1;
    if (y1 >= (int32_t)cfg->height) y1 = (int32_t)cfg->height - 1;
    if (x1 <= x0 || y1 <= y0) return;

    LTDC_Layer_Draw_RectBorder(cfg, (uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - x0), (uint16_t)(y1 - y0), color);
}

