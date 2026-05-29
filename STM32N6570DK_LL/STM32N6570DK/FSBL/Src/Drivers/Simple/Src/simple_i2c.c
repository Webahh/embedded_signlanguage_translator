/*
 * simple_i2c.c
 *
 *  Created on: 21.05.2026
 *      Author: Weber
 */

#include "simple_i2c.h"
#include "simple_rcc.h"
#include <stddef.h>

#define I2C_TIMEOUT_MAX 1000

void I2C_Config(I2C_TypeDef* I2CX, uint32_t clock_source, uint32_t timing){
	RCC_setI2C_clock_source(I2CX, clock_source);
	RCC_enable_I2C(I2CX);
	RCC_reset_I2C(I2CX);

	I2CX->CR1 &= ~I2C_CR1_PE; // safety disable for config changes!

    I2CX->CR1 &= ~I2C_CR1_ANFOFF;
    I2CX->CR1 &= ~I2C_CR1_DNF;
	I2CX->TIMINGR = timing;
	I2CX->CR1 &= ~I2C_CR1_NOSTRETCH;

	I2CX->CR1 |= I2C_CR1_PE; // enable after config changes!
}

static I2C_Status_TypeDef I2C_WaitFlag(I2C_TypeDef *I2CX,
                                        uint32_t flag,
                                        uint8_t set)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;

    while (timeout--) {
        uint32_t isr = I2CX->ISR;

        if (isr & I2C_ISR_NACKF) {
            I2CX->ICR = I2C_ICR_NACKCF;

            if (isr & I2C_ISR_STOPF) {
                I2CX->ICR = I2C_ICR_STOPCF;
            }

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

static I2C_Status_TypeDef I2C_WaitNotBusy(I2C_TypeDef *I2CX)
{
    uint32_t timeout = I2C_TIMEOUT_MAX;

    while (timeout--) {
        if ((I2CX->ISR & I2C_ISR_BUSY) == 0)
            return I2C_OK;
    }

    return I2C_TIMEOUT;
}

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

    I2CX->CR2 =
        ((uint32_t)(dev_addr << 1) << I2C_CR2_SADD_Pos) |
        ((uint32_t)total << I2C_CR2_NBYTES_Pos) |
        I2C_CR2_START |
        I2C_CR2_AUTOEND;

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

    I2CX->CR2 =
        ((uint32_t)(dev_addr << 1) << I2C_CR2_SADD_Pos) |
        (2UL << I2C_CR2_NBYTES_Pos) |
        I2C_CR2_START;

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr >> 8);

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TXIS, 1);
    if (ret != I2C_OK) return ret;
    I2CX->TXDR = (uint8_t)(mem_addr & 0xFF);

    ret = I2C_WaitFlag(I2CX, I2C_ISR_TC, 1);
    if (ret != I2C_OK) return ret;

    I2CX->CR2 =
        ((uint32_t)(dev_addr << 1) << I2C_CR2_SADD_Pos) |
        ((uint32_t)len << I2C_CR2_NBYTES_Pos) |
        I2C_CR2_START |
        I2C_CR2_AUTOEND |
        I2C_CR2_RD_WRN;

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
