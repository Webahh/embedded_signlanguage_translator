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
	RCC_Clock_USART_get(USARTX, &periph_clk);

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
            int width = 0;
            int precision = 6;
            int left_justify = 0;
            int got_precision = 0;
            fmt++;

            if (*fmt == '-'){
                left_justify = 1;
                fmt++;
            }

            while (*fmt >= '0' && *fmt <= '9'){
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            if (*fmt == '.'){
                fmt++;
                precision = 0;
                got_precision = 1;
                while (*fmt >= '0' && *fmt <= '9'){
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }

            if (*fmt == 'l'){
                long_flag = 1;
                fmt++;
            }

            switch (*fmt){
                case 'd':{
                    long val;
                    if(long_flag){
                        val = va_arg(args, long);
                    } else {
                        val = va_arg(args, int);
                    }
                    char b[16];
                    int i = 0;
                    if (val < 0){
                        b[i++] = '-';
                        val = -val;
                    }
                    uint32_t uv = (uint32_t)val;
                    if (uv == 0){
                        b[i++] = '0';
                    } else {
                        char r[14];
                        int ri = 0;
                        while (uv){
                            r[ri++] = '0' + (uv % 10);
                            uv /= 10;
                        }
                        while (ri--) b[i++] = r[ri];
                    }
                    b[i] = '\0';
                    if (width > 0 && !left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
                    }
                    USART_puts(USARTX, b);
                    if (width > 0 && left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
                    }
                    break;
                }

                case 'u':{
                    unsigned long val;
                    if(long_flag){
                        val = va_arg(args, unsigned long);
                    } else {
                        val = va_arg(args, unsigned int);
                    }
                    char b[14];
                    int i = 0;
                    if (val == 0){
                        b[i++] = '0';
                    } else {
                        char r[14];
                        int ri = 0;
                        while (val){
                            r[ri++] = '0' + (val % 10);
                            val /= 10;
                        }
                        while (ri--) b[i++] = r[ri];
                    }
                    b[i] = '\0';
                    if (width > 0 && !left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
                    }
                    USART_puts(USARTX, b);
                    if (width > 0 && left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
                    }
                    break;
                }

                case 'x':{
                    unsigned long val;
                    if(long_flag){
                        val = va_arg(args, unsigned long);
                    } else {
                        val = va_arg(args, uint32_t);
                    }
                    static const char H[] = "0123456789ABCDEF";
                    char b[14];
                    int i = 0;
                    b[i++] = '0'; b[i++] = 'x';
                    for (int s = 28; s >= 0; s -= 4)
                        b[i++] = H[(val >> s) & 0xF];
                    b[i] = '\0';
                    if (width > 0 && !left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
                    }
                    USART_puts(USARTX, b);
                    if (width > 0 && left_justify){
                        for (int j = i; j < width; j++)
                            USART_putc(USARTX, ' ');
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
                    if (!s) s = "(null)";
                    int len = 0;
                    while (s[len]) len++;
                    if (width > 0){
                        int trunc = (len < width) ? len : width;
                        if (!left_justify){
                            for (int i = trunc; i < width; i++)
                                USART_putc(USARTX, ' ');
                        }
                        for (int i = 0; i < trunc; i++)
                            USART_putc(USARTX, s[i]);
                        if (left_justify){
                            for (int i = trunc; i < width; i++)
                                USART_putc(USARTX, ' ');
                        }
                    } else {
                        USART_puts(USARTX, s);
                    }
                    break;
                }

                case 'f':{
                    double val = va_arg(args, double);
                    int neg = 0;
                    if (val < 0){
                        neg = 1;
                        val = -val;
                    }
                    if (!got_precision) precision = 6;
                    double rounding = 0.5;
                    for (int i = 0; i < precision; i++)
                        rounding /= 10.0;
                    val += rounding;

                    uint64_t int_part = (uint64_t)val;
                    double frac = val - (double)int_part;

                    uint64_t tmp = int_part;
                    int int_digits = 0;
                    do {
                        int_digits++;
                        tmp /= 10;
                    } while (tmp);

                    int total_len = (neg ? 1 : 0) + int_digits + 1 + precision;
                    if (width > 0 && !left_justify){
                        for (int i = total_len; i < width; i++)
                            USART_putc(USARTX, ' ');
                    }

                    if (neg) USART_putc(USARTX, '-');

                    char buf[24];
                    int bi = 0;
                    if (int_part == 0){
                        buf[bi++] = '0';
                    } else {
                        char r[24];
                        int ri = 0;
                        while (int_part){
                            r[ri++] = '0' + (int_part % 10);
                            int_part /= 10;
                        }
                        while (ri--) buf[bi++] = r[ri];
                    }
                    for (int i = 0; i < bi; i++)
                        USART_putc(USARTX, buf[i]);

                    USART_putc(USARTX, '.');

                    for (int i = 0; i < precision; i++){
                        frac *= 10.0;
                        int digit = (int)frac;
                        USART_putc(USARTX, '0' + digit);
                        frac -= digit;
                    }

                    if (width > 0 && left_justify){
                        for (int i = total_len; i < width; i++)
                            USART_putc(USARTX, ' ');
                    }
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

