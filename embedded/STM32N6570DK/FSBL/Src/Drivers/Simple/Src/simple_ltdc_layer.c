/**
 * @file simple_ltdc_layer.c
 * @author Groß
 * @date 02.07.2026
 * @brief Contains logic for LTDC
 */
#include <stdint.h>
#include <stddef.h>

#include "simple_ltdc_layer.h"

#include "simple_ltdc.h"
#include "config.h"

// ====================================
// Defines
// ====================================

// ====================================
// Variables
// ====================================

volatile uint8_t ltdc_layer_bg_buffer[LTDC_LAYER_DISPLAY_BUFFER_NB][LTDC_LAYER_BG_WIDTH * LTDC_LAYER_BG_HEIGHT * LTDC_LAYER_DISPLAY_BPP] __attribute__((section(".psram_bss"), aligned(32)));
volatile uint8_t ltdc_layer_fg_buffer[2][LTDC_LAYER_FG_WIDTH * LTDC_LAYER_FG_HEIGHT * LTDC_LAYER_NN_BPP] __attribute__((section(".psram_bss"), aligned(32)));
volatile uint8_t ltdc_layer_nn_raw_buffer[2][LTDC_LAYER_NN_RAW_SIZE] __attribute__((section(".psram_bss"), aligned(32)));
volatile int     ltdc_layer_bg_buffer_disp_idx;

// ====================================
// Private functions
// ====================================

/**
 * @brief Convert ARGB8888 to RGB565
 *
 * @param [in] argb ARGB8888 colour value
 * @return RGB565 pixel value
 */
static uint16_t _ARGBtoRGB565(uint32_t argb) {
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8)  & 0xFF;
    uint8_t b = (argb)       & 0xFF;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/**
 * @brief Encode FPF0R register from flexible pixel format descriptor
 *
 * @param [in] f Flexible pixel format descriptor
 * @return FPF0R register value
 */
static uint32_t _FPF_EncodeFPF0R(const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
    return ((uint32_t)(f->red_len  & 0xF)  << LTDC_LxFPF0R_RLEN_Pos) |
           ((uint32_t)(f->red_pos  & 0x1F) << LTDC_LxFPF0R_RPOS_Pos) |
           ((uint32_t)(f->alpha_len & 0xF)  << LTDC_LxFPF0R_ALEN_Pos) |
           ((uint32_t)(f->alpha_pos & 0x1F) << LTDC_LxFPF0R_APOS_Pos);
}

/**
 * @brief Encode FPF1R register from flexible pixel format descriptor
 *
 * @param [in] f Flexible pixel format descriptor
 * @return FPF1R register value
 */
static int32_t _FPF_EncodeFPF1R(const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
    return ((uint32_t)(f->bytes_per_pixel & 0x7)  << LTDC_LxFPF1R_PSIZE_Pos) |
           ((uint32_t)(f->blue_len        & 0xF)  << LTDC_LxFPF1R_BLEN_Pos) |
           ((uint32_t)(f->blue_pos        & 0x1F) << LTDC_LxFPF1R_BPOS_Pos) |
           ((uint32_t)(f->green_len       & 0xF)  << LTDC_LxFPF1R_GLEN_Pos) |
           ((uint32_t)(f->green_pos       & 0x1F) << LTDC_LxFPF1R_GPOS_Pos);
}

/**
 * @brief Convert ARGB8888 to flexible-format pixel value
 *
 * @param [in] argb ARGB8888 colour value
 * @param [in] f    Flexible pixel format descriptor
 * @return Pixel value encoded in the layer's flexible format
 */
static uint32_t _ARGBtoFlexible(uint32_t argb, const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
    uint32_t pixel = 0;
    if (f->alpha_len) {
        uint32_t v = ((argb >> 24) & 0xFF) >> (8 - f->alpha_len);
        pixel |= (v & ((1U << f->alpha_len) - 1U)) << f->alpha_pos;
    }
    if (f->red_len) {
        uint32_t v = ((argb >> 16) & 0xFF) >> (8 - f->red_len);
        pixel |= (v & ((1U << f->red_len) - 1U)) << f->red_pos;
    }
    if (f->green_len) {
        uint32_t v = ((argb >> 8) & 0xFF) >> (8 - f->green_len);
        pixel |= (v & ((1U << f->green_len) - 1U)) << f->green_pos;
    }
    if (f->blue_len) {
        uint32_t v = (argb & 0xFF) >> (8 - f->blue_len);
        pixel |= (v & ((1U << f->blue_len) - 1U)) << f->blue_pos;
    }
    return pixel;
}

/**
 * @brief Configure the pixel format for a single LTDC layer
 *
 * @param [in] cfg Layer configuration
 */
static void _PixelFormat_Set(const LTDC_Layer_Config_TypeDef *cfg){
    if (cfg->pixel_format == LTDC_PF_Flexible) {
        if (cfg->flexible_fmt != NULL) {
            cfg->regs->PFCR  = 0b111;
            cfg->regs->FPF0R = _FPF_EncodeFPF0R(cfg->flexible_fmt);
            cfg->regs->FPF1R = _FPF_EncodeFPF1R(cfg->flexible_fmt);
        }
        return;
    }

    cfg->regs->PFCR  = (uint32_t)cfg->pixel_format;
    cfg->regs->FPF0R = 0U;
    cfg->regs->FPF1R = 0U;
}

// ====================================
// API
// ====================================

void LTDC_ConfigLayer(const LTDC_Layer_Config_TypeDef *cfg){
    uint32_t hsync = 4U, hbp = 4U, vsync = 4U, vbp = 4U;
    int bpp;
    LTDC_Layer_BytesPerPixel(cfg, &bpp);

    uint32_t buf_pitch = cfg->buf_width * (uint32_t)bpp;
    uint32_t disp_pitch = cfg->width * (uint32_t)bpp;

    cfg->regs->CR = 0U;

    cfg->regs->CKCR = 0U;
    cfg->regs->PCR = 0U;

    cfg->regs->AFBA0R = 0U;
    cfg->regs->AFBA1R = 0U;
    cfg->regs->AFBLR  = 0U;
    cfg->regs->AFBLNR = 0U;

    cfg->regs->SISR  = 0U;
    cfg->regs->SOSR  = 0U;
    cfg->regs->SVSFR = 0U;
    cfg->regs->SVSPR = 0U;
    cfg->regs->SHSFR = 0U;
    cfg->regs->SHSPR = 0U;

    cfg->regs->CYR0R = 0U;
    cfg->regs->CYR1R = 0U;

    cfg->regs->WHPCR =
        ((hsync + hbp + cfg->x) << LTDC_LxWHPCR_WHSTPOS_Pos) |
        ((hsync + hbp + cfg->x + cfg->width - 1U) << LTDC_LxWHPCR_WHSPPOS_Pos);

    cfg->regs->WVPCR =
        ((vsync + vbp + cfg->y) << LTDC_LxWVPCR_WVSTPOS_Pos) |
        ((vsync + vbp + cfg->y + cfg->height - 1U) << LTDC_LxWVPCR_WVSPPOS_Pos);

    _PixelFormat_Set(cfg);

    cfg->regs->CACR = cfg->const_alpha;
    cfg->regs->DCCR = cfg->default_color;

    if (cfg->per_pixel_alpha){
        cfg->regs->BFCR |=
           (6U << LTDC_LxBFCR_BF1_Pos) |
           (7U << LTDC_LxBFCR_BF2_Pos);
    } else {
        cfg->regs->BFCR =
           (4U << LTDC_LxBFCR_BF1_Pos) |
           (5U << LTDC_LxBFCR_BF2_Pos);
    }
    cfg->regs->BFCR |= (cfg->blending_order == 0)? (0U << LTDC_LxBFCR_BOR_Pos): (1U << LTDC_LxBFCR_BOR_Pos);

    cfg->regs->CFBAR = (uint32_t)cfg->fb;

    cfg->regs->CFBLR =
        (buf_pitch << LTDC_LxCFBLR_CFBP_Pos) |
        ((disp_pitch + 7U) << LTDC_LxCFBLR_CFBLL_Pos);

    cfg->regs->CFBLNR = cfg->height;

    cfg->regs->CR = LTDC_LxCR_LEN;

    LTDC->SRCR = LTDC_SRCR_IMR;
    while (LTDC->SRCR & LTDC_SRCR_IMR);
}

void LTDC_Layer_Layer1_Config(void){
    LTDC_ConfigLayer(&LTDC_Layer1Config);
}

void LTDC_Layer_Layer2_Config(void){
    LTDC_ConfigLayer(&LTDC_Layer2Config);
}

void LTDC_Layer_Address_Set(const LTDC_Layer_Config_TypeDef *cfg){
    cfg->regs->CFBAR = (uint32_t)cfg->fb;
    LTDC->SRCR = LTDC_SRCR_VBR;
}

LTDC_Layer_Status_TypeDef LTDC_Layer_BytesPerPixel(const LTDC_Layer_Config_TypeDef *cfg, int *bpp) {
    switch (cfg->pixel_format) {
        case LTDC_PF_ARGB8888:
        case LTDC_PF_ABGR8888:
        case LTDC_PF_BGRA8888:
        case LTDC_PF_RGBA8888:
            *bpp = 4;
            return LTDC_Layer_OK;

        case LTDC_PF_RGB888:
            *bpp = 3;
            return LTDC_Layer_OK;

        case LTDC_PF_RGB565:
        case LTDC_PF_BGR565:
            *bpp = 2;
            return LTDC_Layer_OK;

        case LTDC_PF_Flexible: {
            if (cfg->flexible_fmt != NULL) {
                *bpp = cfg->flexible_fmt->bytes_per_pixel;
                return LTDC_Layer_OK;
            }
            return LTDC_Layer_ERROR;
        }
        default:
            return LTDC_Layer_ERROR;
    }
}

LTDC_Layer_Status_TypeDef LTDC_Layer_ColorToPixel(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color, uint32_t *pixel) {
    if (cfg->pixel_format == LTDC_PF_Flexible && cfg->flexible_fmt != NULL) {
        *pixel = _ARGBtoFlexible(color, cfg->flexible_fmt);
        return LTDC_Layer_OK;
    }
    if (cfg->pixel_format == LTDC_PF_RGB565 || cfg->pixel_format == LTDC_PF_BGR565) {
        *pixel = _ARGBtoRGB565(color);
        return LTDC_Layer_OK;
    }
    if (cfg->pixel_format == LTDC_PF_RGB888) {
        *pixel = ((color >> 16) & 0xFF) | (((color >> 8) & 0xFF) << 8) | ((color & 0xFF) << 16);
        return LTDC_Layer_OK;
    }
    *pixel = color | 0xFF000000;
    return LTDC_Layer_OK;
}


