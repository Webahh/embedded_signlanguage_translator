#include "simple_xspi.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "stm32n657xx.h"
#include "config.h"

#define XSPI_REG_WRITE_TIMEOUT              1000000
#define XSPI_REG_WRITE_FIFO_THRESHOLD       7
#define XSPI_REG_WRITE_OPCODE               0xC0
#define XSPI_REG_WRITE_DUMMY_BYTE           0x00
#define XSPI_REG_WRITE_DATA_LENGTH_BYTE     1
#define XSPI_REG_WRITE_CLEAR_ADDRESS        0

#define XSPI_EXT_REG_CONFIG0_ADDR        	0x00
#define XSPI_EXT_REG_CONFIG1_ADDR        	0x04
#define XSPI_EXT_REG_CONFIG2_ADDR        	0x08
#define XSPI_EXT_REG_CONFIG0_VALUE       	0x30
#define XSPI_EXT_REG_CONFIG1_VALUE       	0x20
#define XSPI_EXT_REG_CONFIG2_BASE        	0x40
#define XSPI_EXT_REG_CONFIG2_LATENCY     	0x03
#define XSPI_EXT_REG_CONFIG2_VALUE       	0x40

#define XSPI_MEMORY_MAPPED_APMS             1U
#define XSPI_MEMORY_MAPPED_FMODE            3
#define XSPI_MEMORY_MAPPED_FIFO_THRESHOLD   7
#define XSPI1_PSRAM_READ_OPCODE             0x20
#define XSPI1_PSRAM_WRITE_OPCODE            0xA0
#define XSPI1_PSRAM_READ_DUMMY_CYCLES       6
#define XSPI1_PSRAM_WRITE_DUMMY_CYCLES      6
#define XSPI2_NOR_PSMKR_READY_MASK          0x1U
#define XSPI2_NOR_POLLING_INTERVAL          0x10U
#define XSPI2_NOR_READ_OPCODE               0xEE11U
#define XSPI2_NOR_READ_DUMMY_CYCLES         0x0AU
#define XSPI2_NOR_WRITE_OPCODE              0x12EDU
#define XSPI_MEMORY_MAPPED_APMS             1U

/* Helper macro: shift VAL into the FIELD position within REG */
#define XSPI_FIELD(REG, FIELD, VAL) \
    (((uint32_t)(VAL) << XSPI_##REG##_##FIELD##_Pos) & XSPI_##REG##_##FIELD##_Msk)


static int XSPI_SetPrescaler_Calibrated(XSPI_TypeDef *xspi, uint32_t prescaler){
    uint32_t timeout;

    timeout = XSPI_REG_WRITE_TIMEOUT;
    while (xspi->SR & XSPI_SR_BUSY) {
        if (--timeout == 0U) {
            return -1;
        }
    }


    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;

    xspi->DCR2 &= ~XSPI_DCR2_PRESCALER_Msk;
    xspi->DCR2 |= XSPI_FIELD(DCR2, PRESCALER, prescaler);

    timeout = XSPI_REG_WRITE_TIMEOUT;
    while (xspi->SR & XSPI_SR_BUSY) {
        if (--timeout == 0U) {
            return -2;
        }
    }

    /* Nach Calibration Fehlerflags löschen */
    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;

    return 0;
}

/*
 * XSPI1_GPIO_Init – XSPI1 (Port 1) → PSRAM (APS256XX)
 * Pins: GPIOO{0,2,3,4} = CLK, IO3, DQS0, NCS1
 *       GPIOP{0..15}   = IO0-IO15
 */
static void XSPI1_GPIO_Init(void){
    RCC_enable_GPIO(GPIOO);
    RCC_enable_GPIO(GPIOP);

    GPIO_Config(GPIOO, 0, GPIO_XSPI_cfg);
    for (int p = 2; p <= 4; p++) {
    	GPIO_Config(GPIOO, p, GPIO_XSPI_cfg);
    }

    GPIO_Config(GPIOP, 0, GPIO_XSPI_cfg);
    for (int p = 1; p <= 15; p++){
        GPIO_Config(GPIOP, p, GPIO_XSPI_cfg);
    }
}

/*
 * XSPI2_GPIO_Init – XSPI2 (Port 2) → NOR Flash (MX66UW1G45G)
 * All pins on GPION{0..11} with AF9:
 *   PN0=DQS0, PN1=NCS1, PN2-5=IO0-3, PN6=CLK, PN7=NCLK, PN8-11=IO4-7
 */
static void XSPI2_GPIO_Init(void){
    RCC_enable_GPIO(GPION);
    for (int p = 0; p <= 11; p++){
        GPIO_Config(GPION, p, GPIO_XSPI_cfg);
    }
}

static void XSPI_Init(XSPI_TypeDef *XSPIX , XSPI_cfg_TypeDef cfg){
	XSPI_TypeDef *xspi_ptr;

	if(XSPIX == XSPI1){
		xspi_ptr = XSPI1;
	} else if (XSPIX == XSPI2) {
		xspi_ptr = XSPI2;
	} else {
		return;
	}

    xspi_ptr->DCR1 = XSPI_FIELD(DCR1, MTYP,    	 cfg.memory_type)
                   | XSPI_FIELD(DCR1, DEVSIZE, 	 cfg.devsize)
                   | XSPI_FIELD(DCR1, CSHT,    	 cfg.chipselect_high_time);

    xspi_ptr->DCR2 = 0;

    delay_ms(10);

    xspi_ptr->DCR3 = XSPI_FIELD(DCR3, MAXTRAN, 	 cfg.maxtran_value)
				   | XSPI_FIELD(DCR3, CSBOUND,	 cfg.chipselect_boundary);

    xspi_ptr->DCR4 = cfg.refresh_cycles;

    xspi_ptr->CR = XSPI_FIELD(CR, FTHRES, 7);

    while (xspi_ptr->SR & XSPI_SR_BUSY) { }

    xspi_ptr->DCR2 = XSPI_FIELD(DCR2, PRESCALER, cfg.prescaler);

    while (xspi_ptr->SR & XSPI_SR_BUSY) { }

    xspi_ptr->TCR = 0;

}

static uint32_t XSPI_BuildWCCR_CCR(const XSPI_ccr_cfg_TypeDef cfg){
    return XSPI_FIELD(CCR, IMODE,  cfg.instruction_mode)
         | XSPI_FIELD(CCR, IDTR,   cfg.instruction_dtr)
         | XSPI_FIELD(CCR, ISIZE,  cfg.instruction_size)
         | XSPI_FIELD(CCR, ADMODE, cfg.address_mode)
         | XSPI_FIELD(CCR, ADDTR,  cfg.address_dtr)
         | XSPI_FIELD(CCR, ADSIZE, cfg.address_size)
         | XSPI_FIELD(CCR, DMODE,  cfg.data_mode)
         | XSPI_FIELD(CCR, DDTR,   cfg.data_dtr)
         | XSPI_FIELD(CCR, DQSE,   cfg.data_qse);
}

static uint32_t XSPI_BuildWCCR_CCR_NoDQS(const XSPI_ccr_cfg_TypeDef cfg)
{
    return XSPI_FIELD(CCR, IMODE,  cfg.instruction_mode)
         | XSPI_FIELD(CCR, IDTR,   cfg.instruction_dtr)
         | XSPI_FIELD(CCR, ISIZE,  cfg.instruction_size)
         | XSPI_FIELD(CCR, ADMODE, cfg.address_mode)
         | XSPI_FIELD(CCR, ADDTR,  cfg.address_dtr)
         | XSPI_FIELD(CCR, ADSIZE, cfg.address_size)
         | XSPI_FIELD(CCR, DMODE,  cfg.data_mode)
         | XSPI_FIELD(CCR, DDTR,   cfg.data_dtr);
         /* bewusst kein DQSE */
}

/*
 * XSPI_WriteReg – indirect register write for any XSPI instance
 * Sends 1 byte via octal DTR protocol: 8-bit cmd + 32-bit addr + 8-bit data
 */
static void XSPI_WriteReg(XSPI_TypeDef *xspi, uint8_t reg_addr, uint8_t value){
    uint32_t timeout;

    xspi->CR = XSPI_FIELD(CR, FTHRES, XSPI_REG_WRITE_FIFO_THRESHOLD)
    		 | XSPI_CR_EN;

    timeout = XSPI_REG_WRITE_TIMEOUT;
    while (xspi->SR & XSPI_SR_BUSY){
        if (--timeout == 0) break;
    }

    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;
    xspi->TCR = 0;
    xspi->CCR = XSPI_BuildWCCR_CCR_NoDQS(XSPI_write_reg_cfg);

    xspi->IR  = XSPI_REG_WRITE_OPCODE;
    xspi->AR  = reg_addr;
    xspi->DLR = XSPI_REG_WRITE_DATA_LENGTH_BYTE;

    *((__IO uint8_t*)&xspi->DR) = value;
    *((__IO uint8_t*)&xspi->DR) = XSPI_REG_WRITE_DUMMY_BYTE;

    timeout = XSPI_REG_WRITE_TIMEOUT;
    while (!(xspi->SR & XSPI_SR_TCF)){
        if (--timeout == 0) break;
    }

    xspi->AR  = XSPI_REG_WRITE_CLEAR_ADDRESS;
    xspi->FCR = XSPI_FCR_CTCF;
}

/* Write PSRAM (APS256XX) configuration registers */
static void PSRAM_WriteConfig(void){
    XSPI_WriteReg(XSPI1, XSPI_EXT_REG_CONFIG0_ADDR, XSPI_EXT_REG_CONFIG0_VALUE);
    XSPI_WriteReg(XSPI1, XSPI_EXT_REG_CONFIG1_ADDR, XSPI_EXT_REG_CONFIG1_VALUE);
    XSPI_WriteReg(XSPI1, XSPI_EXT_REG_CONFIG2_ADDR, XSPI_EXT_REG_CONFIG2_VALUE);
}

/* Write NOR Flash (MX66UW1G45G) configuration registers */
static void NOR_WriteConfig(void){
    XSPI_WriteReg(XSPI2, XSPI_EXT_REG_CONFIG0_ADDR, XSPI_EXT_REG_CONFIG0_VALUE);
    XSPI_WriteReg(XSPI2, XSPI_EXT_REG_CONFIG1_ADDR, XSPI_EXT_REG_CONFIG1_VALUE);
    XSPI_WriteReg(XSPI2, XSPI_EXT_REG_CONFIG2_ADDR, XSPI_EXT_REG_CONFIG2_VALUE);
}

/*
 * XSPI1_EnableMemoryMappedMode – enable memory-mapped reads/writes
 * Configures read (CCR/IR/TCR) and write (WCCR/WIR/WTCR) channel opcodes
 * for octal-DTR protocol, then sets FMODE=3 (memory-mapped).
 */

static void XSPI1_EnableMemoryMappedMode(void){
    XSPI1->WCCR = XSPI_BuildWCCR_CCR(XSPI_memorymapped_cfg);

    XSPI1->WIR  = XSPI1_PSRAM_WRITE_OPCODE;
    XSPI1->WTCR = XSPI_FIELD(WTCR, DCYC, XSPI1_PSRAM_WRITE_DUMMY_CYCLES);

    XSPI1->CCR  = XSPI_BuildWCCR_CCR(XSPI_memorymapped_cfg);

    XSPI1->IR   = XSPI1_PSRAM_READ_OPCODE;
    XSPI1->TCR  = XSPI_FIELD(TCR, DCYC, XSPI1_PSRAM_READ_DUMMY_CYCLES);

    /* Enable XSPI1 in memory-mapped mode (FMODE=3), FIFO threshold=7 */
    XSPI1->CR = XSPI_FIELD(CR, FMODE,  XSPI_MEMORY_MAPPED_FMODE)
              | XSPI_FIELD(CR, FTHRES, XSPI_MEMORY_MAPPED_FIFO_THRESHOLD)
              | XSPI_CR_EN;
}

/*
 * XSPI2_EnableMemoryMappedMode – enable memory-mapped mode for NOR Flash
 * Sets up automatic polling (PSMKR/PIR), read channel (CCR/IR/TCR),
 * write channel (WCCR/WIR), then switches to memory-mapped mode.
 */
static void XSPI2_EnableMemoryMappedMode(void){
    /* Automatic polling: match PSMKR bit 0 to detect ready/busy */
    XSPI2->PSMKR = XSPI2_NOR_PSMKR_READY_MASK;
    XSPI2->PIR   = XSPI2_NOR_POLLING_INTERVAL;             /* polling interval */

    XSPI2->WIR   = XSPI2_NOR_WRITE_OPCODE;
    XSPI2->WCCR  = XSPI_BuildWCCR_CCR(XSPI_memorymapped_cfg);

    XSPI2->CCR = XSPI_BuildWCCR_CCR(XSPI_memorymapped_cfg);

    XSPI2->TCR  = XSPI_TCR_DHQC
                | XSPI_FIELD(TCR, DCYC, XSPI1_PSRAM_READ_DUMMY_CYCLES);
    XSPI2->IR  = XSPI2_NOR_READ_OPCODE;                        /* read opcode */

    /* Write channel: 16-bit instruction, 32-bit address, octal DTR */
                          /* write/erase opcode */

    /* Enable with memory-mapped mode + auto-polling stop on match */
    XSPI2->CR = XSPI_FIELD(CR, FMODE, XSPI_MEMORY_MAPPED_FMODE)
    		  | XSPI_FIELD(CR, APMS, XSPI_MEMORY_MAPPED_APMS)
              | XSPI_CR_EN;
}

/*
 * PSRAM_Init – full initialisation sequence for PSRAM on XSPI1
 *  1) Power up VDDIO2 at 1.8 V
 *  2) Enable/reset XSPI1, XSPIM clocks
 *  3) Configure XSPI1 GPIOs, device registers, PSRAM-specific regs
 */
void PSRAM_Init(XSPI_cfg_TypeDef init_cfg){
    RCC_enable_VDDIO2();
    RCC_config_VDDIO2_1V8();
    delay_ms(5);

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

    XSPI1_GPIO_Init();
    delay_ms(1);

    XSPI_Init(XSPI1, init_cfg);
    delay_ms(1);

    PSRAM_WriteConfig();
    delay_ms(1);

    if (XSPI_SetPrescaler_Calibrated(XSPI1, 1) != 0) {
        while (1) {
            /* Calibration failed */
        }
    }
    delay_ms(10);

    XSPI1_EnableMemoryMappedMode();
    delay_ms(1);
}

/*
 * NOR_Init – full initialisation sequence for NOR Flash on XSPI2
 * Mirrors PSRAM_Init but targets XSPI2 with NOR-specific register writes
 * and auto-polling memory-mapped mode.
 */
void NOR_Init(XSPI_cfg_TypeDef init_cfg){
    RCC_enable_VDDIO2();
    RCC_config_VDDIO2_1V8();
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

    XSPI2_GPIO_Init();
    delay_ms(1);

    XSPI_Init(XSPI2, init_cfg);
    delay_ms(1);

    NOR_WriteConfig();
    delay_ms(1);

    XSPI2_EnableMemoryMappedMode();
    delay_ms(1);
}
