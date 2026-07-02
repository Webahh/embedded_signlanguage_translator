/**
 * @file    simple_ltdc.c
 * @author  Gross
 * @date    21.05.2026
 * @brief   LTDC display controller driver
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "simple_ltdc.h"
#include "simple_ltdc_color.h"

#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_ltdc_layer.h"
#include "simple_ltdc_layer_draw.h"
#include "config.h"

/**
 * @brief Configure LTDC GPIO pins
 */
static void _ConfigGPIO(void){
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
static void _PowerOn(void){
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
static void _ConfigTiming(void){
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

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

void LTDC_Init(void){
    RCC_enable_LTDC_memory();
    RCC_config_LTDC_25MHz_clock();

    _ConfigGPIO();
    _PowerOn();

    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    _ConfigTiming();

    LTDC->GCR &= ~(LTDC_GCR_HSPOL | LTDC_GCR_VSPOL);

    LTDC->BCCR = 0x0;

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void LTDC_BackgroundColor_Set(uint8_t r, uint8_t g, uint8_t b){
    while (!(LTDC->CDSR & LTDC_CDSR_VDES));
    while (LTDC->CDSR & LTDC_CDSR_VDES);
    LTDC->BCCR = ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
}
