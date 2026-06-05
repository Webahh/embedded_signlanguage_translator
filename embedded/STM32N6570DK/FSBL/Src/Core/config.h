/*
 * config.h
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "simple_gpio.h"
#include "simple_ltdc.h"

extern const GPIO_cfg_TypeDef GPIO_default_cfg;
extern const GPIO_cfg_TypeDef GPIO_LTDC_cfg;
extern const GPIO_cfg_TypeDef GPIO_I2C_cfg;
extern const GPIO_cfg_TypeDef GPIO_XSPI_cfg;

extern LCD_LayerConfig LCD_Layer1Config;
extern LCD_LayerConfig LCD_Layer2Config;

#endif /* CONFIG_H_ */
