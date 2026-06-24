/**
 * @file    simple_touch.c
 * @author  Gross
 * @date    24.05.2026
 * @brief   GT911 capacitive touch controller driver for STM32N6570-DK
 */

#include <stdint.h>
#include <stddef.h>

#include "simple_touch.h"
#include "simple_i2c.h"
#include "simple_timer.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "config.h"

// =====================================================================
// Public: register R/W
// =====================================================================

TOUCH_Status_TypeDef TOUCH_ReadReg(TOUCH_Handle_TypeDef *h,
                                   uint16_t reg, uint8_t *val)
{
    if (h == NULL || val == NULL)
        return TOUCH_ERROR;

    I2C_Status_TypeDef ret = I2C_Mem_read(h->i2c, h->addr, reg, val, 1);
    return (ret == I2C_OK) ? TOUCH_OK : TOUCH_ERROR;
}

TOUCH_Status_TypeDef TOUCH_WriteReg(TOUCH_Handle_TypeDef *h,
                                    uint16_t reg, uint8_t val)
{
    if (h == NULL)
        return TOUCH_ERROR;

    I2C_Status_TypeDef ret = I2C_Mem_write(h->i2c, h->addr, reg, &val, 1);
    return (ret == I2C_OK) ? TOUCH_OK : TOUCH_ERROR;
}

// =====================================================================
// GPIO + I2C configuration
// =====================================================================

void TOUCH_ConfigIO(void)
{
    RCC_enable_GPIO(GPIOD);
    RCC_enable_GPIO(GPIOE);
    RCC_enable_GPIO(GPIOQ);

    GPIO_Config(GPIOD, 14, GPIO_I2C_cfg);   /* SCL */
    GPIO_Config(GPIOD,  4, GPIO_I2C_cfg);   /* SDA */

    GPIO_Config(GPIOE, 1, GPIO_TS_RST_cfg);
    GPIO_BSRR_reset(GPIOE, 1);
    TIMER_Delay_ms(10);
    GPIO_BSRR_set(GPIOE, 1);
    TIMER_Delay_ms(100);

    GPIO_Config(GPIOQ, 4, GPIO_TS_INT_cfg);

    I2C_Config(I2C2, 0, I2C_SENSOR_BUS_TIMING);
}

// =====================================================================
// Probe
// =====================================================================

TOUCH_Status_TypeDef TOUCH_Probe(TOUCH_Handle_TypeDef *h, I2C_TypeDef *i2c)
{
    if (h == NULL || i2c == NULL)
        return TOUCH_ERROR;

    h->i2c         = i2c;
    h->addr        = GT911_I2C_ADDR;
    h->initialized = 0;

    uint8_t id[4];
    if (I2C_Mem_read(i2c, h->addr, GT911_REG_CHIP_ID_H, id, 4) != I2C_OK)
        return TOUCH_ERROR;

    if (id[0] == '9' && id[1] == '1' && id[2] == '1' && id[3] == '\0')
    {
        h->initialized = 1;
        return TOUCH_OK;
    }

    return TOUCH_ERROR;
}

// =====================================================================
// Read ID
// =====================================================================

TOUCH_Status_TypeDef TOUCH_ReadID(TOUCH_Handle_TypeDef *h,
                                  uint32_t *id)
{
    if (h == NULL || id == NULL)
        return TOUCH_ERROR;

    uint8_t buf[4];
    if (I2C_Mem_read(h->i2c, h->addr, GT911_REG_CHIP_ID_H, buf, 4) != I2C_OK)
        return TOUCH_ERROR;

    *id = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
        | ((uint32_t)buf[2] << 8)  | (uint32_t)buf[3];
    return TOUCH_OK;
}

// =====================================================================
// Init
// =====================================================================

TOUCH_Status_TypeDef TOUCH_Init(TOUCH_Handle_TypeDef *h)
{
    if (h == NULL || !h->initialized)
        return TOUCH_ERROR;

    TOUCH_WriteReg(h, GT911_REG_CMD, GT911_CMD_READ);
    TIMER_Delay_ms(10);

    return TOUCH_OK;
}

// =====================================================================
// Get state
// =====================================================================

TOUCH_Status_TypeDef TOUCH_GetState(TOUCH_Handle_TypeDef *h,
                                    uint16_t *x, uint16_t *y,
                                    uint8_t *pressed)
{
    if (h == NULL || x == NULL || y == NULL || pressed == NULL)
        return TOUCH_ERROR;

    uint8_t status = 0;
    if (TOUCH_ReadReg(h, GT911_REG_TD_STATUS, &status) != TOUCH_OK)
        return TOUCH_ERROR;

    status &= 0x0F;
    if (status == 0)
    {
        *pressed = 0;
        *x = *y = 0;
        return TOUCH_OK;
    }

    *pressed = 1;

    uint8_t buf[4];
    if (I2C_Mem_read(h->i2c, h->addr, GT911_REG_TOUCH1_XL,
                     buf, 4) != I2C_OK)
        return TOUCH_ERROR;

    *x = (uint16_t)buf[0] | ((uint16_t)(buf[1] & 0x0F) << 8);
    *y = (uint16_t)buf[2] | ((uint16_t)(buf[3] & 0x0F) << 8);

    return TOUCH_OK;
}
