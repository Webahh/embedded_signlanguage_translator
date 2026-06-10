/*
 * config.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include "config.h"

const CAM_Handle h_cam;

const GPIO_cfg_TypeDef GPIO_default_cfg = {
		.mode  	= GPIO_MODE_OUTPUT,
		.otyp 	= GPIO_OTYPE_PP,
		.pupdr	= GPIO_PUPD_NONE,
		.af		= GPIO_AF_NONE,
		.speed  = GPIO_SPEED_LOW
};

const GPIO_cfg_TypeDef GPIO_LTDC_cfg = {
		.mode	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_PP,
	    .pupdr  = GPIO_PUPD_NONE,
	    .af		= GPIO_AF_LTDC,
		.speed  = GPIO_SPEED_VERY_HIGH
};

const GPIO_cfg_TypeDef GPIO_I2C_cfg = {
		.mode	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_OD,
		.pupdr	= GPIO_PUPD_UP,
		.af		= GPIO_AF_I2C,
		.speed  = GPIO_SPEED_HIGH
};

const GPIO_cfg_TypeDef GPIO_XSPI_cfg = {
		.mode 	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_PP,
		.pupdr	= GPIO_PUPD_UP,
		.af		= GPIO_AF_XSPI,
		.speed 	= GPIO_SPEED_VERY_HIGH
};

LCD_LayerConfig LCD_Layer1Config = {
    .regs            = LTDC_Layer1,
    .fb              = (volatile uint8_t *)lcd_bg_buffer[0],
    .x               = 0,
    .y               = 0,
    .width           = LCD_BG_WIDTH,
    .height          = LCD_BG_HEIGHT,
    .buf_width       = LCD_BG_WIDTH,
    .pixel_format    = LCD_PF_RGB888,
    .const_alpha     = 0xFF,
    .per_pixel_alpha = 0,
    .default_color   = 0,
    .blending_order  = 0,
};

LCD_LayerConfig LCD_Layer2Config = {
    .regs            = LTDC_Layer2,
    .fb              = lcd_fg_buffer,
    .x               = 10,
    .y               = 10,
    .width           = LCD_FG_WIDTH,
    .height          = LCD_FG_HEIGHT,
    .buf_width       = LCD_FG_WIDTH,
    .pixel_format    = LCD_PF_ARGB8888,
    .const_alpha     = 0xFF,
    .per_pixel_alpha = 1,
    .default_color   = 0x00000000U,
    .blending_order  = 1,
};



