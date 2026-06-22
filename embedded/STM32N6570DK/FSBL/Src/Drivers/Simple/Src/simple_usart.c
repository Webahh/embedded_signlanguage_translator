/*
 * simple_usart.c
 *
 *  Created on: 22.06.2026
 *      Author: Weber
 */

#include "simple_usart.h"
#include "simple_rcc.h"

static void _ConfigGPIO(USART_TypeDef* USARTX, GPIO_cfg_TypeDef cfg)
{
	// Only USART1 needed for now. Therefore, no configuration for other USARTs!
	if(USARTX == USART1){
		GPIO_Config(GPIOE, 5, cfg);
		GPIO_Config(GPIOE, 6, cfg);
	}
}

void USART_Config(USART_TypeDef* USARTX, int baudrate, int use_irq, GPIO_cfg_TypeDef cfg)
{
	uint32_t periph_clk;
	uint32_t brr;

	RCC_enable_USART(USARTX);
	_ConfigGPIO(USARTX, cfg);
	periph_clk = RCC_GetUSARTClock(USARTX);

	USARTX->CR1 = 0;
	USARTX->CR2 = 0;
	USARTX->CR3 = 0;
	USARTX->PRESC = 0;

	brr = (periph_clk + (baudrate / 2)) / (uint32_t)baudrate;
	USARTX->BRR = brr;
    USARTX->ICR =
          USART_ICR_PECF
        | USART_ICR_FECF
        | USART_ICR_NECF
        | USART_ICR_ORECF
        | USART_ICR_IDLECF
        | USART_ICR_TCCF;

	USARTX->CR1 |= USART_CR1_TE | USART_CR1_RE;

	if (use_irq){
		USARTX->CR1 |= USART_CR1_RXNEIE;
        NVIC_SetPriority(USART1_IRQn, 2);
        NVIC_EnableIRQ(USART1_IRQn);
	}

	USARTX-> CR1 |= USART_CR1_UE;
}

void USART_putc(USART_TypeDef* USARTX, char c)
{
    while (!(USARTX->ISR & USART_ISR_TXE_TXFNF)) {}
    USARTX->TDR = (uint8_t)c;
}

void USART_puts(USART_TypeDef* USARTX, const char* s)
{
    while (*s)
    {
        USART_putc(USARTX, *s++);
    }
}

void USART_print_uint(USART_TypeDef* USARTX, uint32_t num)
{
    char buffer[10];
    int i = 0;

    if (num == 0)
    {
        USART_putc(USARTX, '0');
        return;
    }

    while (num > 0)
    {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i--)
    {
        USART_putc(USARTX, buffer[i]);
    }
}

void USART_print_int(USART_TypeDef* USARTX, int32_t num)
{
    if (num < 0)
    {
        USART_putc(USARTX, '-');
        num = -num;
    }

    USART_print_uint(USARTX, (uint32_t)num);
}

void USART_print_hex8(USART_TypeDef* USARTX, uint8_t num)
{
    static const char H[] = "0123456789ABCDEF";

    USART_puts(USARTX, "0x");
    USART_putc(USARTX, H[(num >> 4) & 0xF]);
    USART_putc(USARTX, H[num & 0xF]);
}

void USART_print_hex32(USART_TypeDef* USARTX, uint32_t num)
{
    static const char H[] = "0123456789ABCDEF";

    USART_puts(USARTX, "0x");

    for (int i = 28; i >= 0; i -= 4)
    {
        USART_putc(USARTX, H[(num >> i) & 0xF]);
    }
}

void USART_vprintf(USART_TypeDef* USARTX, const char* fmt, va_list args)
{
    while (*fmt) {
        if (*fmt == '%'){
            int long_flag = 0;
            fmt++;

            if (*fmt == 'l'){
                long_flag = 1;
                fmt++;
            }

            switch (*fmt){
                case 'd':{
                    if(long_flag){
                        long val = va_arg(args, long);
                        USART_print_int(USARTX, (int32_t)val);
                    } else {
                        int val = va_arg(args, int);
                        USART_print_int(USARTX, val);
                    }
                    break;
                }

                case 'u':{
                    if(long_flag){
                        unsigned long val = va_arg(args, unsigned long);
                        USART_print_uint(USARTX, (uint32_t)val);
                    } else {
                        unsigned int val = va_arg(args, unsigned int);
                        USART_print_uint(USARTX, val);
                    }
                    break;
                }

                case 'x':{
                    if(long_flag){
                        unsigned long val = va_arg(args, unsigned long);
                        USART_print_hex32(USARTX, (uint32_t)val);
                    } else {
                        uint32_t val = va_arg(args, uint32_t);
                        USART_print_hex32(USARTX, val);
                    }
                    break;
                }

                case 'c':{
                    char c = (char)va_arg(args, int);
                    USART_putc(USARTX, c);
                    break;
                }

                case 's':{
                    char* s = va_arg(args, char*);
                    USART_puts(USARTX, s);
                    break;
                }

                case '%':{
                    USART_putc(USARTX, '%');
                    break;
                }

                default:
                    USART_putc(USARTX, '?');
                    break;
            }
        }
        else {
            USART_putc(USARTX, *fmt);
        }

        fmt++;
    }
}

void USART_printf(USART_TypeDef* USARTX, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    USART_vprintf(USARTX, fmt, args);
    va_end(args);
}

