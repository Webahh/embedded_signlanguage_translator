/**
 * @file    simple_i2c.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   I2C bus driver source
 */

#include <stddef.h>

#include "simple_i2c.h"
#include "simple_rcc.h"

// ---- Private defines ----

#define _TIMEOUT_MAX  1000U

#define _7BIT_ADDR(addr7) (((uint32_t)(addr7) & 0x7FU) << 1)

// ---- Private helpers ----

/**
 * @brief  Wait for an ISR flag to be set or cleared
 * @param [in] I2CX  | I2C instance
 * @param [in] flag  | ISR bit to test (e.g. I2C_ISR_TXIS)
 * @param [in] set   | 1 = wait until flag is set, 0 = wait until flag is cleared
 * @retval I2C_OK      Flag reached expected state
 * @retval I2C_NACK    Slave NACK detected
 * @retval I2C_ERROR   Arbitration lost, bus error, or overrun
 * @retval I2C_TIMEOUT Loop expired
 */
static I2C_Status_TypeDef I2C_WaitFlag(I2C_TypeDef *I2CX, uint32_t flag, uint8_t set){
    uint32_t timeout = _TIMEOUT_MAX;

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
 * @param [in] I2CX | I2C instance
 * @retval I2C_OK      Bus became free
 * @retval I2C_TIMEOUT Bus stayed busy
 */
static I2C_Status_TypeDef I2C_WaitNotBusy(I2C_TypeDef *I2CX){
    uint32_t timeout = _TIMEOUT_MAX;

    while (timeout--) {
        if ((I2CX->ISR & I2C_ISR_BUSY) == 0)
            return I2C_OK;
    }

    return I2C_TIMEOUT;
}

// ---- API ----

void I2C_Config(I2C_TypeDef *I2CX, uint32_t clock_source, uint32_t timing){
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

I2C_Status_TypeDef I2C_Mem_write(I2C_TypeDef *I2CX,
                                 uint16_t dev_addr,
                                 uint16_t mem_addr,
                                 const uint8_t *data,
                                 uint16_t len){
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

    I2CX->CR2 = _7BIT_ADDR(dev_addr)
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

I2C_Status_TypeDef I2C_Mem_read(I2C_TypeDef *I2CX,
                                uint16_t dev_addr,
                                uint16_t mem_addr,
                                uint8_t *data,
                                uint16_t len){
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

    I2CX->CR2 = 0;
    I2CX->CR2 = _7BIT_ADDR(dev_addr)
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

    I2CX->CR2 = _7BIT_ADDR(dev_addr)
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

I2C_Status_TypeDef I2C_Device_ready(I2C_TypeDef *I2CX, uint16_t dev_addr){
    I2CX->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;

    I2CX->CR2 &= ~(I2C_CR2_SADD  | I2C_CR2_NBYTES | I2C_CR2_RELOAD  |
                   I2C_CR2_AUTOEND | I2C_CR2_START | I2C_CR2_STOP    |
                   I2C_CR2_RD_WRN);
    I2CX->CR2 |= _7BIT_ADDR(dev_addr) | I2C_CR2_START | I2C_CR2_AUTOEND;

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

void I2C_Scan(I2C_TypeDef *I2CX, uint8_t *found_addrs, uint32_t *count){
    *count = 0;
    for (uint16_t addr = 1; addr <= 0x7F; addr++) {
        if (I2C_Device_ready(I2CX, addr) == I2C_OK) {
            found_addrs[(*count)++] = (uint8_t)addr;
        }
    }
}
