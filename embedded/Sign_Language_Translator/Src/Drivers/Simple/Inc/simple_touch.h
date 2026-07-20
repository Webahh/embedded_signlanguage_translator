/**
 * @file    simple_touch.h
 * @author  Gross
 * @date    24.05.2026
 * @brief   GT911 capacitive touch controller driver header
 *
 * Usage
 * -----
 * 1. TOUCH_ConfigIO()     - configure GPIO, I2C2, reset controller
 * 2. TOUCH_Probe()        - probe GT911 on I2C2, verify chip ID
 * 3. TOUCH_Init()         - write chip-specific config registers
 * 4. TOUCH_GetState()     - poll touch coordinates
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

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t pressed;
} TOUCH_Data_TypeDef;

// =====================================================================
// GT911 (Goodix) — primary touch controller on STM32N6570-DK
// =====================================================================

#define GT911_I2C_ADDR              0x5D

#define GT911_REG_MSW1              0x804D
#define GT911_REG_CONFIG_CHKSUM     0x80FF
#define GT911_REG_CONFIG_FRESH      0x8100

#define GT911_REG_CHIP_ID_H         0x8140

#define GT911_REG_TD_STATUS         0x814E

#define GT911_REG_TOUCH1_XL         0x8150
#define GT911_REG_TOUCH1_XH         0x8151
#define GT911_REG_TOUCH1_YL         0x8152
#define GT911_REG_TOUCH1_YH         0x8153

#define TS_I2C  I2C2

// =====================================================================
// API
// =====================================================================

/**
 * @brief  Check if a touch interrupt is pending
 *
 * @param [out] p	| pending flag [0,1]
 */
void TOUCH_GetPending(uint8_t *p);

/**
 * @brief  Configure all touch-related GPIOs and initialise I2C2
 */
void TOUCH_ConfigIO(void);

/**
 * @brief  Probe the GT911 touch controller on I2C2
 *
 * @param [in]  h	| Touch handle (output: i2c/addr/chip filled)
 * @param [in]	i2c	| I2C instance pointer (pass I2C2)
 *
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
 * @param [in] h	| Touch handle
 *
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR if h is NULL or not initialised
 */
TOUCH_Status_TypeDef TOUCH_Init(TOUCH_Handle_TypeDef *h);

/**
 * @brief  Read the current touch state (single-touch, GT911)
 *
 * Used in a Task in order to fetch from interrupt provided touch data;
 *
 * @param [in]  h		| Touch handle (unused, kept for API compat)
 * @param [out] data	| touch data
 *
 * @retval TOUCH_OK    on success
 * @retval TOUCH_ERROR on NULL pointer
 */
TOUCH_Status_TypeDef TOUCH_GetState(TOUCH_Handle_TypeDef *h, TOUCH_Data_TypeDef *data);

#endif /* SIMPLE_TOUCH_H */
