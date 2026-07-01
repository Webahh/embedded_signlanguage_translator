/**
 * @file    simple_ltdc.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   LTDC display controller driver header
 *
 * Usage
 * -----
 * 1. LTDC_Init()				- initialise LTDC peripheral
 * 2. LTDC_ConfigLayer1/2()		- configure layer parameters
 * 3. LTDC_UpdateLayerAddress() - update framebuffer pointer
 * 4. LTDC_FillLayer()			- fill layer with colour
 */

#ifndef SIMPLE_LTDC_H
#define SIMPLE_LTDC_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"

#include "palm_postprocessing.h"
#include "hand_landmark_postprocessing.h"

#define LTDC_BG_WIDTH  800
#define LTDC_BG_HEIGHT 480

#define LTDC_FG_WIDTH  800
#define LTDC_FG_HEIGHT 480

#define LTDC_DISPLAY_BUFFER_NB    2
#define LTDC_DISPLAY_BPP          3

#define LTDC_NN_BUFFER_NB    2
#define LTDC_NN_BPP          2

#define LTDC_NN_RAW_WIDTH   192
#define LTDC_NN_RAW_HEIGHT  192
#define LTDC_NN_RAW_BPP       3
#define LTDC_NN_RAW_SIZE    (LTDC_NN_RAW_WIDTH * LTDC_NN_RAW_HEIGHT * LTDC_NN_RAW_BPP)

#define LTDC_COLOR_BLACK  0xFF000000U
#define LTDC_COLOR_WHITE  0xFFFFFFFFU
#define LTDC_COLOR_RED    0xFFFF0000U
#define LTDC_COLOR_GREEN  0xFF00FF00U
#define LTDC_COLOR_BLUE   0xFF0000FFU

typedef enum {
    LTDC_OK    = 0,
    LTDC_ERROR = 1
} LTDC_Status_TypeDef;

typedef enum {
    LTDC_PF_ARGB8888 = 0b000,
    LTDC_PF_ABGR8888 = 0b001,
    LTDC_PF_RGBA8888 = 0b010,
    LTDC_PF_BGRA8888 = 0b011,
    LTDC_PF_RGB565   = 0b100,
    LTDC_PF_BGR565   = 0b101,
    LTDC_PF_RGB888   = 0b110,
    LTDC_PF_Flexible = 0b111,
} LTDC_PixelFormat_TypeDef;

/*
 * Flexible pixel format descriptor for custom layer color types.
 * Configures the LTDC FPF0R / FPF1R registers when PF = 0b111.
 */
typedef struct {
    uint8_t bytes_per_pixel;
    uint8_t alpha_len;
    uint8_t alpha_pos;
    uint8_t red_len;
    uint8_t red_pos;
    uint8_t green_len;
    uint8_t green_pos;
    uint8_t blue_len;
    uint8_t blue_pos;
} LTDC_Layer_FlexiblePixelFormat_TypeDef;

#define LTDC_FPF_ARGB4444_INIT { .bytes_per_pixel = 2, .alpha_len = 4, .alpha_pos = 12, .red_len = 4, .red_pos = 8, .green_len = 4, .green_pos = 4, .blue_len = 4, .blue_pos = 0 }
#define LTDC_FPF_ARGB1555_INIT { .bytes_per_pixel = 2, .alpha_len = 1, .alpha_pos = 15, .red_len = 5, .red_pos = 10, .green_len = 5, .green_pos = 5, .blue_len = 5, .blue_pos = 0 }

typedef struct LTDC_LayerConfig {
    LTDC_Layer_TypeDef                              *regs;
    volatile void                                   *fb;
    uint16_t                                         x;
    uint16_t                                         y;
    uint16_t                                         width;
    uint16_t                                         height;
    uint16_t                                         buf_width;
    LTDC_PixelFormat_TypeDef                         pixel_format;
    const LTDC_Layer_FlexiblePixelFormat_TypeDef    *flexible_fmt;
    uint8_t                                          const_alpha;
    uint8_t                                          per_pixel_alpha;
    uint32_t                                         default_color;
    uint8_t                                          blending_order;
} LTDC_LayerConfig_TypeDef;

// -- Mutable runtime state --
extern volatile uint8_t     ltdc_bg_buffer[LTDC_DISPLAY_BUFFER_NB][LTDC_BG_WIDTH * LTDC_BG_HEIGHT * LTDC_DISPLAY_BPP];
extern volatile uint8_t     ltdc_fg_buffer[2][LTDC_FG_WIDTH * LTDC_FG_HEIGHT * LTDC_NN_BPP];
extern volatile uint8_t     ltdc_nn_raw_buffer[2][LTDC_NN_RAW_SIZE];
extern volatile int         ltdc_bg_buffer_disp_idx;

/**
 * @brief Get bytes per pixel for a layer configuration
 *
 * @param [in]  cfg Layer configuration
 * @param [out] bpp Bytes per pixel
 *
 * @retval LTDC_OK    Success
 * @retval LTDC_ERROR Unknown pixel format or missing flexible format descriptor
 */
LTDC_Status_TypeDef LTDC_BytesPerPixel(const LTDC_LayerConfig_TypeDef *cfg, int *bpp);

/**
 * @brief Convert an ARGB colour to the native pixel value for a layer
 *
 * @param [in]  cfg   Layer configuration
 * @param [in]  color ARGB colour value
 * @param [out] pixel Native pixel value
 *
 * @retval LTDC_OK    Success
 * @retval LTDC_ERROR Conversion failed
 */
LTDC_Status_TypeDef LTDC_ColorToPixel(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color, uint32_t *pixel);

/**
 * @brief Initialise the LTDC peripheral and display
 */
void LTDC_Init(void);

/**
 * @brief Set the background colour
 *
 * @param [in] r Red component (0-255)
 * @param [in] g Green component (0-255)
 * @param [in] b Blue component (0-255)
 */
void LTDC_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Configure an LTDC layer from a configuration struct
 *
 * @param [in] cfg Layer configuration
 */
void LTDC_ConfigLayer(const LTDC_LayerConfig_TypeDef *cfg);

/**
 * @brief Fill an entire layer with a single colour
 *
 * @param [in] cfg   Layer configuration
 * @param [in] color ARGB fill colour
 */
void LTDC_LayerFill(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color);

/**
 * @brief Fill a layer with two colours side by side
 *
 * @param [in] cfg    Layer configuration
 * @param [in] color1 Left-side ARGB colour
 * @param [in] color2 Right-side ARGB colour
 */
void LTDC_LayerFill2Sides(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color1, uint32_t color2);

/**
 * @brief Draws circle on Layer at position (x,y) with color and radius
 *
 * @param [in] cfg		Layer configuration
 * @param [in] pos_x	Circle center position x
 * @param [in] pos_y	Circle center position y
 * @param [in] radius	radius in pixels
 * @param [in] color	color of the circle
 */
void LTDC_LayerDrawCricle(const LTDC_LayerConfig_TypeDef* cfg, uint16_t pos_x, uint16_t pos_y, uint16_t radius, uint32_t color);

/**
 * @brief Draw a filled rectangle on a layer
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB fill colour
 */
void LTDC_LayerDrawRect(const LTDC_LayerConfig_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Draw a 1-pixel border of a rectangle
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB border colour
 */
void LTDC_LayerDrawRectBorder(const LTDC_LayerConfig_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Blit an image onto a layer at a destination position
 *
 * @param [in] cfg   Layer configuration
 * @param [in] img   Source image data
 * @param [in] img_w Source image width (pixels)
 * @param [in] img_h Source image height (pixels)
 * @param [in] dst_x Destination X offset (pixels)
 * @param [in] dst_y Destination Y offset (pixels)
 */
void LTDC_BlitImage(const LTDC_LayerConfig_TypeDef *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y);


void LTDC_LayerDrawLine(const LTDC_LayerConfig_TypeDef *cfg, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

void DrawLandmarkROI(const HandROI_TypeDef *roi, uint32_t color);

void ClearPreviousROI(void);

void LTDC_BlitRGB888ToARGB4444(
    const LTDC_LayerConfig_TypeDef *cfg,
    const uint8_t *source,
    uint16_t source_width,
    uint16_t source_height,
    uint16_t destination_x,
    uint16_t destination_y
);

void DrawLandmarks(const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT]);

void ClearPreviousLandmarks(void);

/**
 * @brief Configure layer 1 using the global LTDC_Layer1Config
 */
void LTDC_ConfigLayer1(void);

/**
 * @brief Configure layer 2 using the global LTDC_Layer2Config
 */
void LTDC_ConfigLayer2(void);

/**
 * @brief Update the framebuffer address for a layer
 *
 * @param [in] cfg Layer configuration
 */
void LTDC_UpdateLayerAddress(const LTDC_LayerConfig_TypeDef *cfg);

#endif /* SIMPLE_LTDC_H */
