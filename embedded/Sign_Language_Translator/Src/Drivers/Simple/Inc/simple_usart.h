/*
 * simple_usart.h
 *
 *  Created on: 22.06.2026
 *      Author: Weber
 */

#ifndef SIMPLE_UART_H
#define SIMPLE_UART_H

#include <stdarg.h>
#include "stm32n657xx.h"
#include "simple_gpio.h"

#define USART_IRQ_ON 	1
#define USART_IRQ_OFF	0

void USART_Config(USART_TypeDef* USARTX, int baudrate, int use_irq, GPIO_cfg_TypeDef cfg);

void USART_putc(USART_TypeDef* USARTX, char c);
void USART_puts(USART_TypeDef* USARTX, const char* s);
void USART_print_uint(USART_TypeDef* USARTX, uint32_t num);
void USART_print_int(USART_TypeDef* USARTX, int32_t num);
void USART_print_hex8(USART_TypeDef* USARTX, uint8_t num);
void USART_print_hex32(USART_TypeDef* USARTX, uint32_t num);
void USART_printf(USART_TypeDef* USARTX, const char* fmt, ...);
void USART_vprintf(USART_TypeDef* USARTX, const char* fmt, va_list args);

#endif /* SIMPLE_UART_H */
