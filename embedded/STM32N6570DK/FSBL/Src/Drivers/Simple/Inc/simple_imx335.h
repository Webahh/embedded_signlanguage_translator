/**
  ******************************************************************************
  * @file    simple_imx335.h
  * @author  Groß
  * @brief   Register-level IMX335 (Sony Starvis) sensor driver header
  ******************************************************************************
  */

#ifndef SIMPLE_IMX335_H
#define SIMPLE_IMX335_H

#include "stm32n657xx.h"

/* ---------------------------------------------------------------------------
 * Register map (chip-ID, control, AE)
 * ------------------------------------------------------------------------- */

/** Chip-ID register (8-bit, read-only, value = 0x00) */
#define IMX335_REG_ID           0x3912
/** Expected chip-ID value */
#define IMX335_CHIP_ID          0x00

/** Sensor I2C address (7-bit) */
#define IMX335_I2C_ADDR         0x1A

/** Mode-select register */
#define IMX335_REG_MODE_SELECT  0x3000
#define IMX335_MODE_STREAMING   0x00
#define IMX335_MODE_STANDBY     0x01

/** Register-hold (group write) control */
#define IMX335_REG_HOLD         0x3001

/** Frame-length (VMAX) for framerate control */
#define IMX335_REG_VMAX         0x3030

/** Shutter (exposure) register */
#define IMX335_REG_SHUTTER      0x3058
/** Gain register */
#define IMX335_REG_GAIN         0x30E8
/** Test-pattern generator */
#define IMX335_REG_TPG          0x329E

/** Horizontal/vertical flip */
#define IMX335_REG_HREVERSE     0x304E
#define IMX335_REG_VREVERSE     0x304F
#define IMX335_REG_AREA3_ST_ADR_1_LSB 0x3074
#define IMX335_REG_AREA3_ST_ADR_1_MSB 0x3075

/** Auto-exposure control */
#define IMX335_REG_AEC          0x3A00
#define IMX335_AEC_ENABLE       0x01
#define IMX335_AEC_DISABLE      0x00

#define IMX335_WIDTH              2592
#define IMX335_HEIGHT             1944
#define IMX335_PCLK               396000000

/* ---------------------------------------------------------------------------
 * Exposure / gain limits (milli-dB gain units)
 * ------------------------------------------------------------------------- */

#define IMX335_GAIN_MIN         0
#define IMX335_GAIN_MAX         72000
#define IMX335_GAIN_UNIT_MDB    300
#define IMX335_EXPOSURE_MIN     8
#define IMX335_EXPOSURE_MAX     33266

/** Opaque IMX335 instance handle */
typedef struct {
    I2C_TypeDef *i2c;          /**< I2C peripheral (e.g. I2C1)              */
    uint8_t      addr;         /**< 7-bit I2C address                       */
    uint8_t      initialized;  /**< Non-zero after IMX335_Init              */
} IMX335_Handle;

/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
  * @brief  Probe the sensor: assign I2C bus, read chip ID
  * @param  h    Sensor handle (output: i2c/addr filled)
  * @param  i2c  I2C instance pointer
  * @retval 0 on success, -1 if ID read failed
  */
int32_t IMX335_Probe(IMX335_Handle *h, I2C_TypeDef *i2c);

/**
  * @brief  Initialise the sensor: write all register tables for 2592x1944,
  *         2-lane MIPI, 10-bit, 30 fps
  * @param  h Sensor handle
  * @retval 0 on success, -1 if any register write failed
  */
int32_t IMX335_Init(IMX335_Handle *h);

/**
  * @brief  Start streaming (set mode register to STREAMING)
  * @param  h Sensor handle
  * @retval 0 on success, -1 on I2C error
  */
int32_t IMX335_Start(IMX335_Handle *h);

/**
  * @brief  Stop streaming (set mode register to STANDBY)
  * @param  h Sensor handle
  * @retval 0 on success, -1 on I2C error
  */
int32_t IMX335_Stop(IMX335_Handle *h);

/**
  * @brief  Set framerate by writing VMAX (sets total frame length)
  * @param  h   Sensor handle
  * @param  fps Target framerate: 10, 15, 20, 25, or 30 fps
  * @retval 0 on success, -1 on I2C error
  */
int32_t IMX335_SetFramerate(IMX335_Handle *h, uint32_t fps);

/**
  * @brief  Set mirror / flip configuration
  * @param  h      Sensor handle
  * @param  config 0 = normal, non-zero = mirrored
  * @retval 0 on success, -1 on I2C error
  */
int32_t IMX335_SetMirrorFlip(IMX335_Handle *h, uint32_t config);

/**
  * @brief  Read 8-bit chip ID at register 0x3912
  * @param  h  Sensor handle
  * @param  id Output: chip-ID byte
  * @retval 0 on success, -1 if h/id is NULL or I2C read fails
  */
int32_t IMX335_ReadID(IMX335_Handle *h, uint32_t *id);

/**
  * @brief  Enable the sensor's internal auto-exposure controller
  * @param  h Sensor handle
  * @retval 0 on success, -1 on I2C error
  */
int32_t IMX335_EnableAutoExposure(IMX335_Handle *h);

/**
  * @brief  Verify that all written configuration registers read back correctly
  *         Iterates every register from all init tables, reads it back,
  *         and compares against the written value
  * @param  h Sensor handle (must have been initialised with IMX335_Init)
  * @retval 0     all registers verified
  * @retval -1    at least one register mismatch or I2C error
  */
int32_t IMX335_VerifyConfig(IMX335_Handle *h);

int32_t IMX335_ReadReg(IMX335_Handle *h, uint16_t reg, uint8_t *val);

void IMX335_SetExposureUs(IMX335_Handle *h, uint32_t exposure_us);

void IMX335_DumpDebugRegs(IMX335_Handle *h);

int32_t IMX335_SetGainMdB(IMX335_Handle *h, uint32_t gain_mdb);

#endif /* SIMPLE_IMX335_H */
