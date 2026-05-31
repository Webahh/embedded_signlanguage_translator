/*
 * simple.i2c.h
 *
 *  Created on: 21.05.2026
 *      Author: Weber
 */

#ifndef SIMPLE_I2C_H
#define SIMPLE_I2C_H

#include "stm32n657xx.h"

typedef enum {
	I2C_OK = 0,
	I2C_ERROR,
	I2C_BUSY,
	I2C_TIMEOUT,
	I2C_NACK,
} I2C_Status_TypeDef;

void I2C_Config(I2C_TypeDef* I2CX, uint32_t clock_source, uint32_t timing);
I2C_Status_TypeDef I2C_Mem_Write(I2C_TypeDef* I2CX, uint16_t dev_addr, uint16_t mem_addr,const uint8_t *data, uint16_t len);
I2C_Status_TypeDef I2C_Mem_Read(I2C_TypeDef* I2CX, uint16_t dev_addr, uint16_t mem_addr, uint8_t *data, uint16_t len);

#endif /* SIMPLE_I2C */
