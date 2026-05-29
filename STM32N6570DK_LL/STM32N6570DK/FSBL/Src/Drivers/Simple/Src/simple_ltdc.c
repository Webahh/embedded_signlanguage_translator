/*
 * simple_ltdc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_ltdc.h"
#include "simple_lcd_framebuffer.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"

static void LCD_ConfigGPIO(void){
    uint32_t pa_pins[] = {0, 1, 2, 7, 8, 15};
    for (int i = 0; i < 6; i++)
        GPIO_Config(GPIOA, pa_pins[i], GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    uint32_t pb_pins[] = {2, 4, 11, 12, 13, 14, 15};
    for (int i = 0; i < 7; i++)
        GPIO_Config(GPIOB, pb_pins[i], GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    GPIO_Config(GPIOD, 8, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);
    GPIO_Config(GPIOD, 9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);
    GPIO_Config(GPIOD, 15, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    GPIO_Config(GPIOE, 11, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    uint32_t pg_pins[] = {0, 1, 6, 8, 11, 12};
    for (int i = 0; i < 6; i++)
        GPIO_Config(GPIOG, pg_pins[i], GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    GPIO_Config(GPIOH, 3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);
    GPIO_Config(GPIOH, 4, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);
    GPIO_Config(GPIOH, 6, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_NONE, LTDC_AF);

    GPIO_Config(GPIOE, 1, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOG, 13, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOQ, 3, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOQ, 6, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
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
    RCC_config_LTDC_clock();

    LCD_ConfigGPIO();
    LCD_PowerOn();

    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    LCD_ConfigTiming();

    LTDC->GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL | LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);
    LTDC->GCR |= (LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);

    LTDC->BCCR = 0xFFFFFFFFUL;

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b){
    while (!(LTDC->CDSR & LTDC_CDSR_VDES));
    while (LTDC->CDSR & LTDC_CDSR_VDES);
    LTDC->BCCR = ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
}

void LCD_ConfigLayer1(void){
    uint32_t hsync = 4U;
    uint32_t hbp   = 4U;
    uint32_t vsync = 4U;
    uint32_t vbp   = 4U;

    uint32_t pitch = LCD_WIDTH * LCD_BYTES_PER_PIXEL;

    LTDC_Layer1->CR = 0U;

    LTDC_Layer1->CKCR = 0U;
    LTDC_Layer1->PCR = 0U;

    LTDC_Layer1->AFBA0R = 0U;
    LTDC_Layer1->AFBA1R = 0U;
    LTDC_Layer1->AFBLR  = 0U;
    LTDC_Layer1->AFBLNR = 0U;

    LTDC_Layer1->SISR  = 0U;
    LTDC_Layer1->SOSR  = 0U;
    LTDC_Layer1->SVSFR = 0U;
    LTDC_Layer1->SVSPR = 0U;
    LTDC_Layer1->SHSFR = 0U;
    LTDC_Layer1->SHSPR = 0U;

    LTDC_Layer1->CYR0R = 0U;
    LTDC_Layer1->CYR1R = 0U;

    LTDC_Layer1->WHPCR =
        ((hsync + hbp) << LTDC_LxWHPCR_WHSTPOS_Pos) |
        ((hsync + hbp + LCD_WIDTH - 1U) << LTDC_LxWHPCR_WHSPPOS_Pos);

    LTDC_Layer1->WVPCR =
        ((vsync + vbp) << LTDC_LxWVPCR_WVSTPOS_Pos) |
        ((vsync + vbp + LCD_HEIGHT - 1U) << LTDC_LxWVPCR_WVSPPOS_Pos);

    LTDC_Layer1->PFCR =  0U;
    LTDC_Layer1->FPF0R = 0U;
    LTDC_Layer1->FPF1R = 0U;

    LTDC_Layer1->CACR = 0xFF;
    LTDC_Layer1->DCCR = 0x00000000U;

    LTDC_Layer1->BFCR =
       (4U << LTDC_LxBFCR_BF1_Pos) |
       (5U << LTDC_LxBFCR_BF2_Pos);

    LTDC_Layer1->CFBAR = (uint32_t)lcd_framebuffer;

    LTDC_Layer1->CFBLR =
        (pitch << LTDC_LxCFBLR_CFBP_Pos) |
        ((pitch + 7U) << LTDC_LxCFBLR_CFBLL_Pos);

    LTDC_Layer1->CFBLNR = LCD_HEIGHT;

    LTDC_Layer1->CR = LTDC_LxCR_LEN;

    LTDC->SRCR = LTDC_SRCR_IMR;
    while (LTDC->SRCR & LTDC_SRCR_IMR) {
    }
}

void LCD_ConfigLayer2(void){
    uint32_t hsync = 4U;
    uint32_t hbp   = 4U;
    uint32_t vsync = 4U;
    uint32_t vbp   = 4U;

    uint32_t buf_pitch   = LCD_FG_WIDTH * LCD_BYTES_PER_PIXEL;
    uint32_t disp_pitch  = 400U * LCD_BYTES_PER_PIXEL;

    LTDC_Layer2->CR = 0U;

    LTDC_Layer2->CKCR = 0U;
    LTDC_Layer2->PCR = 0U;

    LTDC_Layer2->AFBA0R = 0U;
    LTDC_Layer2->AFBA1R = 0U;
    LTDC_Layer2->AFBLR  = 0U;
    LTDC_Layer2->AFBLNR = 0U;

    LTDC_Layer2->SISR  = 0U;
    LTDC_Layer2->SOSR  = 0U;
    LTDC_Layer2->SVSFR = 0U;
    LTDC_Layer2->SVSPR = 0U;
    LTDC_Layer2->SHSFR = 0U;
    LTDC_Layer2->SHSPR = 0U;

    LTDC_Layer2->CYR0R = 0U;
    LTDC_Layer2->CYR1R = 0U;

    LTDC_Layer2->WHPCR =
        ((hsync + hbp + 400U) << LTDC_LxWHPCR_WHSTPOS_Pos) |
        ((hsync + hbp + 400U + 400U - 1U) << LTDC_LxWHPCR_WHSPPOS_Pos);

    LTDC_Layer2->WVPCR =
        ((vsync + vbp) << LTDC_LxWVPCR_WVSTPOS_Pos) |
        ((vsync + vbp + 480U - 1U) << LTDC_LxWVPCR_WVSPPOS_Pos);

    LTDC_Layer2->PFCR =  0U;
    LTDC_Layer2->FPF0R = 0U;
    LTDC_Layer2->FPF1R = 0U;

    LTDC_Layer2->CACR = 0xFF;
    LTDC_Layer2->DCCR = 0x00000000U;

    LTDC_Layer2->BFCR =
       (4U << LTDC_LxBFCR_BF1_Pos) |
       (5U << LTDC_LxBFCR_BF2_Pos);

    LTDC_Layer2->CFBAR = (uint32_t)lcd_fg_buffer;

    LTDC_Layer2->CFBLR =
        (buf_pitch << LTDC_LxCFBLR_CFBP_Pos) |
        ((disp_pitch + 7U) << LTDC_LxCFBLR_CFBLL_Pos);

    LTDC_Layer2->CFBLNR = 480U;

    LTDC_Layer2->CR = LTDC_LxCR_LEN;

    LTDC->SRCR = LTDC_SRCR_IMR;
    while (LTDC->SRCR & LTDC_SRCR_IMR) {
    }
}
