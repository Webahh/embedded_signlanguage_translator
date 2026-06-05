#include "simple_ltdc.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "config.h"

volatile uint16_t lcd_bg_buffer[LCD_BG_WIDTH * LCD_BG_HEIGHT] __attribute__((section(".psram_bss"), aligned(32)));
volatile uint32_t lcd_fg_buffer[LCD_FG_WIDTH * LCD_FG_HEIGHT] __attribute__((section(".psram_bss"), aligned(32)));

static void LCD_ConfigGPIO(void){
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

static void LCD_PowerOn(void){
    GPIO_BSRR_reset(GPIOE, 1);
    delay_ms(10);
    GPIO_BSRR_set(GPIOE, 1);
    delay_ms(10);

    GPIO_BSRR_set(GPIOQ, 3);
    GPIO_BSRR_set(GPIOQ, 6);
    GPIO_BSRR_set(GPIOG, 13);
}

static void LCD_ConfigTiming(void){
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

void LCD_Init(void){
    RCC_enable_LTDC_memory();
    RCC_config_LTDC_25MHz_clock();

    LCD_ConfigGPIO();
    LCD_PowerOn();

    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    LCD_ConfigTiming();

    LTDC->GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL);

    LTDC->BCCR = 0xFFFFFFFFUL;

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b){
    while (!(LTDC->CDSR & LTDC_CDSR_VDES));
    while (LTDC->CDSR & LTDC_CDSR_VDES);
    LTDC->BCCR = ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
}

/**
 * @ret return bytes of pixel format except for Flexible because it is flexible
 *		unsoported fmt: -1;
 */
static uint32_t LCD_BytesPerPixel(LCD_PixelFormat fmt){
    switch (fmt){
        case LCD_PF_ARGB8888: return  4;
        case LCD_PF_ABGR8888: return  4;
        case LCD_PF_RGBA8888: return  4;
        case LCD_PF_BGRA8888: return  4;
        case LCD_PF_RGB565:   return  2;
        case LCD_PF_BGR565:   return  2;
        case LCD_PF_RGB888:   return  3;
        case LCD_PF_Flexible: return  1;
		default:              return -1;
    }
}

void LCD_ConfigLayer(const LCD_LayerConfig *cfg){
    uint32_t hsync = 4U, hbp = 4U, vsync = 4U, vbp = 4U;
    uint32_t bpp = LCD_BytesPerPixel(cfg->pixel_format);
    uint32_t buf_pitch = cfg->buf_width * bpp;
    uint32_t disp_pitch = cfg->width * bpp;

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

    cfg->regs->PFCR  = (uint32_t)cfg->pixel_format;
    cfg->regs->FPF0R = 0U;
    cfg->regs->FPF1R = 0U;

    cfg->regs->CACR = cfg->const_alpha;
    cfg->regs->DCCR = cfg->default_color;

    // BFCR
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

void LCD_FillLayer(const LCD_LayerConfig *cfg, uint32_t color){
    if (cfg->pixel_format == LCD_PF_RGB565 || cfg->pixel_format == LCD_PF_BGR565) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        uint16_t c = LCD_ARGBtoRGB565(color);
        for (uint32_t i = 0; i < (uint32_t)cfg->buf_width * cfg->height; i++)
            fb[i] = c;
    } else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        for (uint32_t i = 0; i < (uint32_t)cfg->buf_width * cfg->height; i++)
            fb[i] = color;
    }
}

void LCD_FillLayer2Sides(const LCD_LayerConfig *cfg, uint32_t color1, uint32_t color2) {
    if (cfg->pixel_format == LCD_PF_RGB565 || cfg->pixel_format == LCD_PF_BGR565) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        uint16_t c1 = LCD_ARGBtoRGB565(color1);
        uint16_t c2 = LCD_ARGBtoRGB565(color2);
        for (uint32_t y = 0; y < cfg->height; y++)
            for (uint32_t x = 0; x < cfg->width; x++)
                fb[y * cfg->width + x] = (x < cfg->width / 2) ? c1 : c2;
    } else {
        volatile uint32_t *fb = (volatile uint32_t *)cfg->fb;
        for (uint32_t y = 0; y < cfg->height; y++)
            for (uint32_t x = 0; x < cfg->width; x++)
                fb[y * cfg->width + x] = (x < cfg->width / 2) ? color1 : color2;
    }
}

void LCD_BlitImage(const LCD_LayerConfig *cfg, const uint16_t *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y) {
    if (cfg->pixel_format == LCD_PF_RGB565 || cfg->pixel_format == LCD_PF_BGR565) {
        volatile uint16_t *fb = (volatile uint16_t *)cfg->fb;
        for (uint16_t y = 0; y < img_h && (dst_y + y) < cfg->height; y++)
            for (uint16_t x = 0; x < img_w && (dst_x + x) < cfg->width; x++)
                fb[(dst_y + y) * cfg->buf_width + dst_x + x] = img[y * img_w + x];
    }
}

void LCD_ConfigLayer1(void){
    LCD_ConfigLayer(&LCD_Layer1Config);
}

void LCD_ConfigLayer2(void){
    LCD_ConfigLayer(&LCD_Layer2Config);
}

