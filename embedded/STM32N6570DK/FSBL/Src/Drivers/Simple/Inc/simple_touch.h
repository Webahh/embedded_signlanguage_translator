/**
 * @file    simple_touch.h
 * @author  Gross
 * @date    24.05.2026
 * @brief   GT911 capacitive touch controller driver header
 *
 * Usage
 * -----
 * 1. TOUCH_ConfigIO()     – configure GPIO, I2C2, reset controller
 * 2. TOUCH_Probe()        – probe GT911 on I2C2, verify chip ID
 * 3. TOUCH_Init()         – write chip-specific config registers
 * 4. TOUCH_GetState()     – poll touch coordinates
 */

#ifndef SIMPLE_TOUCH_H
#define SIMPLE_TOUCH_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

typedef enum {
    TOUCH_OK     = 0,
    TOUCH_ERROR  = -1
} TOUCH_Status_TypeDef;

/** Touch controller instance handle */
typedef struct {
    I2C_TypeDef     *i2c;           /**< I2C interface for communication */
    uint8_t          addr;          /**< 7-bit I2C slave address         */
    uint8_t          initialized;   /**< Device initialized flag         */
} TOUCH_Handle_TypeDef;

// =====================================================================
// GT911 (Goodix) — primary touch controller on STM32N6570-DK
// =====================================================================

#define GT911_I2C_ADDR              0x5D
#define GT911_I2C_ADDR_ALT          0x14   /**< Alternative address     */

#define GT911_REG_CMD               0x8040
#define GT911_CMD_READ              0x00

#define GT911_REG_CHIP_ID_H         0x8140

#define GT911_REG_CONFIG_VERSION    0x8047
#define GT911_REG_CONFIG_FRESH      0x8100

#define GT911_REG_TD_STATUS         0x814E

#define GT911_REG_TOUCH1_XL         0x8150
#define GT911_REG_TOUCH1_XH         0x8151
#define GT911_REG_TOUCH1_YL         0x8152
#define GT911_REG_TOUCH1_YH         0x8153
#define GT911_REG_TOUCH1_SIZE       0x8154

#define GT911_REG_TOUCH2_XL         0x8156
#define GT911_REG_TOUCH2_XH         0x8157
#define GT911_REG_TOUCH2_YL         0x8158
#define GT911_REG_TOUCH2_YH         0x8159
#define GT911_REG_TOUCH2_SIZE       0x815A

#define GT911_MAX_TOUCHES           5

// =====================================================================
// Common pins
// =====================================================================

#define TS_I2C          I2C2
#define TS_I2C_SCL_PIN  14
#define TS_I2C_SDA_PIN  4
#define TS_I2C_PORT     GPIOD

#define TS_RST_PORT     GPIOE
#define TS_RST_PIN      1

#define TS_INT_PORT     GPIOQ
#define TS_INT_PIN      4

#define TS_I2C_CLK_SRC  0   /**< RCC_I2C2CLKSOURCE_PCLK1 */

// =====================================================================
// Public API
// =====================================================================

/**
 * @brief  Configure all touch-related GPIOs and initialise I2C2
 *
 * Enables clocks for GPIOD/E/Q, configures I2C2 pins (PD14/PD4),
 * resets the touch controller (PE1 pulse), configures interrupt pin
 * (PQ4), and calls I2C_Config for I2C2.
 *
 * Must be called once before TOUCH_Probe().
 */
void TOUCH_ConfigIO(void);

/**
 * @brief  Probe the GT911 touch controller on I2C2
 *
 * @param  h   Touch handle (output: i2c/addr/chip filled)
 * @param  i2c I2C instance pointer (pass I2C2)
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR if GT911 does not respond
 */
TOUCH_Status_TypeDef TOUCH_Probe(TOUCH_Handle_TypeDef *h, I2C_TypeDef *i2c);

/**
 * @brief  Initialise the GT911
 *
 * Writes init registers and sets operating mode.
 * Must be called after TOUCH_Probe() succeeds.
 *
 * @param  h Touch handle
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR if h is NULL or not initialised
 */
TOUCH_Status_TypeDef TOUCH_Init(TOUCH_Handle_TypeDef *h);

/**
 * @brief  Read GT911 chip ID (4 bytes at 0x8140)
 *
 * @param  h  Touch handle
 * @param  id Output: chip ID as big-endian uint32 (e.g. 0x39313100)
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR on I2C read failure
 */
TOUCH_Status_TypeDef TOUCH_ReadID(TOUCH_Handle_TypeDef *h,
                                  uint32_t *id);

/**
 * @brief  Read the current touch state (single-touch, GT911)
 *
 * @param  h        Touch handle
 * @param  x        Output: X coordinate
 * @param  y        Output: Y coordinate
 * @param  pressed  Output: 1 if touched, 0 if not
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR on I2C read failure
 */
TOUCH_Status_TypeDef TOUCH_GetState(TOUCH_Handle_TypeDef *h,
                                    uint16_t *x, uint16_t *y,
                                    uint8_t *pressed);

/**
 * @brief  Read an 8-bit register via I2C
 *
 * @param  h   Touch handle
 * @param  reg 16-bit register address
 * @param  val Output: register value
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR on I2C error or NULL pointer
 */
TOUCH_Status_TypeDef TOUCH_ReadReg(TOUCH_Handle_TypeDef *h,
                                   uint16_t reg, uint8_t *val);

/**
 * @brief  Write an 8-bit register via I2C
 *
 * @param  h   Touch handle
 * @param  reg 16-bit register address
 * @param  val Value to write
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR on I2C error
 */
TOUCH_Status_TypeDef TOUCH_WriteReg(TOUCH_Handle_TypeDef *h,
                                    uint16_t reg, uint8_t val);

#endif /* SIMPLE_TOUCH_H */
