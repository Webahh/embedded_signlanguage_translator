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
#include "simple_camera.h"
#include "simple_rcc.h"

extern const GPIO_cfg_TypeDef GPIO_default_cfg;
extern const GPIO_cfg_TypeDef GPIO_LTDC_cfg;
extern const GPIO_cfg_TypeDef GPIO_I2C_cfg;
extern const GPIO_cfg_TypeDef GPIO_XSPI_cfg;

extern const RCC_PLL_ConfigTypeDef BOARD_PLL_CONFIG[4];
extern const RCC_IC_ConfigTypeDef BOARD_IC_CONFIG[20];

extern CAM_Handle h_cam;
extern LCD_LayerConfig LCD_Layer1Config;
extern LCD_LayerConfig LCD_Layer2Config;

#endif /* CONFIG_H_ */
