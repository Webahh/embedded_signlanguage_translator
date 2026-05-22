/*
 * simple_gpio.h
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_GPIO_H
#define SIMPLE_GPIO_H

#include "stm32n657xx.h"

#define GPIO_MODE_INPUT 	0
#define GPIO_MODE_OUTPUT	1
#define GPIO_MODE_AF		2
#define GPIO_MODE_ANALOG	3

#define GPIO_OTYPE_PP		0
#define GPIO_OTYPE_OD		1

#define GPIO_PUPD_NONE		0
#define GPIO_PUPD_UP		1
#define GPIO_PUPD_DOWN		2

#define GPIO_AF_NONE		-1
#define GPIO_I2C			4 // VERIFY

void GPIO_Config(GPIO_TypeDef* GPIOX, int pinNr, int mode, int otyp, int pupdr, int af);
int GPIO_get(GPIO_TypeDef* GPIOX, int pinNr);
void GPIO_BSRR_set(GPIO_TypeDef* GPIOX, int pinNr);
void GPIO_BSRR_reset(GPIO_TypeDef* GPIOX, int pinNr);

#endif // SIMPLE_GPIO_H
