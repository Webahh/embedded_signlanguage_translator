/**
 * @file simple_ltdc_layer.h
 * @author Groß
 * @date 02.07.2026
 * @brief Contains logic for LTDC
 *
 * Usage
 * -----
 * 1. LTDC_ConfigLayer
 */

#ifndef SIMPLE_LTDC_LAYER_H
#define SIMPLE_LTDC_LAYER_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"


// ==========================================================
// Defines
// ==========================================================

#define LTDC_LAYER_BG_WIDTH  800
#define LTDC_LAYER_BG_HEIGHT 480

#define LTDC_LAYER_FG_WIDTH  800
#define LTDC_LAYER_FG_HEIGHT 480

#define LTDC_LAYER_DISPLAY_BUFFER_NB    2
#define LTDC_LAYER_DISPLAY_BPP          3

#define LTDC_LAYER_NN_BUFFER_NB    2
#define LTDC_LAYER_NN_BPP          2

#define LTDC_LAYER_NN_RAW_WIDTH   192
#define LTDC_LAYER_NN_RAW_HEIGHT  192
#define LTDC_LAYER_NN_RAW_BPP       3
#define LTDC_LAYER_NN_RAW_SIZE    (LTDC_LAYER_NN_RAW_WIDTH * LTDC_LAYER_NN_RAW_HEIGHT * LTDC_LAYER_NN_RAW_BPP)

#define LTDC_LAYER_FPF_ARGB4444_INIT { .bytes_per_pixel = 2, .alpha_len = 4, .alpha_pos = 12, .red_len = 4, .red_pos = 8, .green_len = 4, .green_pos = 4, .blue_len = 4, .blue_pos = 0 }
#define LTDC_LAYER_FPF_ARGB1555_INIT { .bytes_per_pixel = 2, .alpha_len = 1, .alpha_pos = 15, .red_len = 5, .red_pos = 10, .green_len = 5, .green_pos = 5, .blue_len = 5, .blue_pos = 0 }

// ==========================================================
// Typedefs
// ==========================================================


typedef enum {
    LTDC_PF_ARGB8888 = 0b000,
    LTDC_PF_ABGR8888 = 0b001,
    LTDC_PF_RGBA8888 = 0b010,
    LTDC_PF_BGRA8888 = 0b011,
    LTDC_PF_RGB565   = 0b100,
    LTDC_PF_BGR565   = 0b101,
    LTDC_PF_RGB888   = 0b110,
    LTDC_PF_Flexible = 0b111,
} LTDC_Layer_PixelFormat_TypeDef;

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

typedef struct LTDC_LayerConfig {
    LTDC_Layer_TypeDef                              *regs;
    volatile void                                   *fb;
    uint16_t                                         x;
    uint16_t                                         y;
    uint16_t                                         width;
    uint16_t                                         height;
    uint16_t                                         buf_width;
    LTDC_Layer_PixelFormat_TypeDef                         pixel_format;
    const LTDC_Layer_FlexiblePixelFormat_TypeDef    *flexible_fmt;
    uint8_t                                          const_alpha;
    uint8_t                                          per_pixel_alpha;
    uint32_t                                         default_color;
    uint8_t                                          blending_order;
} LTDC_Layer_Config_TypeDef;

typedef enum {
    LTDC_Layer_OK    = 0,
    LTDC_Layer_ERROR = 1
} LTDC_Layer_Status_TypeDef;

// ==========================================================
// API
// ==========================================================

// -- Mutable runtime state --
extern volatile uint8_t     ltdc_layer_bg_buffer[LTDC_LAYER_DISPLAY_BUFFER_NB][LTDC_LAYER_BG_WIDTH * LTDC_LAYER_BG_HEIGHT * LTDC_LAYER_DISPLAY_BPP];
extern volatile uint8_t     ltdc_layer_fg_buffer[2][LTDC_LAYER_FG_WIDTH * LTDC_LAYER_FG_HEIGHT * LTDC_LAYER_NN_BPP];
extern volatile uint8_t     ltdc_layer_nn_raw_buffer[2][LTDC_LAYER_NN_RAW_SIZE];
extern volatile int         ltdc_layer_bg_buffer_disp_idx;

/**
 * @brief Configure an LTDC layer from a configuration struct
 *
 * @param [in] cfg | Layer configuration
 */
void LTDC_ConfigLayer(const LTDC_Layer_Config_TypeDef *cfg);

/**
 * @brief Configure layer 1 using the global LTDC_Layer1Config
 */
void LTDC_Layer_Layer1_Config(void);

/**
 * @brief Configure layer 2 using the global LTDC_Layer2Config
 */
void LTDC_Layer_Layer2_Config(void);

/**
 * @brief Update the framebuffer address for a layer
 *
 * @param [in] cfg Layer configuration
 */
void LTDC_Layer_Address_Set(const LTDC_Layer_Config_TypeDef *cfg);

/**
 * @brief Get bytes per pixel for a layer configuration
 *
 * @param [in]  cfg Layer configuration
 * @param [out] bpp Bytes per pixel
 *
 * @retval LTDC_OK    Success
 * @retval LTDC_ERROR Unknown pixel format or missing flexible format descriptor
 */
LTDC_Layer_Status_TypeDef LTDC_Layer_BytesPerPixel(const LTDC_Layer_Config_TypeDef *cfg, int *bpp);

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
LTDC_Layer_Status_TypeDef LTDC_Layer_ColorToPixel(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color, uint32_t *pixel);

#endif
