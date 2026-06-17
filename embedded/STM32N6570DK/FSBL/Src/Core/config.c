/*
 * config.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include "config.h"

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

const RCC_PLL_ConfigTypeDef BOARD_PLL_CONFIG[4] = {
    { .CFGR1 = 0x201900, .CFGR2 = 0x0, .CFGR3 = 0x49000005 },
    { .CFGR1 = 0x807D00, .CFGR2 = 0x0, .CFGR3 = 0x49000005 },
    { .CFGR1 = 0x80E100, .CFGR2 = 0x0, .CFGR3 = 0x4A000005 },
    { .CFGR1 = 0x80E100, .CFGR2 = 0x0, .CFGR3 = 0x76000005 },
};

const RCC_IC_ConfigTypeDef BOARD_IC_CONFIG[20] = {
    [0]  = { .CFGR = 0x00000000 },  /* IC1:  PLL1 / 1  - CPU */
    [1]  = { .CFGR = 0x00010000 },  /* IC2:  PLL1 / 2  - SYSB/AXI */
    [5]  = { .CFGR = 0x10000000 },  /* IC6:  PLL2 / 1 */
    [10] = { .CFGR = 0x20000000 },  /* IC11: PLL3 / 1 */
    [15] = { .CFGR = 0x30010000 },  /* IC16: PLL4 / 2  - LTDC */
    [16] = { .CFGR = 0x10020000 },  /* IC17: PLL2 / 3  - DCMIPP */
    [17] = { .CFGR = 0x00270000 },  /* IC18: PLL1 / 40 - CSI */
};



CAM_Handle h_cam;

LCD_LayerConfig LCD_Layer1Config = {
    .regs            = LTDC_Layer1,
    .fb              = lcd_bg_buffer[0],
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


static const LCD_Layer_FlexiblePixelFormat LCD_FPF_ARGB4444 = LCD_FPF_ARGB4444_INIT;
LCD_LayerConfig LCD_Layer2Config = {
    .regs            = LTDC_Layer2,
    .fb              = lcd_fg_buffer[0],
    .x               = 10,
    .y               = 10,
    .width           = LCD_FG_WIDTH,
    .height          = LCD_FG_HEIGHT,
    .buf_width       = LCD_FG_WIDTH,
    .pixel_format    = LCD_PF_Flexible,
    .flexible_fmt    = &LCD_FPF_ARGB4444,
    .const_alpha     = 0xFF,
    .per_pixel_alpha = 1,
    .default_color   = 0x00,
    .blending_order  = 1,
};



