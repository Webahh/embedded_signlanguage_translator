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

#define GPIO_SPEED_LOW        	0U
#define GPIO_SPEED_MEDIUM     	1U
#define GPIO_SPEED_HIGH       	2U
#define GPIO_SPEED_VERY_HIGH  	3U

#define GPIO_AF_NONE		-1
#define GPIO_I2C			4
#define LTDC_AF 			14
#define XSPI_AF 			9

// TODO: write a cfg struct for the configuration of the GPIOs

void GPIO_Config(GPIO_TypeDef* GPIOX, uint32_t pinNr, uint32_t mode, uint32_t otyp, uint32_t pupdr, uint32_t af, uint32_t speed);
uint32_t GPIO_get(GPIO_TypeDef* GPIOX, uint32_t pinNr);
void GPIO_BSRR_set(GPIO_TypeDef* GPIOX, uint32_t pinNr);
void GPIO_BSRR_reset(GPIO_TypeDef* GPIOX, uint32_t pinNr);
void GPIO_BSRR_toggle(GPIO_TypeDef* GPIOX, uint32_t pinNr);

#endif // SIMPLE_GPIO_H
