/**
 * @file    simple_i2c.c
 * @brief   Register-level I2C master driver for STM32N6.
 *
 *  Created on: 21.05.2026
 *      Author: Weber
 */

#include "simple_i2c.h"
#include "simple_rcc.h"
#include <stddef.h>

/** Maximum poll iterations before returning I2C_TIMEOUT */
#define I2C_TIMEOUT_MAX 1000U

/** Shift a 7-bit address left by 1 for the SADD field */
#define I2C_7BIT_ADDR(addr7) (((uint32_t)(addr7) & 0x7FU) << 1)

/**
 * @brief  Configure and enable an I2C peripheral.
 * @param  I2CX        I2C instance (e.g. I2C1)
 * @param  clock_source RCC source selection (0 = PCLK1, etc.)
 * @param  timing       Value for I2C_TIMINGR register
 */
void I2C_Config(I2C_TypeDef *I2CX, uint32_t clock_source, uint32_t timing)
{
    RCC_setI2C_clock_source(I2CX, clock_source);
    RCC_enable_I2C(I2CX);
    RCC_reset_I2C(I2CX);

    I2CX->CR1 &= ~I2C_CR1_PE;

    I2CX->CR1 &= ~I2C_CR1_ANFOFF;
    I2CX->CR1 &= ~I2C_CR1_DNF;
    I2CX->TIMINGR = timing;
    I2CX->CR1 &= ~I2C_CR1_NOSTRETCH;

    I2CX->OAR1 = I2C_OAR1_OA1EN;

    I2CX->CR1 |= I2C_CR1_PE;
}

/**
 * @brief  Wait for an ISR flag to be set or cleared.
 *
 * @param  I2CX  I2C instance
 * @param  flag  ISR bit to test (e.g. I2C_ISR_TXIS)
 * @param  set   1 = wait until flag is set, 0 = wait until flag is cleared
 * @retval I2C_OK      flag reached expected state
 * @retval I2C_NACK    slave NACK detected
 * @retval I2C_ERROR   arbitration lost, bus error, or overrun
 * @retval I2C_TIMEOUT loop expired
 */
static I2C_Status_TypeDef I2C_WaitFlag(I2C_TypeDef *I2CX,
                                        uint32_t flag,
                                        uint8_t set)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;

    while (timeout--) {
        uint32_t isr = I2CX->ISR;

        if (isr & I2C_ISR_NACKF) {
            I2CX->ICR = I2C_ICR_NACKCF;
            if (isr & I2C_ISR_STOPF)
                I2CX->ICR = I2C_ICR_STOPCF;
            return I2C_NACK;
        }

        if (isr & I2C_ISR_ARLO) {
            I2CX->ICR = I2C_ICR_ARLOCF;
            return I2C_ERROR;
        }

        if (isr & I2C_ISR_BERR) {
            I2CX->ICR = I2C_ICR_BERRCF;
            return I2C_ERROR;
        }

        if (isr & I2C_ISR_OVR) {
            I2CX->ICR = I2C_ICR_OVRCF;
            return I2C_ERROR;
        }

        if (set) {
            if (isr & flag)
                return I2C_OK;
        } else {
            if (!(isr & flag))
                return I2C_OK;
        }
    }

    return I2C_TIMEOUT;
}

/**
 * @brief  Wait until the I2C bus is not busy (BUSY flag cleared)
 * @param  I2CX I2C instance
 * @retval I2C_OK if bus became free
 * @retval I2C_TIMEOUT if bus stayed busy
 */
static I2C_Status_TypeDef I2C_WaitNotBusy(I2C_TypeDef *I2CX)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;

    while (timeout--) {
        if ((I2CX->ISR & I2C_ISR_BUSY) == 0)
            return I2C_OK;
    }

    return I2C_TIMEOUT;
}

/**
 * @brief  Write data to a device at a 16-bit register address
 *
 * @param  I2CX      I2C instance
 * @param  dev_addr  7-bit slave address
 * @param  mem_addr  16-bit register address (MSB first)
 * @param  data      Payload bytes to transmit
 * @param  len       Number of payload bytes (total frame = 2 + len, max 253)
 * @retval I2C_OK on success
 */
I2C_Status_TypeDef I2C_Mem_Write(I2C_TypeDef *I2CX,
                                 uint16_t dev_addr,
                                 uint16_t mem_addr,
                                 const uint8_t *data,
                                 uint16_t len)
{
    I2C_Status_TypeDef ret;
    uint16_t total = 2U + len;

    if (data == NULL || len == 0 || total > 255U)
        return I2C_ERROR;

    ret = I2C_WaitNotBusy(I2CX);
    if (ret != I2C_OK)
        return ret;

    I2CX->ICR = I2C_ICR_STOPCF
              | I2C_ICR_NACKCF
              | I2C_ICR_BERRCF
              | I2C_ICR_ARLOCF
              | I2C_ICR_OVRCF;

    I2CX->CR2 = 0;

    I2CX->CR2 = I2C_7BIT_ADDR(dev_addr)
              | ((uint32_t)total << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START
              | I2C_CR2_AUTOEND;

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr >> 8);

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr & 0xFF);

    for (uint16_t i = 0; i < len; i++) {
        ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
        if (ret != I2C_OK) return ret;
        I2CX->TXDR = data[i];
    }

    ret = I2C_WaitFlag(I2CX, I2C_ISR_STOPF, 1);
    if (ret != I2C_OK) return ret;
    I2CX->ICR = I2C_ICR_STOPCF;

    return I2C_OK;
}

/**
 * @brief  Read data from a device at a 16-bit register address
 *
 * @param  I2CX      I2C instance
 * @param  dev_addr  7-bit slave address
 * @param  mem_addr  16-bit register address (MSB first)
 * @param  data      Destination buffer for received bytes
 * @param  len       Number of bytes to read (max 255)
 * @retval I2C_OK on success
 */
I2C_Status_TypeDef I2C_Mem_Read(I2C_TypeDef *I2CX,
                                uint16_t dev_addr,
                                uint16_t mem_addr,
                                uint8_t *data,
                                uint16_t len)
{
    I2C_Status_TypeDef ret;

    if (data == NULL || len == 0 || len > 255U)
        return I2C_ERROR;

    ret = I2C_WaitNotBusy(I2CX);
    if (ret != I2C_OK)
        return ret;

    I2CX->ICR = I2C_ICR_STOPCF
              | I2C_ICR_NACKCF
              | I2C_ICR_BERRCF
              | I2C_ICR_ARLOCF
              | I2C_ICR_OVRCF;

    /* Phase 1: write 16-bit register address, keep bus (SOFTEND) */
    I2CX->CR2 = 0;
    I2CX->CR2 = I2C_7BIT_ADDR(dev_addr)
              | (2U << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START;

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr >> 8);

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr & 0xFF);

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TC, 1);
    if (ret != I2C_OK) return ret;

    /* Phase 2: read len bytes, AUTOEND sends STOP after last byte */
    I2CX->CR2 = I2C_7BIT_ADDR(dev_addr)
              | ((uint32_t)len << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_START
              | I2C_CR2_AUTOEND
              | I2C_CR2_RD_WRN;

    for (uint16_t i = 0; i < len; i++) {
        ret = I2C_WaitFlag(I2CX, I2C_ISR_RXNE, 1);
        if (ret != I2C_OK) return ret;
        data[i] = (uint8_t)I2CX->RXDR;
    }

    ret = I2C_WaitFlag(I2CX, I2C_ISR_STOPF, 1);
    if (ret != I2C_OK) return ret;
    I2CX->ICR = I2C_ICR_STOPCF;

    return I2C_OK;
}

/**
 * @brief  Check whether a slave responds at a given 7-bit address
 *
 * @param  I2CX     I2C instance
 * @param  dev_addr 7-bit address to probe
 * @retval I2C_OK   device present (ACK received)
 * @retval I2C_NACK no device at this address
 * @retval I2C_TIMEOUT bus stuck busy
 */
I2C_Status_TypeDef I2C_IsDeviceReady(I2C_TypeDef *I2CX, uint16_t dev_addr)
{
    I2CX->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;

    I2CX->CR2 &= ~(I2C_CR2_SADD  | I2C_CR2_NBYTES | I2C_CR2_RELOAD  |
                   I2C_CR2_AUTOEND | I2C_CR2_START | I2C_CR2_STOP    |
                   I2C_CR2_RD_WRN);
    I2CX->CR2 |= I2C_7BIT_ADDR(dev_addr) | I2C_CR2_START | I2C_CR2_AUTOEND;

    uint32_t timeout = 200;
    while (timeout--) {
        uint32_t isr = I2CX->ISR;

        if (isr & I2C_ISR_NACKF) {
            I2CX->ICR = I2C_ICR_NACKCF;
            if (isr & I2C_ISR_STOPF)
                I2CX->ICR = I2C_ICR_STOPCF;
            return I2C_NACK;
        }
        if (isr & I2C_ISR_STOPF) {
            I2CX->ICR = I2C_ICR_STOPCF;
            return I2C_OK;
        }
    }
    return I2C_TIMEOUT;
}

/**
 * @brief  Scan the entire 7-bit address space and record responding slaves
 *
 * @param  I2CX         I2C instance
 * @param  found_addrs  Buffer written with addresses that ACKed (min 128 bytes)
 * @param  count        Output: number of addresses found
 */
void I2C_Scan(I2C_TypeDef *I2CX, uint8_t *found_addrs, uint32_t *count)
{
    *count = 0;
    for (uint16_t addr = 1; addr <= 0x7F; addr++) {
        if (I2C_IsDeviceReady(I2CX, addr) == I2C_OK) {
            found_addrs[(*count)++] = (uint8_t)addr;
        }
    }
}
