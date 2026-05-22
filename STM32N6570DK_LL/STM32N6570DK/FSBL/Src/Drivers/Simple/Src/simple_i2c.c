/*
 * simple_i2c.c
 *
 *  Created on: 21.05.2026
 *      Author: Weber
 */

#include "simple_i2c.h"
#include "simple_rcc.h"

void I2C_Config(I2C_TypeDef* I2CX, uint32_t clock_source, uint32_t timing){
	RCC_setI2C_clock_source(I2CX, clock_source);
	RCC_enable_I2C(I2CX);
	RCC_reset_I2C(I2CX);

	I2CX->CR1 &= ~I2C_CR1_PE; // safety disable for config changes!

    I2CX->CR1 &= I2C_CR1_ANFOFF;
    I2CX->CR1 &= ~I2C_CR1_DNF;
	I2CX->TIMINGR = timing;
	I2CX->CR1 &= I2C_CR1_NOSTRETCH;

	I2CX->CR1 |= I2C_CR1_PE; // enable after config changes!
}
