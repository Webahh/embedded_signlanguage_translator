/*
 * simple_i2c.h
 *
 *  Created on: 21.05.2026
 *      Author: Weber
 */

#ifndef SIMPLE_I2C_H
#define SIMPLE_I2C_H

#include "stm32n657xx.h"

#define I2C_SENSOR_BUS_TIMING 	0x01B11628

/** I2C transfer status codes */
typedef enum {
    I2C_OK      = 0, /**< Transaction completed successfully */
    I2C_ERROR   = 1, /**< Arbitration lost, bus error, or overrun */
    I2C_BUSY    = 2, /**< Bus busy (not currently used) */
    I2C_TIMEOUT = 3, /**< Polling loop expired without expected flag */
    I2C_NACK    = 4, /**< Slave responded with NACK */
} I2C_Status_TypeDef;

/**
 * @brief  Configure I2C peripheral clock source, timing, and enable
 * @param  I2CX       I2C instance (e.g. I2C1)
 * @param  clock_source RCC clock source selection (passed to RCC_setI2C_clock_source)
 * @param  timing     I2C_TIMINGR register value (PRESC, SCLL, SCLH, SDADEL, SCLDEL)
 */
void I2C_Config(I2C_TypeDef *I2CX, uint32_t clock_source, uint32_t timing);

/**
 * @brief  Write data to a 16-bit register address (combined format)
 * @param  I2CX      I2C instance
 * @param  dev_addr  7-bit slave address
 * @param  mem_addr  16-bit register address (big-endian on wire)
 * @param  data      Payload bytes to write
 * @param  len       Number of payload bytes (total frame = 2 + len, max 255)
 * @retval I2C_OK on success, I2C_NACK/I2C_TIMEOUT/I2C_ERROR on failure
 */
I2C_Status_TypeDef I2C_Mem_Write(I2C_TypeDef *I2CX, uint16_t dev_addr,
                                  uint16_t mem_addr, const uint8_t *data,
                                  uint16_t len);

/**
 * @brief  Read data from a 16-bit register address (combined format)
 * @param  I2CX      I2C instance
 * @param  dev_addr  7-bit slave address
 * @param  mem_addr  16-bit register address (big-endian on wire)
 * @param  data      Destination buffer for received bytes
 * @param  len       Number of bytes to read (max 255)
 * @retval I2C_OK on success, I2C_NACK/I2C_TIMEOUT/I2C_ERROR on failure
 */
I2C_Status_TypeDef I2C_Mem_Read(I2C_TypeDef *I2CX, uint16_t dev_addr,
                                 uint16_t mem_addr, uint8_t *data,
                                 uint16_t len);

/**
 * @brief  Probe a 7-bit address to check if a slave ACKs
 * @param  I2CX     I2C instance
 * @param  dev_addr 7-bit slave address
 * @retval I2C_OK if slave ACKed, I2C_NACK if no response, I2C_TIMEOUT on busy
 */
I2C_Status_TypeDef I2C_IsDeviceReady(I2C_TypeDef *I2CX, uint16_t dev_addr);

/**
 * @brief  Scan all 7-bit addresses (1..0x7F) and collect responding slaves
 * @param  I2CX         I2C instance
 * @param  found_addrs  Output buffer (must hold at least 128 bytes)
 * @param  count        Set to number of addresses found
 */
void I2C_Scan(I2C_TypeDef *I2CX, uint8_t *found_addrs, uint32_t *count);

#endif /* SIMPLE_I2C_H */
