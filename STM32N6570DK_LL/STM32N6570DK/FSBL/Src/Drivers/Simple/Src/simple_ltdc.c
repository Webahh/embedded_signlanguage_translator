/*
 * simple_ltdc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_ltdc.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"

static void LCD_ConfigGPIO(void){
    RCC_enable_GPIO(GPIOA);
    RCC_enable_GPIO(GPIOB);
    RCC_enable_GPIO(GPIOD);
    RCC_enable_GPIO(GPIOE);
    RCC_enable_GPIO(GPIOG);
    RCC_enable_GPIO(GPIOH);
    RCC_enable_GPIO(GPIOQ);

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
    delay_init();

    RCC_config_LTDC_clock();

    LCD_ConfigGPIO();
    LCD_PowerOn();

    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    LCD_ConfigTiming();

    LTDC->GCR |= LTDC_GCR_BCKEN;
    LTDC->GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL | LTDC_GCR_DEPOL | LTDC_GCR_PCPOL);

    LTDC->BCCR = 0x0000FF00U;

    LTDC->SRCR = LTDC_SRCR_IMR;
    while (LTDC->SRCR & LTDC_SRCR_IMR);

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void LCD_SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b){
    while (!(LTDC->CDSR & LTDC_CDSR_VDES));
    while (LTDC->CDSR & LTDC_CDSR_VDES);
    LTDC->BCCR = ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
}
