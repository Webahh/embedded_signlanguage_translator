/*
 * simple_debug_log.c
 *
 *  Created on: 22.06.2026
 *      Author: Weber
 */

#include "simple_debug_log.h"

void debug_init(Debug_log_cfg_TypeDef cfg)
{
	if (cfg.enabled){
		// Init USART for debug output
		USART_Config(cfg.usart, cfg.baudrate, USART_IRQ_OFF, cfg.gpio_cfg);
		// Send startup message
		USART_printf(cfg.usart, "UART Debug online!\r\n");
		USART_puts(cfg.usart, "\n\r");
	}
}

