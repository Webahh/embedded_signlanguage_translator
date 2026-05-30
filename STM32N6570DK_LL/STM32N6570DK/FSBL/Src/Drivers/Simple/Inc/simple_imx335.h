/*
 * simple_imx335.h
 *
 * Register-level imx335 (sensor) driver
 *
 *  Created on: May 29, 2026
 *      Author: Groß
 */

#ifndef SIMPLE_IMX335_H
#define SIMPLE_IMX335_H

#include "stm32n657xx.h"

#define IMX335_REG_ID      0x3912U
#define IMX335_CHIP_ID     0x00U

#define IMX335_I2C_ADDR         0x1AU

#define IMX335_REG_MODE_SELECT  0x3000
#define IMX335_MODE_STREAMING   0x00
#define IMX335_MODE_STANDBY     0x01
#define IMX335_REG_HOLD         0x3001
#define IMX335_REG_VMAX         0x3030
#define IMX335_REG_SHUTTER      0x3058
#define IMX335_REG_GAIN         0x30E8
#define IMX335_REG_TPG          0x329E
#define IMX335_REG_HREVERSE     0x304E
#define IMX335_REG_VREVERSE     0x304F
#define IMX335_REG_AEC          0x3A00
#define IMX335_AEC_ENABLE       0x01
#define IMX335_AEC_DISABLE      0x00

#define IMX335_GAIN_MIN         0
#define IMX335_GAIN_MAX         72000
#define IMX335_GAIN_UNIT_MDB    300
#define IMX335_EXPOSURE_MIN     8
#define IMX335_EXPOSURE_MAX     33266

typedef struct {
    I2C_TypeDef *i2c;
    uint8_t addr;
    uint8_t initialized;
} IMX335_Handle;

int32_t IMX335_Probe(IMX335_Handle *h, I2C_TypeDef *i2c);
int32_t IMX335_Init(IMX335_Handle *h);
int32_t IMX335_Start(IMX335_Handle *h);
int32_t IMX335_Stop(IMX335_Handle *h);
int32_t IMX335_SetFramerate(IMX335_Handle *h, uint32_t fps);
int32_t IMX335_SetMirrorFlip(IMX335_Handle *h, uint32_t config);
int32_t IMX335_ReadID(IMX335_Handle *h, uint32_t *id);
int32_t IMX335_EnableAutoExposure(IMX335_Handle *h);

#endif
