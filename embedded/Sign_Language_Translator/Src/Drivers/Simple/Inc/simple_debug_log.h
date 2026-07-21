/*
 * simple_debug_log.h
 *
 *  Created on: 22.06.2026
 *      Author: Weber
 */

#ifndef SIMPLE_DEBUG_LOG_H
#define SIMPLE_DEBUG_LOG_H

#include "simple_usart.h"
#include "simple_gpio.h"

typedef struct {
	USART_TypeDef* usart;
	GPIO_cfg_TypeDef gpio_cfg;
	int baudrate;
	int enabled;
} Debug_log_cfg_TypeDef;

void debug_init(Debug_log_cfg_TypeDef cfg);

#ifdef DEBUG
#define DEBUG_PRINTF(...) \
    do { if (dbg_cfg.enabled) USART_printf(dbg_cfg.usart, __VA_ARGS__); } while(0)
#else
#define DEBUG_PRINTF(...)
#endif

#endif /* SIMPLE_DEBUG_LOG_H */
