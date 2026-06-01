#include "simple_xspi.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "stm32n657xx.h"

void XSPI1_GPIO_Init(void){
    RCC_enable_GPIO(GPIOO);
    RCC_enable_GPIO(GPIOP);

    GPIO_Config(GPIOO, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    GPIO_Config(GPIOO, 2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    GPIO_Config(GPIOO, 3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    GPIO_Config(GPIOO, 4, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);

    GPIO_Config(GPIOP, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    for (int p = 1; p <= 15; p++){
        GPIO_Config(GPIOP, p, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    }
}

void XSPI1_Init(void){
    XSPI1->DCR1 = (6UL << 24) | (24UL << 16) | (4UL << 8);
    XSPI1->DCR2 = 3UL;
    XSPI1->DCR3 = 0x000B0000;
    XSPI1->DCR4 = 129UL;
    XSPI1->TCR  = (1UL << 28);
}

void XSPI1_WriteReg(uint8_t reg_addr, uint8_t value){
    uint32_t timeout;

    XSPI1->CR = (7UL << 8) | 1UL;

    timeout = 1000000;
    while (XSPI1->SR & XSPI_SR_BUSY){
        if (--timeout == 0) break;
    }

    XSPI1->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;

    XSPI1->CCR = (4UL << 0)
               | (0UL << 3)
               | (0UL << 4)
               | (4UL << 8)
               | (1UL << 11)
               | (3UL << 12)
               | (5UL << 24)
               | (1UL << 27)
               ;

    XSPI1->IR  = 0xC0;
    XSPI1->TCR = (1UL << 28);
    XSPI1->AR  = reg_addr;
    XSPI1->DLR = 1;

    *((__IO uint8_t*)&XSPI1->DR) = value;
    *((__IO uint8_t*)&XSPI1->DR) = 0x00;

    timeout = 1000000;
    while (!(XSPI1->SR & XSPI_SR_TCF)){
        if (--timeout == 0) break;
    }

    XSPI1->FCR = XSPI_FCR_CTCF;
}

static void PSRAM_WriteConfig(void){
    XSPI1_WriteReg(0, 0x30);
    XSPI1_WriteReg(4, 0x20);
    XSPI1_WriteReg(8, 0x40 | 0x03);
}

static void PSRAM_BypassPrescaler(void){
    XSPI1->DCR2 = 0UL;
}

void XSPI1_EnableMemoryMappedMode(void){
    XSPI1->WCCR = (4UL << 0)
                 | (0UL << 3)
                 | (0UL << 4)
                 | (4UL << 8)
                 | (1UL << 11)
                 | (3UL << 12)
                 | (5UL << 24)
                 | (1UL << 27)
                 | (1UL << 29)
                 ;

    XSPI1->WIR  = 0xA0;
    XSPI1->WTCR = 6;

    XSPI1->CCR = (4UL << 0)
                | (0UL << 3)
                | (0UL << 4)
                | (4UL << 8)
                | (1UL << 11)
                | (3UL << 12)
                | (5UL << 24)
                | (1UL << 27)
                | (1UL << 29)
                ;

    XSPI1->IR  = 0x20;
    XSPI1->TCR = 6;

    XSPI1->CR = 0x30000000 | (7UL << 8) | 1UL;
}

void PSRAM_Init(void){
    RCC_enable_VDDIO2();
    RCC_config_VDDIO2_1V8();
    delay_ms(5);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    delay_ms(1);

    RCC_reset_XSPI1();
    RCC_reset_XSPIM();
    delay_ms(1);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    delay_ms(1);

    XSPI1_GPIO_Init();
    delay_ms(1);

    XSPI1_Init();
    delay_ms(1);

    PSRAM_WriteConfig();
    delay_ms(1);

    PSRAM_BypassPrescaler();
    delay_ms(1);

    XSPI1_EnableMemoryMappedMode();
    delay_ms(1);
}
