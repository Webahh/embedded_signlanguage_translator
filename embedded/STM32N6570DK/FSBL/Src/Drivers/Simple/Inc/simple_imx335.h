/**
 * @file    simple_imx335.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   Register-level IMX335 (Sony Starvis) sensor driver header
 *
 * Usage
 * -----
 * 1. IMX335_Probe()   - verify sensor ID
 * 2. IMX335_Init()    - initialise sensor registers
 * 3. IMX335_Start()   - start streaming
 */

#ifndef SIMPLE_IMX335_H
#define SIMPLE_IMX335_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

typedef enum {
    IMX335_OK    = 0,
    IMX335_ERROR = -1
} IMX335_Status_TypeDef;

// ---- Register map (chip-ID, control, AE) ----

#define IMX335_REG_ID           0x3912
#define IMX335_CHIP_ID          0x00
#define IMX335_I2C_ADDR         0x1A
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
#define IMX335_REG_AREA3_ST_ADR_1_LSB 0x3074
#define IMX335_REG_AREA3_ST_ADR_1_MSB 0x3075
#define IMX335_REG_AEC          0x3A00
#define IMX335_AEC_ENABLE       0x01
#define IMX335_AEC_DISABLE      0x00

#define IMX335_WIDTH            2592
#define IMX335_HEIGHT           1944
#define IMX335_PCLK             396000000

// ---- Exposure / gain limits (milli-dB gain units) ----

#define IMX335_GAIN_MIN         0
#define IMX335_GAIN_MAX         72000
#define IMX335_GAIN_UNIT_MDB    300
#define IMX335_EXPOSURE_MIN     8
#define IMX335_EXPOSURE_MAX     33266

/** Opaque IMX335 instance handle */
typedef struct {
    I2C_TypeDef *i2c;			/**< I2C interface for communication */
    uint8_t      addr;			/**< Adress of device*/
    uint8_t      initialized;	/**< Device initialized*/
} IMX335_Handle_TypeDef;

// ---- API ----

/**
 * @brief  Probe the sensor: assign I2C bus, read chip ID
 * @param  h    Sensor handle (output: i2c/addr filled)
 * @param  i2c  I2C instance pointer
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR if ID read failed
 */
IMX335_Status_TypeDef IMX335_Probe(IMX335_Handle_TypeDef *h, I2C_TypeDef *i2c);

/**
 * @brief  Initialise the sensor: write all register tables for 2592x1944,
 *         2-lane MIPI, 10-bit, 30 fps
 * @param  h Sensor handle
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR if any register write failed
 */
IMX335_Status_TypeDef IMX335_Init(IMX335_Handle_TypeDef *h);

/**
 * @brief  Start streaming (set mode register to STREAMING)
 * @param  h Sensor handle
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_Start(IMX335_Handle_TypeDef *h);

/**
 * @brief  Stop streaming (set mode register to STANDBY)
 * @param  h Sensor handle
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_Stop(IMX335_Handle_TypeDef *h);

/**
 * @brief  Set framerate by writing VMAX (sets total frame length)
 * @param  h   Sensor handle
 * @param  fps Target framerate: 10, 15, 20, 25, or 30 fps
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_SetFramerate(IMX335_Handle_TypeDef *h, uint32_t fps);

/**
 * @brief  Set mirror / flip configuration
 * @param  h      Sensor handle
 * @param  config 0 = normal, non-zero = mirrored
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_SetMirrorFlip(IMX335_Handle_TypeDef *h, uint32_t config);

/**
 * @brief  Read 8-bit chip ID at register 0x3912
 * @param  h  Sensor handle
 * @param  id Output: chip-ID byte
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR if h/id is NULL or I2C read fails
 */
IMX335_Status_TypeDef IMX335_ReadID(IMX335_Handle_TypeDef *h, uint32_t *id);

/**
 * @brief  Enable the sensor's internal auto-exposure controller
 * @param  h Sensor handle
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_EnableAutoExposure(IMX335_Handle_TypeDef *h);

/**
 * @brief  Verify that all written configuration registers read back correctly
 * @param  h Sensor handle (must have been initialised with IMX335_Init)
 * @retval IMX335_OK    all registers verified
 * @retval IMX335_ERROR at least one register mismatch or I2C error
 */
IMX335_Status_TypeDef IMX335_VerifyConfig(IMX335_Handle_TypeDef *h);

/**
 * @brief  Read a single 8-bit register
 * @param  h   Sensor handle
 * @param  reg 16-bit register address
 * @param  val Output: register value
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error or NULL pointer
 */
IMX335_Status_TypeDef IMX335_ReadReg(IMX335_Handle_TypeDef *h, uint16_t reg, uint8_t *val);

/**
 * @brief  Set exposure time in microseconds
 * @param  h           Sensor handle
 * @param  exposure_us Exposure time in microseconds
 */
void IMX335_SetExposureUs(IMX335_Handle_TypeDef *h, uint32_t exposure_us);

/**
 * @brief  Read and store a predefined set of debug registers
 * @param  h Sensor handle
 */
void IMX335_DumpDebugRegs(IMX335_Handle_TypeDef *h);

/**
 * @brief  Set sensor gain in milli-dB
 * @param  h        Sensor handle
 * @param  gain_mdb Gain in milli-dB (0-72000)
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
IMX335_Status_TypeDef IMX335_SetGainMdB(IMX335_Handle_TypeDef *h, uint32_t gain_mdb);

#endif /* SIMPLE_IMX335_H */
