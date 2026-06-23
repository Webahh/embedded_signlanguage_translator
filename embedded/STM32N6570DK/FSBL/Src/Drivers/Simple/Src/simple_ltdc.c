#include <stdint.h>
#include <stddef.h>

#include "simple_ltdc.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "config.h"

volatile uint8_t ltdc_bg_buffer[LTDC_DISPLAY_BUFFER_NB][LTDC_BG_WIDTH * LTDC_BG_HEIGHT * LTDC_DISPLAY_BPP] __attribute__((section(".psram_bss"), aligned(32)));
volatile uint8_t ltdc_fg_buffer[LTDC_NN_BUFFER_NB][LTDC_FG_WIDTH * LTDC_FG_HEIGHT * LTDC_NN_BPP] __attribute__((section(".psram_bss"), aligned(32)));
volatile int ltdc_bg_buffer_disp_idx = 0;

/**
 * @brief Configure LTDC GPIO pins
 */
static void LTDC_ConfigGPIO(void){
    uint32_t pa_pins[] = {0, 1, 2, 7, 8, 15};
    for (int i = 0; i < 6; i++)
        GPIO_Config(GPIOA, pa_pins[i], GPIO_LTDC_cfg);

    uint32_t pb_pins[] = {2, 4, 11, 12, 13, 14, 15};
    for (int i = 0; i < 7; i++)
        GPIO_Config(GPIOB, pb_pins[i], GPIO_LTDC_cfg);

    GPIO_Config(GPIOD, 8, GPIO_LTDC_cfg);
    GPIO_Config(GPIOD, 9, GPIO_LTDC_cfg);
    GPIO_Config(GPIOD, 15, GPIO_LTDC_cfg);

    GPIO_Config(GPIOE, 11, GPIO_LTDC_cfg);

    uint32_t pg_pins[] = {0, 1, 6, 8, 11, 12};
    for (int i = 0; i < 6; i++)
        GPIO_Config(GPIOG, pg_pins[i], GPIO_LTDC_cfg);

    GPIO_Config(GPIOH, 3, GPIO_LTDC_cfg);
    GPIO_Config(GPIOH, 4, GPIO_LTDC_cfg);
    GPIO_Config(GPIOH, 6, GPIO_LTDC_cfg);

    GPIO_Config(GPIOE, 1, GPIO_default_cfg);
    GPIO_Config(GPIOQ, 3, GPIO_default_cfg);
    GPIO_Config(GPIOQ, 6, GPIO_default_cfg);
    GPIO_Config(GPIOG, 13, GPIO_default_cfg);
}

/**
 * @brief Power on the display panel
 */
static void LTDC_PowerOn(void){
    GPIO_BSRR_reset(GPIOE, 1);
    TIMER_Delay_ms(10);
    GPIO_BSRR_set(GPIOE, 1);
    TIMER_Delay_ms(10);

    GPIO_BSRR_set(GPIOQ, 3);
    GPIO_BSRR_set(GPIOQ, 6);
    GPIO_BSRR_set(GPIOG, 13);
}

/**
 * @brief Configure LTDC display timings
 */
static void LTDC_ConfigTiming(void){
    uint32_t hsync = 4U, hbp = 4U, hfp = 4U, width = 800U;
    uint32_t vsync = 4U, vbp = 4U, vfp = 4U, height = 480U;

    LTDC->SSCR = ((hsync - 1U) << LTDC_SSCR_HSW_Pos) |
                 ((vsync - 1U) << LTDC_SSCR_VSH_Pos);

    LTDC->BPCR = ((hsync + hbp - 1U) << LTDC_BPCR_AHBP_Pos) |
                 ((vsync + vbp - 1U) << LTDC_BPCR_AVBP_Pos);

    LTDC->AWCR = ((hsync + hbp + width - 1U) << LTDC_AWCR_AAW_Pos) |
                 ((vsync + vbp + height - 1U) << LTDC_AWCR_AAH_Pos);

    LTDC->TWCR = ((hsync + hbp + width + hfp - 1U) << LTDC_TWCR_TOTALW_Pos) |
                 ((vsync + vbp + height + vfp - 1U) << LTDC_TWCR_TOTALH_Pos);
}

// ---- API ----

void LTDC_Init(void){
    RCC_enable_LTDC_memory();
    RCC_config_LTDC_25MHz_clock();

    LTDC_ConfigGPIO();
    LTDC_PowerOn();

    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    LTDC_ConfigTiming();

    LTDC->GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL);

    LTDC->BCCR = 0x0;

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void LTDC_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b){
    while (!(LTDC->CDSR & LTDC_CDSR_VDES));
    while (LTDC->CDSR & LTDC_CDSR_VDES);
    LTDC->BCCR = ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
}

// ---- Private helpers ----

/**
 * @brief Convert ARGB8888 to RGB565
 *
 * @param [in] argb ARGB8888 colour value
 * @return RGB565 pixel value
 */
static uint16_t LTDC_ARGBtoRGB565(uint32_t argb) {
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
static uint32_t LTDC_FPF_EncodeFPF0R(const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
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
static int32_t LTDC_FPF_EncodeFPF1R(const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
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
static uint32_t LTDC_ARGBtoFlexible(uint32_t argb, const LTDC_Layer_FlexiblePixelFormat_TypeDef *f) {
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

LTDC_Status_TypeDef LTDC_BytesPerPixel(const LTDC_LayerConfig_TypeDef *cfg, int *bpp) {
    switch (cfg->pixel_format) {
        case LTDC_PF_ARGB8888:
        case LTDC_PF_ABGR8888:
        case LTDC_PF_BGRA8888:
        case LTDC_PF_RGBA8888:
            *bpp = 4;
            return LTDC_OK;

        case LTDC_PF_RGB888:
            *bpp = 3;
            return LTDC_OK;

        case LTDC_PF_RGB565:
        case LTDC_PF_BGR565:
            *bpp = 2;
            return LTDC_OK;

        case LTDC_PF_Flexible: {
            if (cfg->flexible_fmt != NULL) {
                *bpp = cfg->flexible_fmt->bytes_per_pixel;
                return LTDC_OK;
            }
            return LTDC_ERROR;
        }
        default:
            return LTDC_ERROR;
    }
}

LTDC_Status_TypeDef LTDC_ColorToPixel(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color, uint32_t *pixel) {
    if (cfg->pixel_format == LTDC_PF_Flexible && cfg->flexible_fmt != NULL) {
        *pixel = LTDC_ARGBtoFlexible(color, cfg->flexible_fmt);
        return LTDC_OK;
    }
    if (cfg->pixel_format == LTDC_PF_RGB565 || cfg->pixel_format == LTDC_PF_BGR565) {
        *pixel = LTDC_ARGBtoRGB565(color);
        return LTDC_OK;
    }
    if (cfg->pixel_format == LTDC_PF_RGB888) {
        *pixel = ((color >> 16) & 0xFF) | (((color >> 8) & 0xFF) << 8) | ((color & 0xFF) << 16);
        return LTDC_OK;
    }
    *pixel = color | 0xFF000000;
    return LTDC_OK;
}

/**
 * @brief Configure the pixel format for a single LTDC layer
 *
 * @param [in] cfg Layer configuration
 */
static void LTDC_ConfigLayer_PixelFormat(const LTDC_LayerConfig_TypeDef *cfg){
    if (cfg->pixel_format == LTDC_PF_Flexible) {
        if (cfg->flexible_fmt != NULL) {
            cfg->regs->PFCR  = 0b111;
            cfg->regs->FPF0R = LTDC_FPF_EncodeFPF0R(cfg->flexible_fmt);
            cfg->regs->FPF1R = LTDC_FPF_EncodeFPF1R(cfg->flexible_fmt);
        }
        return;
    }

    cfg->regs->PFCR  = (uint32_t)cfg->pixel_format;
    cfg->regs->FPF0R = 0U;
    cfg->regs->FPF1R = 0U;
}

void LTDC_ConfigLayer(const LTDC_LayerConfig_TypeDef *cfg){
    uint32_t hsync = 4U, hbp = 4U, vsync = 4U, vbp = 4U;
    int bpp;
    LTDC_BytesPerPixel(cfg, &bpp);

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

    LTDC_ConfigLayer_PixelFormat(cfg);

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

void LTDC_FillLayer(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color){
    uint32_t pixel;
    LTDC_ColorToPixel(cfg, color, &pixel);
    int bpp;
    LTDC_BytesPerPixel(cfg, &bpp);
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

void LTDC_FillLayer2Sides(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color1, uint32_t color2) {
    uint32_t p1;
    LTDC_ColorToPixel(cfg, color1, &p1);
    uint32_t p2;
    LTDC_ColorToPixel(cfg, color2, &p2);
    int bpp;
    LTDC_BytesPerPixel(cfg, &bpp);
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

void LTDC_BlitImage(const LTDC_LayerConfig_TypeDef *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y) {
    int bpp;
    LTDC_BytesPerPixel(cfg, &bpp);

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

void LTDC_ConfigLayer1(void){
    LTDC_ConfigLayer(&LTDC_Layer1Config);
}

void LTDC_ConfigLayer2(void){
    LTDC_ConfigLayer(&LTDC_Layer2Config);
}

void LTDC_UpdateLayerAddress(const LTDC_LayerConfig_TypeDef *cfg){
    cfg->regs->CFBAR = (uint32_t)cfg->fb;
    LTDC->SRCR = LTDC_SRCR_VBR;
}
