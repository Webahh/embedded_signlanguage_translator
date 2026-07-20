/**
 * @file    simple_xspi.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   Bare-metal XSPI driver for APS256XX PSRAM and MX66UW1G45G NOR
 */

#include <stdint.h>

#include "stm32n657xx.h"

#include "simple_xspi.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_timer.h"
#include "config.h"

#define _XSPI_TIMEOUT                   1000000
#define _XSPI_FIFO_THRESHOLD            0
#define _XSPI_CLEAR_ADDRESS             0

#define _XSPI_FMODE_INDIRECT_WRITE   	0
#define _XSPI_FMODE_INDIRECT_READ    	1
#define _XSPI_FMODE_MEMORY_MAPPED    	3

#define _PSRAM_MR0_ADDRESS             	0x00
#define _PSRAM_MR4_ADDRESS             	0x04
#define _PSRAM_MR8_ADDRESS             	0x08
#define _PSRAM_MR0_VALUE            	0x30
#define _PSRAM_MR4_VALUE            	0x20
#define _PSRAM_MR8_VALUE            	0x40

#define _PSRAM_REGISTER_WRITE_CMD    	0xC0
#define _PSRAM_REGISTER_WRITE_DLR     	0x01
#define _PSRAM_REGISTER_DUMMY_BYTE    	0x00

#define _PSRAM_LINEAR_READ_CMD        	0x20
#define _PSRAM_LINEAR_WRITE_CMD       	0xA0
#define _PSRAM_READ_DUMMY_CYCLES    	6
#define _PSRAM_WRITE_DUMMY_CYCLES   	6

#define _NOR_RESET_ENABLE_CMD         	0x66
#define _NOR_RESET_MEMORY_CMD         	0x99
#define _NOR_READ_ID_CMD              	0x9F
#define _NOR_READ_STATUS_CMD          	0x05
#define _NOR_WRITE_ENABLE_CMD         	0x06
#define _NOR_WRITE_CFG_REG2_CMD       	0x72
#define _NOR_OCTA_READ_STATUS_CMD     	0x05FA
#define _NOR_OCTA_READ_DTR_CMD        	0xEE11
#define _NOR_OCTA_PAGE_PROGRAM_CMD    	0x12ED
#define _NOR_CR2_PROTOCOL_ADDRESS     	0x00000000
#define _NOR_CR2_DOPI_VALUE           	0x02
#define _NOR_ID_SIZE                 	3
#define _NOR_DTR_STATUS_SIZE          	2
#define _NOR_STATUS_WIP               	0x01
#define _NOR_STATUS_WEL               	0x02
#define _NOR_MANUFACTURER_ID          	0xC2
#define _NOR_MEMORY_TYPE_ID           	0x81
#define _NOR_MEMORY_DENSITY_ID        	0x3B
#define _NOR_REGISTER_DTR_DUMMY       	5
#define _NOR_READ_DTR_DUMMY           	10

#define _XSPI_FIELD(REG, FIELD, VAL) \
    (((uint32_t)(VAL) << XSPI_##REG##_##FIELD##_Pos) & XSPI_##REG##_##FIELD##_Msk)

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

/**
 * @brief Wait until the specified XSPI peripheral is no longer busy
 *
 * @param[in] xspi Pointer to the XSPI peripheral instance
 *
 * @retval XSPI_OK      Peripheral is ready
 * @retval XSPI_TIMEOUT Timeout while waiting for the peripheral
 */
static XSPI_Status_TypeDef _XSPI_wait_not_busy(XSPI_TypeDef *xspi)
{
    uint32_t timeout = _XSPI_TIMEOUT;

    while ((xspi->SR & XSPI_SR_BUSY) != 0U) {
        if (--timeout == 0U) {
            return XSPI_TIMEOUT;
        }
    }
    return XSPI_OK;
}

/**
 * @brief Wait for completion of the current XSPI transfer
 *
 * @param[in] xspi Pointer to the XSPI peripheral instance
 *
 * @retval XSPI_OK      Transfer completed successfully
 * @retval XSPI_ERROR   Transfer error detected
 * @retval XSPI_TIMEOUT Timeout while waiting for completion
 */
static XSPI_Status_TypeDef _XSPI_wait_transfer_complete(XSPI_TypeDef *xspi)
{
    uint32_t timeout = _XSPI_TIMEOUT;

    while ((xspi->SR & XSPI_SR_TCF) == 0U) {
        if ((xspi->SR & XSPI_SR_TEF) != 0U) {
            xspi->FCR = XSPI_FCR_CTEF;
            return XSPI_ERROR;
        }

        if (--timeout == 0U) {
            return XSPI_TIMEOUT;
        }
    }

    xspi->FCR = XSPI_FCR_CTCF;

    return XSPI_OK;
}

/**
 * @brief Clear the XSPI transfer-complete and transfer-error flags
 *
 * @param[in] xspi Pointer to the XSPI peripheral instance
 */
static void _XSPI_clear_flags(XSPI_TypeDef *xspi)
{
    xspi->FCR = XSPI_FCR_CTCF | XSPI_FCR_CTEF;
}

/**
 * @brief Set the clock prescaler of an XSPI peripheral
 *
 * @param[in] xspi      Pointer to the XSPI peripheral instance
 * @param[in] prescaler Prescaler register value
 *
 * @retval XSPI_OK      Prescaler changed successfully
 * @retval XSPI_TIMEOUT Peripheral remained busy
 */
static XSPI_Status_TypeDef _XSPI_prescaler_set(XSPI_TypeDef *xspi, uint32_t prescaler)
{
    XSPI_Status_TypeDef status;

    status = _XSPI_wait_not_busy(xspi);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(xspi);

    xspi->DCR2 =
        (xspi->DCR2 & ~XSPI_DCR2_PRESCALER_Msk)
        | _XSPI_FIELD(DCR2, PRESCALER, prescaler);

    status = _XSPI_wait_not_busy(xspi);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(xspi);

    return XSPI_OK;
}

/**
 * @brief Build an XSPI CCR or WCCR register value
 *
 * @param[in] cfg Communication configuration
 *
 * @return Encoded CCR register value
 */
static uint32_t _XSPI_CCR_build(XSPI_CCR_cfg_TypeDef cfg)
{
    return _XSPI_FIELD(CCR, IMODE,  cfg.instruction_mode)
         | _XSPI_FIELD(CCR, IDTR,   cfg.instruction_dtr)
         | _XSPI_FIELD(CCR, ISIZE,  cfg.instruction_size)
         | _XSPI_FIELD(CCR, ADMODE, cfg.address_mode)
         | _XSPI_FIELD(CCR, ADDTR,  cfg.address_dtr)
         | _XSPI_FIELD(CCR, ADSIZE, cfg.address_size)
         | _XSPI_FIELD(CCR, DMODE,  cfg.data_mode)
         | _XSPI_FIELD(CCR, DDTR,   cfg.data_dtr)
         | _XSPI_FIELD(CCR, DQSE,   cfg.data_qse);
}

/**
 * @brief Configure the common registers of an XSPI peripheral
 *
 * @param[in] xspi Pointer to the XSPI peripheral instance
 * @param[in] cfg  Device configuration
 */
static void _XSPI_device_init(XSPI_TypeDef *xspi, XSPI_cfg_TypeDef cfg)
{
    xspi->DCR1 =
          _XSPI_FIELD(DCR1, MTYP,    cfg.memory_type)
        | _XSPI_FIELD(DCR1, DEVSIZE, cfg.devsize)
        | _XSPI_FIELD(DCR1, CSHT,    cfg.chipselect_high_time);

    xspi->DCR2 =
        _XSPI_FIELD(DCR2, PRESCALER, cfg.prescaler);

    xspi->DCR3 =
          _XSPI_FIELD(DCR3, MAXTRAN, cfg.maxtran_value)
        | _XSPI_FIELD(DCR3, CSBOUND, cfg.chipselect_boundary);

    xspi->DCR4 = cfg.refresh_cycles;

    xspi->CR =
        _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD);

    xspi->TCR = 0U;
}

/**
 * @brief Configure XSPI1 GPIO pins for PSRAM (APS256XX)
 *
 * XSPI1 pin mapping:
 *   GPIOO{0,2,3,4} = CLK, IO3, DQS0, NCS1
 *   GPIOP{0..15}   = IO0-IO15
 */
static void _XSPI1_PSRAM_GPIO_init(void)
{
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

/**
 * @brief Configure XSPI2 GPIO pins for NOR Flash (MX66UW1G45G)
 *
 * XSPI2 pin mapping (all GPION, AF9):
 *   PN0=DQS0, PN1=NCS1, PN2-5=IO0-3, PN6=CLK,
 *   PN7=NCLK, PN8-11=IO4-7
 */
static void _XSPI2_NOR_GPIO_init(void)
{
    RCC_enable_GPIO(GPION);

    for (int p = 0; p <= 11; p++){
        GPIO_Config(GPION, p, GPIO_XSPI_cfg);
    }
}

/**
 * @brief Write one APS256XX mode register
 *
 * @param[in] register_address Address of the PSRAM mode register
 * @param[in] value            Value to write
 *
 * @retval XSPI_OK      Register written successfully
 * @retval XSPI_ERROR   Transfer error detected
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _PSRAM_register_write(uint8_t register_address, uint8_t value)
{
    XSPI_Status_TypeDef status;
    uint32_t ccr;

    status = _XSPI_wait_not_busy(XSPI1);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI1);

    XSPI1->CR =
          _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
        | XSPI_CR_EN;

    XSPI1->TCR = 0U;

    ccr = _XSPI_CCR_build(XSPI_write_reg_cfg);
    XSPI1->CCR = ccr;

    XSPI1->IR  = _PSRAM_REGISTER_WRITE_CMD;
    XSPI1->AR  =  register_address;
    XSPI1->DLR = _PSRAM_REGISTER_WRITE_DLR;

    *((__IO uint8_t *)&XSPI1->DR) = value;
    *((__IO uint8_t *)&XSPI1->DR) = _PSRAM_REGISTER_DUMMY_BYTE;

    status = _XSPI_wait_transfer_complete(XSPI1);
    if (status != XSPI_OK) {
        return status;
    }

    XSPI1->AR = _XSPI_CLEAR_ADDRESS;

    return XSPI_OK;
}

/**
 * @brief Configure the APS256XX mode registers
 *
 * Programs mode registers MR0, MR4 and MR8.
 *
 * @retval XSPI_OK      Configuration completed successfully
 * @retval XSPI_ERROR   Register write failed
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _PSRAM_config_write(void)
{
    XSPI_Status_TypeDef status;

    status = _PSRAM_register_write(
        _PSRAM_MR0_ADDRESS,
        _PSRAM_MR0_VALUE);

    if (status != XSPI_OK) {
        return status;
    }

    status = _PSRAM_register_write(
        _PSRAM_MR4_ADDRESS,
        _PSRAM_MR4_VALUE);

    if (status != XSPI_OK) {
        return status;
    }

    status = _PSRAM_register_write(
        _PSRAM_MR8_ADDRESS,
        _PSRAM_MR8_VALUE);

    if (status != XSPI_OK) {
        return status;
    }

    return XSPI_OK;
}

/**
 * @brief Enable memory-mapped mode for the APS256XX PSRAM
 *
 * @retval XSPI_OK      Memory-mapped mode enabled
 * @retval XSPI_TIMEOUT XSPI1 remained busy
 */
static XSPI_Status_TypeDef _PSRAM_memory_mapped_enable(void)
{
    XSPI_Status_TypeDef status;
    uint32_t ccr;

    status = _XSPI_wait_not_busy(XSPI1);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI1);

    ccr = _XSPI_CCR_build(XSPI_PSRAM_memorymapped_cfg);

    XSPI1->WCCR = ccr;
    XSPI1->WIR  = _PSRAM_LINEAR_WRITE_CMD;
    XSPI1->WTCR =
        _XSPI_FIELD(WTCR, DCYC, _PSRAM_WRITE_DUMMY_CYCLES);

    XSPI1->CCR = ccr;
    XSPI1->IR  = _PSRAM_LINEAR_READ_CMD;
    XSPI1->TCR =
        _XSPI_FIELD(TCR, DCYC, _PSRAM_READ_DUMMY_CYCLES);

    XSPI1->CR =
          _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_MEMORY_MAPPED)
        | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
        | XSPI_CR_EN;

    return XSPI_OK;
}

/**
 * @brief Send a command to the NOR flash in SPI-STR mode
 *
 * Uses the 1-0-0 protocol without address or data phase.
 *
 * @param[in] instruction Command opcode
 *
 * @retval XSPI_OK      Command completed successfully
 * @retval XSPI_ERROR   Transfer error detected
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _NOR_command_SPI(uint8_t instruction)
{
    XSPI_Status_TypeDef status;

    status = _XSPI_wait_not_busy(XSPI2);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI2);

    XSPI2->TCR = 0U;
    XSPI2->DLR = 0U;

    XSPI2->CCR =
          _XSPI_FIELD(CCR, IMODE, 1U)
        | _XSPI_FIELD(CCR, IDTR, 0U)
        | _XSPI_FIELD(CCR, ISIZE, 0U)
        | _XSPI_FIELD(CCR, ADMODE, 0U)
        | _XSPI_FIELD(CCR, DMODE, 0U);

    XSPI2->CR =
          _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_INDIRECT_WRITE)
        | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
        | XSPI_CR_EN;

    XSPI2->IR = instruction;

    return _XSPI_wait_transfer_complete(XSPI2);
}

/**
 * @brief Read data from the NOR flash in SPI-STR mode
 *
 * Uses the 1-0-1 protocol.
 *
 * @param[in]  instruction Command opcode
 * @param[out] data        Destination buffer
 * @param[in]  length      Number of bytes to receive
 *
 * @retval XSPI_OK            Read completed successfully
 * @retval XSPI_ERROR         Transfer error detected
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid buffer or length
 */
static XSPI_Status_TypeDef _NOR_read_SPI(
    uint8_t instruction,
    uint8_t *data,
    uint32_t length)
{
    XSPI_Status_TypeDef status;
    uint32_t timeout;

    if ((data == NULL) || (length == 0U)) {
        return XSPI_INVALID_PARAM;
    }

    status = _XSPI_wait_not_busy(XSPI2);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI2);

    XSPI2->TCR = 0U;
    XSPI2->DLR = length - 1U;

    XSPI2->CCR =
          _XSPI_FIELD(CCR, IMODE, 1U)
        | _XSPI_FIELD(CCR, IDTR, 0U)
        | _XSPI_FIELD(CCR, ISIZE, 0U)
        | _XSPI_FIELD(CCR, ADMODE, 0U)
        | _XSPI_FIELD(CCR, DMODE, 1U)
        | _XSPI_FIELD(CCR, DDTR, 0U)
        | _XSPI_FIELD(CCR, DQSE, 0U);

    XSPI2->CR =
          _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_INDIRECT_READ)
        | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
        | XSPI_CR_EN;

    XSPI2->IR = instruction;

    for (uint32_t index = 0U; index < length; index++) {
        timeout = _XSPI_TIMEOUT;

        while ((XSPI2->SR & XSPI_SR_FLEVEL_Msk) == 0U) {
            if ((XSPI2->SR & XSPI_SR_TEF) != 0U) {
                XSPI2->FCR = XSPI_FCR_CTEF;
                return XSPI_ERROR;
            }

            if (--timeout == 0U) {
                return XSPI_TIMEOUT;
            }
        }

        data[index] = *((__IO uint8_t *)&XSPI2->DR);
    }

    return _XSPI_wait_transfer_complete(XSPI2);
}

/**
 * @brief Write one byte to an addressed NOR configuration register
 *
 * Uses the SPI 1-1-1 protocol with a 32-bit address.
 *
 * @param[in] instruction Command opcode
 * @param[in] address     Register address
 * @param[in] value       Value to write
 *
 * @retval XSPI_OK      Write completed successfully
 * @retval XSPI_ERROR   Transfer error detected
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _NOR_write_SPI_addressed(uint8_t instruction, uint32_t address, uint8_t value)
{
    XSPI_Status_TypeDef status;

    status = _XSPI_wait_not_busy(XSPI2);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI2);

    XSPI2->TCR = 0U;
    XSPI2->DLR = 0U;

    XSPI2->CCR = _XSPI_FIELD(CCR, IMODE,  1U)
			   | _XSPI_FIELD(CCR, IDTR,   0U)
			   | _XSPI_FIELD(CCR, ISIZE,  0U)
			   | _XSPI_FIELD(CCR, ADMODE, 1U)
			   | _XSPI_FIELD(CCR, ADDTR,  0U)
			   | _XSPI_FIELD(CCR, ADSIZE, 3U)
			   | _XSPI_FIELD(CCR, DMODE,  1U)
			   | _XSPI_FIELD(CCR, DDTR,   0U)
			   | _XSPI_FIELD(CCR, DQSE,   0U);

    XSPI2->CR = _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_INDIRECT_WRITE)
        	  | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
			  | XSPI_CR_EN;

    XSPI2->IR = instruction;
    XSPI2->AR = address;

    *((__IO uint8_t *)&XSPI2->DR) = value;

    return _XSPI_wait_transfer_complete(XSPI2);
}

/**
 * @brief Reset the MX66UW1G45G NOR flash in SPI-STR mode
 *
 * @retval XSPI_OK      Reset completed successfully
 * @retval XSPI_ERROR   Reset command failed
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _NOR_reset(void)
{
    XSPI_Status_TypeDef status;

    status = _NOR_command_SPI(_NOR_RESET_ENABLE_CMD);
    if (status != XSPI_OK) {
        return status;
    }

    status = _NOR_command_SPI(_NOR_RESET_MEMORY_CMD);
    if (status != XSPI_OK) {
        return status;
    }

    TIMER_Delay_ms(1);

    return XSPI_OK;
}


/**
 * @brief Read the JEDEC identification of the NOR flash
 *
 * @param[out] id Buffer for the three identification bytes
 *
 * @retval XSPI_OK            Identification read successfully
 * @retval XSPI_ERROR         Transfer error detected
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid destination buffer
 */
static XSPI_Status_TypeDef _NOR_read_ID(uint8_t id[_NOR_ID_SIZE])
{
    return _NOR_read_SPI(_NOR_READ_ID_CMD, id, _NOR_ID_SIZE);
}


/**
 * @brief Validate the NOR flash JEDEC identification
 *
 * @param[in] id Three-byte JEDEC identification
 *
 * @retval 1 Identification is valid
 * @retval 0 Identification is invalid
 */
static uint8_t _NOR_ID_is_valid(const uint8_t id[_NOR_ID_SIZE])
{
    if (id == NULL) {
        return 0;
    }

    return (id[0] == _NOR_MANUFACTURER_ID)
        && (id[1] == _NOR_MEMORY_TYPE_ID)
        && (id[2] == _NOR_MEMORY_DENSITY_ID);
}


/**
 * @brief Read the NOR status register in SPI-STR mode
 *
 * @param[out] status Destination for the status-register value
 *
 * @retval XSPI_OK            Status read successfully
 * @retval XSPI_ERROR         Transfer error detected
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid destination pointer
 */
static XSPI_Status_TypeDef _NOR_read_status_SPI(uint8_t *status)
{
    return _NOR_read_SPI(_NOR_READ_STATUS_CMD, status, 1);
}

/**
 * @brief Enable write operations on the NOR flash
 *
 * Sends Write Enable and verifies the WEL bit.
 *
 * @retval XSPI_OK      Write Enable Latch was set
 * @retval XSPI_ERROR   WEL bit was not set
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _NOR_write_enable(void)
{
    XSPI_Status_TypeDef status;
    uint8_t flash_status = 0U;

    status = _NOR_command_SPI(_NOR_WRITE_ENABLE_CMD);
    if (status != XSPI_OK) {
        return status;
    }

    status = _NOR_read_status_SPI(&flash_status);
    if (status != XSPI_OK) {
        return status;
    }

    if ((flash_status & _NOR_STATUS_WEL) == 0U) {
        return XSPI_ERROR;
    }

    return XSPI_OK;
}

/**
 * @brief Switch the NOR flash from SPI-STR to OPI-DTR mode
 *
 * Programs the DOPI bit in Configuration Register 2.
 *
 * @retval XSPI_OK      OPI-DTR mode configured successfully
 * @retval XSPI_ERROR   Configuration write failed
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
static XSPI_Status_TypeDef _NOR_OPI_DTR_enable(void)
{
    XSPI_Status_TypeDef status;

    status = _NOR_write_enable();
    if (status != XSPI_OK) {
        return status;
    }

    status = _NOR_write_SPI_addressed(_NOR_WRITE_CFG_REG2_CMD, _NOR_CR2_PROTOCOL_ADDRESS, _NOR_CR2_DOPI_VALUE);

    if (status != XSPI_OK) {
        return status;
    }

    TIMER_Delay_ms(1);

    return XSPI_OK;
}

/**
 * @brief Read the NOR status register in OPI-DTR mode
 *
 * @param[out] status_data Two-byte status response
 *
 * @retval XSPI_OK            Status read successfully
 * @retval XSPI_ERROR         Transfer error detected
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid destination buffer
 */
static XSPI_Status_TypeDef _NOR_read_status_OPI_DTR(uint8_t status_data[_NOR_DTR_STATUS_SIZE])
{
    XSPI_Status_TypeDef status;
    uint32_t timeout;

    if (status_data == NULL) {
        return XSPI_INVALID_PARAM;
       }

       status = _XSPI_wait_not_busy(XSPI2);
       if (status != XSPI_OK) {
           return status;
       }

       _XSPI_clear_flags(XSPI2);

       XSPI2->DLR = _NOR_DTR_STATUS_SIZE - 1;

       XSPI2->TCR = XSPI_TCR_DHQC
    		   	  | _XSPI_FIELD(TCR, DCYC, _NOR_REGISTER_DTR_DUMMY);

       XSPI2->CCR = _XSPI_FIELD(CCR, IMODE, 4U)
                  | _XSPI_FIELD(CCR, IDTR, 1U)
				  | _XSPI_FIELD(CCR, ISIZE, 1U)
				  | _XSPI_FIELD(CCR, ADMODE, 4U)
				  | _XSPI_FIELD(CCR, ADDTR, 1U)
				  | _XSPI_FIELD(CCR, ADSIZE, 3U)
				  | _XSPI_FIELD(CCR, DMODE, 4U)
				  | _XSPI_FIELD(CCR, DDTR, 1U)
				  | _XSPI_FIELD(CCR, DQSE, 1U);

       XSPI2->CR  = _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_INDIRECT_READ)
                  | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
				  | XSPI_CR_EN;

       XSPI2->IR = _NOR_OCTA_READ_STATUS_CMD;
       XSPI2->AR = 0U;

       for (uint32_t index = 0U;
            index < _NOR_DTR_STATUS_SIZE;
            index++) {

           timeout = _XSPI_TIMEOUT;

           while ((XSPI2->SR & XSPI_SR_FLEVEL_Msk) == 0U) {
               if ((XSPI2->SR & XSPI_SR_TEF) != 0U) {
                   XSPI2->FCR = XSPI_FCR_CTEF;
                   return XSPI_ERROR;
               }

               if (--timeout == 0U) {
                   return XSPI_TIMEOUT;
               }
           }

           status_data[index] =
               *((__IO uint8_t *)&XSPI2->DR);
       }

       return _XSPI_wait_transfer_complete(XSPI2);
   }

/**
 * @brief Enable memory-mapped OPI-DTR mode for the NOR flash
 *
 * Configures the octal DTR read and page-program command formats
 * and switches XSPI2 into memory-mapped mode.
 *
 * @retval XSPI_OK      Memory-mapped mode enabled
 * @retval XSPI_TIMEOUT XSPI2 remained busy
 */
static XSPI_Status_TypeDef _NOR_memory_mapped_enable(void)
{
    XSPI_Status_TypeDef status;
    uint32_t ccr;

    status = _XSPI_wait_not_busy(XSPI2);
    if (status != XSPI_OK) {
        return status;
    }

    _XSPI_clear_flags(XSPI2);

    ccr = _XSPI_CCR_build(XSPI_NOR_memorymapped_cfg);

    XSPI2->CCR = ccr;
    XSPI2->IR  = _NOR_OCTA_READ_DTR_CMD;
    XSPI2->TCR =
          XSPI_TCR_DHQC
        | _XSPI_FIELD(TCR, DCYC, _NOR_READ_DTR_DUMMY);

    XSPI2->WCCR = ccr;
    XSPI2->WIR  = _NOR_OCTA_PAGE_PROGRAM_CMD;
    XSPI2->WTCR = 0U;

    XSPI2->CR =
          _XSPI_FIELD(CR, FMODE, _XSPI_FMODE_MEMORY_MAPPED)
        | _XSPI_FIELD(CR, FTHRES, _XSPI_FIFO_THRESHOLD)
        | XSPI_CR_EN;

    return XSPI_OK;
}

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * @brief Initialise the APS256XX PSRAM connected to XSPI1
 *
 * @param[in] init_cfg XSPI1 and PSRAM configuration
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   PSRAM or XSPI transfer failed
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
XSPI_Status_TypeDef XSPI_PSRAM_init(XSPI_cfg_TypeDef init_cfg)
{
	XSPI_Status_TypeDef status;

    RCC_setXSPI1_clock_source(0);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    TIMER_Delay_ms(1);

    RCC_reset_XSPI1();
    RCC_reset_XSPIM();
    TIMER_Delay_ms(1);

    RCC_enable_XSPI1();
    RCC_enable_XSPIM();
    TIMER_Delay_ms(1);

    _XSPI1_PSRAM_GPIO_init();
    TIMER_Delay_ms(1);

    _XSPI_device_init(XSPI1, init_cfg);
    TIMER_Delay_ms(1);

    status = _PSRAM_config_write();
    if (status != XSPI_OK) {
        return status;
    }
    TIMER_Delay_ms(1);

    status = _XSPI_prescaler_set(XSPI1, 0);
    if (status != XSPI_OK) {
        return status;
    }
    TIMER_Delay_ms(1);

    status = _PSRAM_memory_mapped_enable();
    if (status != XSPI_OK) {
        return status;
    }
    TIMER_Delay_ms(1);

    return XSPI_OK;
}

/**
 * @brief Initialise the MX66UW1G45G NOR flash connected to XSPI2
 *
 * @param[in] init_cfg XSPI2 and NOR configuration
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   Identification, status or transfer validation failed
 * @retval XSPI_TIMEOUT Peripheral operation timed out
 */
XSPI_Status_TypeDef XSPI_NOR_init(XSPI_cfg_TypeDef init_cfg)
{
    XSPI_Status_TypeDef status;
    uint8_t nor_id[_NOR_ID_SIZE] = {0};
    uint8_t spi_status = 0;
    uint8_t dtr_status[_NOR_DTR_STATUS_SIZE] = {0};

    RCC_setXSPI2_clock_source(0U);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();
    TIMER_Delay_ms(1U);

    RCC_reset_XSPI2();
    RCC_reset_XSPIM();
    TIMER_Delay_ms(1U);

    RCC_enable_XSPI2();
    RCC_enable_XSPIM();
    TIMER_Delay_ms(1U);

    _XSPI2_NOR_GPIO_init();
    TIMER_Delay_ms(1U);

    _XSPI_device_init(XSPI2, init_cfg);
    TIMER_Delay_ms(1U);

    status = _NOR_reset();
    if (status != XSPI_OK) {
        return status;
    }

    status = _NOR_read_ID(nor_id);
    if (status != XSPI_OK) {
        return status;
    }

    if (!_NOR_ID_is_valid(nor_id)) {
        return XSPI_ERROR;
    }

    status = _NOR_read_status_SPI(&spi_status);
    if (status != XSPI_OK) {
        return status;
    }

    if ((spi_status & _NOR_STATUS_WIP) != 0U) {
        return XSPI_ERROR;
    }

    status = _NOR_OPI_DTR_enable();
    if (status != XSPI_OK) {
        return status;
    }

    status = _NOR_read_status_OPI_DTR(dtr_status);
    if (status != XSPI_OK) {
        return status;
    }

    if ((dtr_status[0] & _NOR_STATUS_WIP) != 0U) {
        return XSPI_ERROR;
    }


    status = _XSPI_prescaler_set(XSPI2, 0U);
    if (status != XSPI_OK) {
        return status;
    }

    TIMER_Delay_ms(1U);

    status = _NOR_memory_mapped_enable();
    if (status != XSPI_OK) {
        return status;
    }

    TIMER_Delay_ms(1U);

    return XSPI_OK;
}
