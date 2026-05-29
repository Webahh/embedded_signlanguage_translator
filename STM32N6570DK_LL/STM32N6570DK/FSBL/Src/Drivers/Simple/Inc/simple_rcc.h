/*
 * simple_rcc.h
 *
 *  Created on: May 22, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_RCC_H
#define SIMPLE_RCC_H

#include "stm32n657xx.h"

#define LTDC_CLOCK_SOURCE_PCLK5      0U
#define LTDC_CLOCK_SOURCE_PER_CK     1U
#define LTDC_CLOCK_SOURCE_IC16_CK    2U
#define LTDC_CLOCK_SOURCE_HSI_DIV_CK 3U



void RCC_enable_GPIO(GPIO_TypeDef* GPIOX);

void RCC_enable_I2C(I2C_TypeDef* I2CX);
void RCC_reset_I2C(I2C_TypeDef* I2CX);
void RCC_setI2C_clock_source(I2C_TypeDef* I2CX, uint32_t source);

void RCC_enable_LTDC_memory(void);
void RCC_enable_LTDC(void);
void RCC_reset_LTDC(void);
void RCC_setLTDC_clock_source(uint32_t source);
void RCC_config_LTDC_clock(void);


#endif // SIMPLE_RCC_H
