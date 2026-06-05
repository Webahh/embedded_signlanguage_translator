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
		.af		= GPIO_MODE_AF,
		.speed  = GPIO_SPEED_HIGH
};

const GPIO_cfg_TypeDef GPIO_XSPI_cfg = {
		.mode 	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_PP,
		.pupdr	= GPIO_PUPD_UP,
		.af		= GPIO_AF_XSPI,
		.speed 	= GPIO_SPEED_VERY_HIGH
};


