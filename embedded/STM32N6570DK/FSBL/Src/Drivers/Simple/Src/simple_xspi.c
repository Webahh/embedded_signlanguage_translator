#include <stdint.h>

#include "simple_xspi.h"

#include "stm32n657xx.h"

#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "config.h"

#define _TIMEOUT                    1000000
#define _FIFO_THRESHOLD             7
#define _CLEAR_ADDRESS              0
#define _MM_APMS                    1U
#define _MM_FMODE                   3

#define _PSRAM_MR0_ADDR             0x00
#define _PSRAM_MR4_ADDR             0x04
#define _PSRAM_MR8_ADDR             0x08
#define _PSRAM_MR0_VALUE            0x30
#define _PSRAM_MR4_VALUE            0x20
#define _PSRAM_MR8_VALUE            0x40
#define _PSRAM_REG_WRITE_DLR        0x1
#define _PSRAM_REG_WRITE_DUMMY_BYTE 0x0
#define _PSRAM_REG_WRITE_CMD        0xC0
#define _PSRAM_CMD_LINEAR_BURST_READ   0x20
#define _PSRAM_CMD_LINEAR_BURST_WRITE  0xA0
#define _PSRAM_READ_DUMMY_CYCLES    6
#define _PSRAM_WRITE_DUMMY_CYCLES   6

#define _NOR_PSMKR_READY_MASK       0x1U
#define _NOR_POLLING_INTERVAL       0x10U
#define _NOR_READ_OPCODE            0xEE11U
#define _NOR_READ_DUMMY_CYCLES      0x0AU
#define _NOR_WRITE_OPCODE           0x12EDU

#define _FIELD(REG, FIELD, VAL) \
    (((uint32_t)(VAL) << XSPI_##REG##_##FIELD##_Pos) & XSPI_##REG##_##FIELD##_Msk)

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

static void XSPI_prescaler_set(XSPI_TypeDef *xspi, uint32_t prescaler){
    uint32_t timeout = _TIMEOUT;

    while (xspi->SR & XSPI_SR_BUSY){
        if (--timeout == 0) break;
    }

    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;

    // Set prescaler
    xspi->DCR2 &= ~XSPI_DCR2_PRESCALER_Msk;
    xspi->DCR2 |= _FIELD(DCR2, PRESCALER, prescaler);

    // Timeout guard
    timeout = _TIMEOUT;
    while (xspi->SR & XSPI_SR_BUSY){
        if (--timeout == 0) break;
    }

    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;
}

/*
 * XSPI1 (Port 1) → PSRAM (APS256XX)
 * Pins: GPIOO{0,2,3,4} = CLK, IO3, DQS0, NCS1
 *       GPIOP{0..15}   = IO0-IO15
 */
static void XSPI_PSRAM_gpio_init(void){
    RCC_enable_GPIO(GPIOO);
    RCC_enable_GPIO(GPIOP);

    GPIO_Config(GPIOO, 0, GPIO_XSPI_cfg);
    for (int p = 2; p <= 4; p++){
        GPIO_Config(GPIOO, p, GPIO_XSPI_cfg);
    }

    GPIO_Config(GPIOP, 0, GPIO_XSPI_cfg);
    for (int p = 1; p <= 15; p++){
        GPIO_Config(GPIOP, p, GPIO_XSPI_cfg);
    }
}

/*
 * XSPI2 (Port 2) → NOR Flash (MX66UW1G45G)
 * All pins on GPION{0..11} with AF9:
 *   PN0=DQS0, PN1=NCS1, PN2-5=IO0-3, PN6=CLK, PN7=NCLK, PN8-11=IO4-7
 */
static void XSPI_NOR_gpio_init(void){
    RCC_enable_GPIO(GPION);
    for (int p = 0; p <= 11; p++){
        GPIO_Config(GPION, p, GPIO_XSPI_cfg);
    }
}

static void XSPI_device_init(XSPI_TypeDef *xspi, XSPI_cfg_TypeDef cfg){
    xspi->DCR1 = _FIELD(DCR1, MTYP,     cfg.memory_type)
               | _FIELD(DCR1, DEVSIZE,  cfg.devsize)
               | _FIELD(DCR1, CSHT,     cfg.chipselect_high_time);
    xspi->DCR2 = _FIELD(DCR2, PRESCALER, cfg.prescaler);
    xspi->DCR3 = _FIELD(DCR3, MAXTRAN,  cfg.maxtran_value)
               | _FIELD(DCR3, CSBOUND,  cfg.chipselect_boundary);
    xspi->DCR4 = cfg.refresh_cycles;

    xspi->CR  = _FIELD(CR, FTHRES, _FIFO_THRESHOLD);
    xspi->TCR = 0;
}

static void XSPI_ccr_build(const XSPI_CCR_cfg_TypeDef cfg, uint32_t *result){
    *result = _FIELD(CCR, IMODE,  cfg.instruction_mode)
            | _FIELD(CCR, IDTR,   cfg.instruction_dtr)
            | _FIELD(CCR, ISIZE,  cfg.instruction_size)
            | _FIELD(CCR, ADMODE, cfg.address_mode)
            | _FIELD(CCR, ADDTR,  cfg.address_dtr)
            | _FIELD(CCR, ADSIZE, cfg.address_size)
            | _FIELD(CCR, DMODE,  cfg.data_mode)
            | _FIELD(CCR, DDTR,   cfg.data_dtr)
            | _FIELD(CCR, DQSE,   cfg.data_qse);
}

/*
 * Indirect register write for any XSPI instance.
 * Sends 1 byte via octal DTR protocol: 8-bit cmd + 32-bit addr + 8-bit data
 */
static XSPI_Status_TypeDef XSPI_register_write(XSPI_TypeDef *xspi, uint8_t reg_addr, uint8_t value){
    uint32_t timeout;
    uint32_t ccr_val;

    xspi->CR = _FIELD(CR, FTHRES, _FIFO_THRESHOLD)
             | XSPI_CR_EN;

    timeout = _TIMEOUT;
    while (xspi->SR & XSPI_SR_BUSY){
        if (--timeout == 0) return XSPI_TIMEOUT;
    }

    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;
    xspi->TCR = 0;

    XSPI_ccr_build(XSPI_write_reg_cfg, &ccr_val);
    xspi->CCR = ccr_val;

    xspi->IR  = _PSRAM_REG_WRITE_CMD;
    xspi->AR  = reg_addr;
    xspi->DLR = _PSRAM_REG_WRITE_DLR;

    *((__IO uint8_t*)&xspi->DR) = value;
    *((__IO uint8_t*)&xspi->DR) = _PSRAM_REG_WRITE_DUMMY_BYTE;

    timeout = _TIMEOUT;
    while (!(xspi->SR & XSPI_SR_TCF)){
        if (--timeout == 0) return XSPI_TIMEOUT;
    }

    xspi->AR  = _CLEAR_ADDRESS;
    xspi->FCR = XSPI_FCR_CTCF;

    return XSPI_OK;
}

static void XSPI_PSRAM_config_write(void){
    XSPI_register_write(XSPI1, _PSRAM_MR0_ADDR, _PSRAM_MR0_VALUE);
    XSPI_register_write(XSPI1, _PSRAM_MR4_ADDR, _PSRAM_MR4_VALUE);
    XSPI_register_write(XSPI1, _PSRAM_MR8_ADDR, _PSRAM_MR8_VALUE);
}

static void XSPI_NOR_config_write(void){
    XSPI_register_write(XSPI2, _PSRAM_MR0_ADDR, _PSRAM_MR0_VALUE);
    XSPI_register_write(XSPI2, _PSRAM_MR4_ADDR, _PSRAM_MR4_VALUE);
    XSPI_register_write(XSPI2, _PSRAM_MR8_ADDR, _PSRAM_MR8_VALUE);
}

/*
 * Enable memory-mapped reads/writes on XSPI1 (PSRAM).
 * Configures read (CCR/IR/TCR) and write (WCCR/WIR/WTCR) channel opcodes
 * for octal-DTR protocol, then sets FMODE=3 (memory-mapped).
 */
static void XSPI_PSRAM_memoryMapped_enable(void){
    uint32_t ccr_val;

    XSPI_ccr_build(XSPI_memorymapped_cfg, &ccr_val);
    XSPI1->WCCR = ccr_val;

    XSPI1->WIR  = _PSRAM_CMD_LINEAR_BURST_WRITE;
    XSPI1->WTCR = _FIELD(WTCR, DCYC, _PSRAM_WRITE_DUMMY_CYCLES);

    XSPI_ccr_build(XSPI_memorymapped_cfg, &ccr_val);
    XSPI1->CCR  = ccr_val;

    XSPI1->IR   = _PSRAM_CMD_LINEAR_BURST_READ;
    XSPI1->TCR  = _FIELD(TCR, DCYC, _PSRAM_READ_DUMMY_CYCLES);

    XSPI1->CR = _FIELD(CR, FMODE,  _MM_FMODE)
              | _FIELD(CR, FTHRES, _FIFO_THRESHOLD)
              | XSPI_CR_EN;
}

/*
 * Enable memory-mapped mode for NOR Flash on XSPI2.
 * Sets up automatic polling (PSMKR/PIR), read channel (CCR/IR/TCR),
 * write channel (WCCR/WIR), then switches to memory-mapped mode.
 */
static void XSPI_NOR_memoryMapped_enable(void){
    uint32_t ccr_val;

    XSPI2->PSMKR = _NOR_PSMKR_READY_MASK;
    XSPI2->PIR   = _NOR_POLLING_INTERVAL;

    XSPI2->WIR   = _NOR_WRITE_OPCODE;

    XSPI_ccr_build(XSPI_memorymapped_cfg, &ccr_val);
    XSPI2->WCCR  = ccr_val;

    XSPI_ccr_build(XSPI_memorymapped_cfg, &ccr_val);
    XSPI2->CCR   = ccr_val;

    XSPI2->TCR  = XSPI_TCR_DHQC
                | _FIELD(TCR, DCYC, _PSRAM_READ_DUMMY_CYCLES);
    XSPI2->IR   = _NOR_READ_OPCODE;

    XSPI2->CR = _FIELD(CR, FMODE, _MM_FMODE)
              | _FIELD(CR, APMS,  _MM_APMS)
              | XSPI_CR_EN;
}

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * @brief Initialise PSRAM device on XSPI1
 *
 * Full sequence: clock enable/reset, GPIO config, device register setup,
 * PSRAM-specific register writes, prescaler bypass, memory-mapped mode.
 *
 * @param [in] init_cfg | XSPI configuration parameters
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   Initialisation failed
 */
XSPI_Status_TypeDef XSPI_PSRAM_init(XSPI_cfg_TypeDef init_cfg){
    RCC_setXSPI1_clock_source(0);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    delay_ms(1);

    RCC_reset_XSPI1();
    RCC_reset_XSPIM();
    delay_ms(1);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    delay_ms(1);

    XSPI_PSRAM_gpio_init();
    delay_ms(1);

    XSPI_device_init(XSPI1, init_cfg);
    delay_ms(1);

    XSPI_PSRAM_config_write();
    delay_ms(1);

    XSPI_prescaler_set(XSPI1, 0);
    delay_ms(1);

    XSPI_PSRAM_memoryMapped_enable();
    delay_ms(1);

    return XSPI_OK;
}

/**
 * @brief Initialise NOR Flash device on XSPI2
 *
 * Full sequence: clock enable/reset, GPIO config, device register setup,
 * NOR-specific register writes, memory-mapped mode with auto-polling.
 *
 * @param [in] init_cfg | XSPI configuration parameters
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   Initialisation failed
 *
 * @note Not yet fully supported – placeholder implementation.
 */
XSPI_Status_TypeDef XSPI_NOR_init(XSPI_cfg_TypeDef init_cfg){
    delay_ms(5);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();
    delay_ms(1);

    RCC_reset_XSPI2();
    RCC_reset_XSPIM();
    delay_ms(1);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();
    delay_ms(1);

    XSPI_NOR_gpio_init();
    delay_ms(1);

    XSPI_device_init(XSPI2, init_cfg);
    delay_ms(1);

    XSPI_NOR_config_write();
    delay_ms(1);

    XSPI_NOR_memoryMapped_enable();
    delay_ms(1);

    return XSPI_OK;
}
