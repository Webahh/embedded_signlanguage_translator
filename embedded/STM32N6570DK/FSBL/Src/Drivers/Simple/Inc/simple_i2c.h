/**
 * @file    simple_i2c.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   I2C bus driver header
 *
 * Usage
 * -----
 * 1. RCC_enable_I2C() + I2C_Config()   – initialise I2C bus
 * 2. I2C_Device_ready()                 – probe device
 * 3. I2C_Mem_write() / I2C_Mem_read()   – register access
 */

#ifndef SIMPLE_I2C_H
#define SIMPLE_I2C_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

#define I2C_SENSOR_BUS_TIMING   0x01B11628

/** I2C transfer status codes */
typedef enum {
    I2C_OK      = 0, /**< Transaction completed successfully */
    I2C_ERROR   = 1, /**< Arbitration lost, bus error, or overrun */
    I2C_BUSY    = 2, /**< Bus busy (not currently used) */
    I2C_TIMEOUT = 3, /**< Polling loop expired without expected flag */
    I2C_NACK    = 4, /**< Slave responded with NACK */
} I2C_Status_TypeDef;

// ---- API ----

/**
 * @brief  Configure I2C peripheral clock source, timing, and enable
 * @param [in] I2CX         | I2C instance (e.g. I2C1)
 * @param [in] clock_source | RCC clock source selection
 * @param [in] timing       | I2C_TIMINGR register value
 */
void I2C_Config(I2C_TypeDef *I2CX, uint32_t clock_source, uint32_t timing);

/**
 * @brief  Write data to a 16-bit register address (combined format)
 * @param [in] I2CX      | I2C instance
 * @param [in] dev_addr  | 7-bit slave address
 * @param [in] mem_addr  | 16-bit register address (big-endian on wire)
 * @param [in] data      | Payload bytes to write
 * @param [in] len       | Number of payload bytes
 * @retval I2C_OK      Success
 * @retval I2C_NACK    Slave NACK
 * @retval I2C_TIMEOUT Poll loop expired
 * @retval I2C_ERROR   Arbitration lost, bus error, or overrun
 */
I2C_Status_TypeDef I2C_Mem_write(I2C_TypeDef *I2CX, uint16_t dev_addr,
                                  uint16_t mem_addr, const uint8_t *data,
                                  uint16_t len);

/**
 * @brief  Read data from a 16-bit register address (combined format)
 * @param [in] I2CX      | I2C instance
 * @param [in] dev_addr  | 7-bit slave address
 * @param [in] mem_addr  | 16-bit register address (big-endian on wire)
 * @param [in] data      | Destination buffer for received bytes
 * @param [in] len       | Number of bytes to read
 * @retval I2C_OK      Success
 * @retval I2C_NACK    Slave NACK
 * @retval I2C_TIMEOUT Poll loop expired
 * @retval I2C_ERROR   Arbitration lost, bus error, or overrun
 */
I2C_Status_TypeDef I2C_Mem_read(I2C_TypeDef *I2CX, uint16_t dev_addr,
                                 uint16_t mem_addr, uint8_t *data,
                                 uint16_t len);

/**
 * @brief  Probe a 7-bit address to check if a slave ACKs
 * @param [in] I2CX     | I2C instance
 * @param [in] dev_addr | 7-bit slave address
 * @retval I2C_OK   Device present (ACK received)
 * @retval I2C_NACK No device at this address
 * @retval I2C_TIMEOUT Bus stuck busy
 */
I2C_Status_TypeDef I2C_Device_ready(I2C_TypeDef *I2CX, uint16_t dev_addr);

/**
 * @brief  Scan all 7-bit addresses and collect responding slaves
 * @param [in]  I2CX        | I2C instance
 * @param [out] found_addrs | Output buffer (must hold at least 128 bytes)
 * @param [out] count       | Set to number of addresses found
 */
void I2C_Scan(I2C_TypeDef *I2CX, uint8_t *found_addrs, uint32_t *count);

#endif /* SIMPLE_I2C_H */
