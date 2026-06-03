#include "simple_xspi.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "stm32n657xx.h"

/* Helper macro: shift VAL into the FIELD position within REG */
#define XSPI_FIELD(REG, FIELD, VAL) \
    (((uint32_t)(VAL) << XSPI_##REG##_##FIELD##_Pos) & XSPI_##REG##_##FIELD##_Msk)

/*
 * XSPI1_GPIO_Init – XSPI1 (Port 1) → PSRAM (APS256XX)
 * Pins: GPIOO{0,2,3,4} = CLK, IO3, DQS0, NCS1
 *       GPIOP{0..15}   = IO0-IO15
 */
void XSPI1_GPIO_Init(void){
    RCC_enable_GPIO(GPIOO);
    RCC_enable_GPIO(GPIOP);

    GPIO_Config(GPIOO, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    for (int p = 2; p <= 4; p++) {
    	GPIO_Config(GPIOO, p, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    	GPIO_set_speed(GPIOO, p, GPIO_SPEED_VERY_HIGH);
    }

    GPIO_Config(GPIOP, 0, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
    for (int p = 1; p <= 15; p++){
        GPIO_Config(GPIOP, p, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
        GPIO_set_speed(GPIOP, p, GPIO_SPEED_VERY_HIGH);
    }
}

/*
 * XSPI2_GPIO_Init – XSPI2 (Port 2) → NOR Flash (MX66UW1G45G)
 * All pins on GPION{0..11} with AF9:
 *   PN0=DQS0, PN1=NCS1, PN2-5=IO0-3, PN6=CLK, PN7=NCLK, PN8-11=IO4-7
 */
void XSPI2_GPIO_Init(void){
    RCC_enable_GPIO(GPION);
    for (int p = 0; p <= 11; p++){
        GPIO_Config(GPION, p, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_PUPD_UP, XSPI_AF);
        GPIO_set_speed(GPION, p, GPIO_SPEED_VERY_HIGH);
    }
}

/* XSPI1 device-configuration registers: PSRAM (APS256XX) */
void XSPI1_Init(void){
    XSPI1->DCR1 = XSPI_FIELD(DCR1, MTYP,    6)
                | XSPI_FIELD(DCR1, DEVSIZE, 24)
                | XSPI_FIELD(DCR1, CSHT,    1);

    XSPI1->DCR2 = XSPI_FIELD(DCR2, PRESCALER, 3);  /* kernel / (3+1) */
    XSPI1->DCR3 = 0x000B0000;                       /* CS boundary */
    XSPI1->DCR4 = 129UL;                            /* refresh cycles */
    XSPI1->TCR  = XSPI_TCR_DHQC;                    /* DTR + high-performance config */
}

/* XSPI2 device-configuration registers: NOR Flash (MX66UW1G45G) */
void XSPI2_Init(void){
    XSPI2->DCR1 = XSPI_FIELD(DCR1, MTYP,    0x1)
                | XSPI_FIELD(DCR1, DEVSIZE, 0x1a)
                | XSPI_FIELD(DCR1, CSHT,    0x1);

    XSPI2->TCR  = XSPI_TCR_DHQC;
}

/*
 * XSPI_WriteReg – indirect register write for any XSPI instance
 * Sends 1 byte via octal DTR protocol: 8-bit cmd + 32-bit addr + 8-bit data
 */
static void XSPI_WriteReg(XSPI_TypeDef *xspi, uint8_t reg_addr, uint8_t value){
    uint32_t timeout;

    xspi->CR = XSPI_FIELD(CR, FTHRES, 7) | XSPI_CR_EN;

    timeout = 1000000;
    while (xspi->SR & XSPI_SR_BUSY){
        if (--timeout == 0) break;
    }

    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;

    xspi->CCR = XSPI_FIELD(CCR, IMODE,  7)
              | XSPI_FIELD(CCR, IDTR,   0)
              | XSPI_FIELD(CCR, ISIZE,  0)
              | XSPI_FIELD(CCR, ADMODE, 7)
              | XSPI_FIELD(CCR, ADDTR,  1)
              | XSPI_FIELD(CCR, ADSIZE, 3)
              | XSPI_FIELD(CCR, DMODE,  7)
              | XSPI_FIELD(CCR, DDTR,   1)
              | XSPI_FIELD(CCR, DQSE,   1);

    xspi->IR  = 0xC0;
    xspi->TCR = XSPI_TCR_DHQC;
    xspi->AR  = reg_addr;
    xspi->DLR = 1;

    *((__IO uint8_t*)&xspi->DR) = value;
    *((__IO uint8_t*)&xspi->DR) = 0x00;

    timeout = 1000000;
    while (!(xspi->SR & XSPI_SR_TCF)){
        if (--timeout == 0) break;
    }

    xspi->AR  = 0x0;
    xspi->FCR = XSPI_FCR_CTCF;
}

void XSPI1_WriteReg(uint8_t reg_addr, uint8_t value){
    XSPI_WriteReg(XSPI1, reg_addr, value);
}

void XSPI2_WriteReg(uint8_t reg_addr, uint8_t value){
    XSPI_WriteReg(XSPI2, reg_addr, value);
}


/* Write PSRAM (APS256XX) configuration registers */
static void PSRAM_WriteConfig(void){
    XSPI_WriteReg(XSPI1, 0, 0x30);
    XSPI_WriteReg(XSPI1, 4, 0x20);
    XSPI_WriteReg(XSPI1, 8, 0x40 | 0x03);
}

/* Write NOR Flash (MX66UW1G45G) configuration registers */
static void NOR_WriteConfig(void){
    XSPI_WriteReg(XSPI2, 0, 0x30);
    XSPI_WriteReg(XSPI2, 4, 0x20);
    XSPI_WriteReg(XSPI2, 8, 0x40 | 0x03);
}

static void XSPI_BypassPrescaler(XSPI_TypeDef *xspi){
    xspi->DCR2 = 0UL;
}

/*
 * XSPI1_EnableMemoryMappedMode – enable memory-mapped reads/writes
 * Configures read (CCR/IR/TCR) and write (WCCR/WIR/WTCR) channel opcodes
 * for octal-DTR protocol, then sets FMODE=3 (memory-mapped).
 */
void XSPI1_EnableMemoryMappedMode(void){
    XSPI1->WCCR = XSPI_FIELD(WCCR, IMODE,  4)
                | XSPI_FIELD(WCCR, IDTR,   0)
                | XSPI_FIELD(WCCR, ISIZE,  0)
                | XSPI_FIELD(WCCR, ADMODE, 4)
                | XSPI_FIELD(WCCR, ADDTR,  1)
                | XSPI_FIELD(WCCR, ADSIZE, 3)
                | XSPI_FIELD(WCCR, DMODE,  5)
                | XSPI_FIELD(WCCR, DDTR,   1)
                | XSPI_FIELD(WCCR, DQSE,   1);

    XSPI1->WIR  = 0xA0;
    XSPI1->WTCR = 6;

    XSPI1->CCR = XSPI_FIELD(CCR, IMODE,  4)
               | XSPI_FIELD(CCR, IDTR,   0)
               | XSPI_FIELD(CCR, ISIZE,  0)
               | XSPI_FIELD(CCR, ADMODE, 4)
               | XSPI_FIELD(CCR, ADDTR,  1)
               | XSPI_FIELD(CCR, ADSIZE, 3)
               | XSPI_FIELD(CCR, DMODE,  5)
               | XSPI_FIELD(CCR, DDTR,   1)
               | XSPI_FIELD(CCR, DQSE,   1);

    XSPI1->IR  = 0x20;
    XSPI1->TCR = 6;

    /* Enable XSPI1 in memory-mapped mode (FMODE=3), FIFO threshold=7 */
    XSPI1->CR = XSPI_FIELD(CR, FMODE, 3)
              | XSPI_FIELD(CR, FTHRES, 7)
              | XSPI_CR_EN;
}

/*
 * XSPI2_EnableMemoryMappedMode – enable memory-mapped mode for NOR Flash
 * Sets up automatic polling (PSMKR/PIR), read channel (CCR/IR/TCR),
 * write channel (WCCR/WIR), then switches to memory-mapped mode.
 */
void XSPI2_EnableMemoryMappedMode(void){
    /* Automatic polling: match PSMKR bit 0 to detect ready/busy */
    XSPI2->PSMKR = 0x1;
    XSPI2->PIR   = 0x10;             /* polling interval */

    /* Read channel: 16-bit instruction, 32-bit address, octal DTR with DQS */
    XSPI2->CCR = XSPI_FIELD(CCR, IMODE,  4)
               | XSPI_FIELD(CCR, IDTR,   1)
               | XSPI_FIELD(CCR, ISIZE,  1)
               | XSPI_FIELD(CCR, ADMODE, 4)
               | XSPI_FIELD(CCR, ADDTR,  1)
               | XSPI_FIELD(CCR, ADSIZE, 3)
			   | XSPI_FIELD(CCR, DMODE,  4)
               | XSPI_FIELD(CCR, DDTR,   1)
               | XSPI_FIELD(CCR, DQSE,   1);

    XSPI2->TCR = XSPI_FIELD(TCR, DCYC, 0xa);   /* 10 dummy cycles */
    XSPI2->IR  = 0xee11;                        /* read opcode */

    /* Write channel: 16-bit instruction, 32-bit address, octal DTR */
    XSPI2->WCCR = XSPI_FIELD(WCCR, IMODE,  4)
    		    | XSPI_FIELD(WCCR, IDTR,  1)
				| XSPI_FIELD(WCCR, ISIZE, 1)
				| XSPI_FIELD(WCCR, ADMODE, 4)
				| XSPI_FIELD(WCCR, ADDTR, 1)
				| XSPI_FIELD(WCCR, ADSIZE, 3)
				| XSPI_FIELD(WCCR, DMODE, 4)
				| XSPI_FIELD(WCCR, DDTR, 1);

    XSPI2->WIR  = 0x12ed;                       /* write/erase opcode */

    /* Enable with memory-mapped mode + auto-polling stop on match */
    XSPI2->CR = XSPI_FIELD(CR, FMODE, 3)
    		  | XSPI_FIELD(CR, APMS, 1)
              | XSPI_CR_EN;
}

/*
 * PSRAM_Init – full initialisation sequence for PSRAM on XSPI1
 *  1) Power up VDDIO2 at 1.8 V
 *  2) Enable/reset XSPI1, XSPI2, XSPIM clocks
 *  3) Configure XSPI1 GPIOs, device registers, PSRAM-specific regs
 *  4) Bypass prescaler and switch to memory-mapped mode
 */
void PSRAM_Init(void){
    RCC_enable_VDDIO2();
    RCC_config_VDDIO2_1V8();
    delay_ms(5);

    RCC_enable_XSPI1();
    RCC_enable_XSPI2();
    RCC_enable_XSPIM();

    // Sleep enable
    RCC->MISCLPENR |= RCC_MISCLPENR_XSPIPHYCOMPLPEN;

    RCC->AHB5LPENR |=
    		RCC_AHB5ENR_XSPI1EN
			| RCC_AHB5ENR_XSPI2EN
			| RCC_AHB5ENR_XSPIMEN
			| RCC_AHB5ENR_XSPI3EN;

    delay_ms(1);

    RCC_reset_XSPI1();
    RCC_reset_XSPI2();
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

    XSPI_BypassPrescaler(XSPI1);
    delay_ms(1);

    XSPI1_EnableMemoryMappedMode();
    delay_ms(1);
}

/*
 * NOR_Init – full initialisation sequence for NOR Flash on XSPI2
 * Mirrors PSRAM_Init but targets XSPI2 with NOR-specific register writes
 * and auto-polling memory-mapped mode.
 */
void NOR_Init(void){
    RCC_enable_VDDIO2();
    RCC_config_VDDIO2_1V8();
    delay_ms(5);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();

    RCC->MISCLPENR |= RCC_MISCLPENR_XSPIPHYCOMPLPEN;

    RCC->AHB5LPENR |= RCC_AHB5ENR_XSPI2EN
                    | RCC_AHB5ENR_XSPIMEN;

    delay_ms(1);

    RCC_reset_XSPI2();
    RCC_reset_XSPIM();
    delay_ms(1);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();
    delay_ms(1);

    XSPI2_GPIO_Init();
    delay_ms(1);

    XSPI2_Init();
    delay_ms(1);

    NOR_WriteConfig();
    delay_ms(1);

    XSPI_BypassPrescaler(XSPI2);
    delay_ms(1);

    XSPI2_EnableMemoryMappedMode();
    delay_ms(1);
}
