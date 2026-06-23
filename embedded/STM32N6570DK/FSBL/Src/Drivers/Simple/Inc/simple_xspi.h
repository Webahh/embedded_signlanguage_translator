/**
 * @file    simple_xspi.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   XSPI PSRAM / NOR flash driver header
 *
 * Usage
 * -----
 * 1. XSPI_PSRAM_init() / XSPI_NOR_init()   – initialise with config
 * 2. Memory-mapped access                    – read / write via mapped address
 */

#ifndef SIMPLE_XSPI_H
#define SIMPLE_XSPI_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

typedef enum {
    XSPI_OK      = 0,
    XSPI_ERROR   = 1,
    XSPI_TIMEOUT = 2
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
 * @brief Initialise PSRAM device on XSPI1
 *
 * @param [in] init_cfg | XSPI configuration parameters
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   Initialisation failed
 */
XSPI_Status_TypeDef XSPI_PSRAM_init(XSPI_cfg_TypeDef init_cfg);

/**
 * @brief Initialise NOR Flash device on XSPI2
 *
 * @param [in] init_cfg | XSPI configuration parameters
 *
 * @retval XSPI_OK      Initialisation successful
 * @retval XSPI_ERROR   Initialisation failed
 */
XSPI_Status_TypeDef XSPI_NOR_init(XSPI_cfg_TypeDef init_cfg);

#endif /* SIMPLE_XSPI_H */
