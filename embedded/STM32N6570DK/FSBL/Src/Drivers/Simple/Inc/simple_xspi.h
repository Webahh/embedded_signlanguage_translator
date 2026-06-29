/**
 * @file    simple_xspi.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   XSPI PSRAM / NOR flash driver header
 *
 * Usage
 * -----
 * 1. XSPI_PSRAM_init() / XSPI_NOR_init()	-  initialise with config
 * 2. Memory-mapped access                  - read / write via mapped address
 */

#ifndef SIMPLE_XSPI_H
#define SIMPLE_XSPI_H

#include <stdint.h>

typedef enum {
    XSPI_OK      = 0,
    XSPI_ERROR,
    XSPI_TIMEOUT,
	XSPI_INVALID_PARAM
} XSPI_Status_TypeDef;

typedef struct {
    uint8_t memory_type;
    uint8_t devsize;
    uint8_t chipselect_high_time;
    uint8_t prescaler;
    uint8_t chipselect_boundary;
    uint16_t maxtran_value;
    uint16_t refresh_cycles;
} XSPI_cfg_TypeDef;

typedef struct {
    uint8_t instruction_mode;
    uint8_t instruction_dtr;
    uint8_t instruction_size;
    uint8_t address_mode;
    uint8_t address_dtr;
    uint8_t address_size;
    uint8_t data_mode;
    uint8_t data_dtr;
    uint8_t data_qse;
} XSPI_CCR_cfg_TypeDef;

/**
 * @brief Initialise the APS256XX PSRAM connected to XSPI1
 *
 * The function performs:
 * - XSPI1 clock and GPIO initialisation
 * - peripheral reset and configuration
 * - PSRAM mode-register configuration
 * - clock-prescaler configuration
 * - activation of memory-mapped mode
 *
 * After successful initialisation, the PSRAM can be accessed through its
 * memory-mapped address range.
 *
 * @param[in] init_cfg XSPI1 and PSRAM configuration
 *
 * @retval XSPI_OK            Initialisation successful
 * @retval XSPI_ERROR         XSPI transfer error
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid configuration parameter
 */
XSPI_Status_TypeDef XSPI_PSRAM_init(XSPI_cfg_TypeDef init_cfg);

/**
 * @brief Initialise the MX66UW1G45G NOR flash connected to XSPI2
 *
 * The function performs:
 * - XSPI2 clock and GPIO initialisation
 * - peripheral reset and configuration
 * - NOR reset in SPI-STR mode
 * - JEDEC-ID validation
 * - flash-ready status validation
 * - switch from SPI-STR to OPI-DTR mode
 * - OPI-DTR communication validation
 * - clock-prescaler configuration
 * - activation of memory-mapped mode
 *
 * After successful initialisation, the NOR flash can be read through the
 * memory-mapped address range beginning at 0x71000000.
 *
 * @param[in] init_cfg XSPI2 and NOR configuration
 *
 * @retval XSPI_OK            Initialisation successful
 * @retval XSPI_ERROR         Flash identification, status or transfer error
 * @retval XSPI_TIMEOUT       Peripheral operation timed out
 * @retval XSPI_INVALID_PARAM Invalid configuration parameter
 */
XSPI_Status_TypeDef XSPI_NOR_init(XSPI_cfg_TypeDef init_cfg);

#endif /* SIMPLE_XSPI_H */
