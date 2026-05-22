/*
 * simple_rcc.h
 *
 *  Created on: May 22, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_RCC_H
#define SIMPLE_RCC_H

#include "stm32n657xx.h"

void RCC_enable_GPIO(GPIO_TypeDef* GPIOX);
void RCC_enable_I2C(I2C_TypeDef* I2CX);
void RCC_reset_I2C(I2C_TypeDef* I2CX);
void RCC_setI2C_Clock_Source(I2C_TypeDef* I2CX, uint32_t source);

#endif // SIMPLE_RCC_H
