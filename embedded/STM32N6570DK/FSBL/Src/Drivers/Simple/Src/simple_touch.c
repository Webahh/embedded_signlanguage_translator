#include <stdint.h>
#include <stddef.h>

#include "simple_touch.h"
#include "simple_i2c.h"
#include "simple_timer.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "config.h"
#include "stm32n657xx.h"

#define CONFIG_START     0x8047
#define CONFIG_END       0x80FF  /* exclusive: sum covers 0x8047..0x80FE */
#define CONFIG_LEN       (CONFIG_END - CONFIG_START)

#define X_RES_L          (CONFIG_START + 1)
#define X_RES_H          (CONFIG_START + 2)
#define Y_RES_L          (CONFIG_START + 3)
#define Y_RES_H          (CONFIG_START + 4)
#define MSW1_REG         CONFIG_START + 6

#define X_RES_VAL        800
#define Y_RES_VAL        480
#define MSW1_VAL         0x0C  /* bits 2|3 = Y/X reverse (Max-Min) */

#define CONFIG_VERSION   0x82

static uint8_t _config_buf[CONFIG_LEN];

static volatile uint8_t  _touch_pending;
static volatile uint16_t _saved_x;
static volatile uint16_t _saved_y;
static volatile uint8_t  _saved_pressed;

static uint8_t _calc_checksum(const uint8_t *buf, uint16_t len)
{
    uint8_t sum = 0;
    for (uint16_t i = 0; i < len; i++)
        sum += buf[i];
    return ((~sum) + 1) & 0xFF;
}

static TOUCH_Status_TypeDef _read_config(TOUCH_Handle_TypeDef *h)
{
    if (I2C_Mem_read(h->i2c, h->addr, CONFIG_START, _config_buf, CONFIG_LEN) != I2C_OK)
        return TOUCH_ERROR;
    return TOUCH_OK;
}

static TOUCH_Status_TypeDef _write_config(TOUCH_Handle_TypeDef *h)
{
    if (I2C_Mem_write(h->i2c, h->addr, CONFIG_START, _config_buf, CONFIG_LEN) != I2C_OK)
        return TOUCH_ERROR;
    return TOUCH_OK;
}

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

void TOUCH_ConfigIO(void)
{
    RCC_enable_GPIO(GPIOD);
    RCC_enable_GPIO(GPIOE);
    RCC_enable_GPIO(GPIOQ);

    GPIO_Config(GPIOD, 14, GPIO_I2C_cfg);
    GPIO_Config(GPIOD,  4, GPIO_I2C_cfg);

    GPIO_Config(GPIOE, 1, GPIO_TS_RST_cfg);
    GPIO_BSRR_reset(GPIOE, 1);
    TIMER_Delay_ms(10);
    GPIO_BSRR_set(GPIOE, 1);
    TIMER_Delay_ms(100);

    GPIO_Config(GPIOQ, 4, GPIO_TS_INT_cfg);

    I2C_Config(I2C2, 0, I2C_SENSOR_BUS_TIMING);

    _touch_pending = 0;
    _saved_x = 0;
    _saved_y = 0;
    _saved_pressed = 0;

    EXTI->RPR1 = EXTI_RPR1_RPIF4;

    EXTI->EXTICR[1] = (EXTI->EXTICR[1] & ~EXTI_EXTICR2_EXTI4) | 0x0BU;
    EXTI->RTSR1 |= EXTI_RTSR1_RT4;
    EXTI->IMR1  |= EXTI_IMR1_IM4;

    NVIC_SetPriority(EXTI4_IRQn, 0x80);
    NVIC_ClearPendingIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
}

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

uint8_t TOUCH_GetPending(void)
{
    uint8_t p = _touch_pending;
    if (p)
        _touch_pending = 0;
    return p;
}

TOUCH_Status_TypeDef TOUCH_Init(TOUCH_Handle_TypeDef *h)
{
    if (h == NULL || !h->initialized)
        return TOUCH_ERROR;

    uint8_t tmp;

    /* Read factory config */
    if (_read_config(h) != TOUCH_OK)
        return TOUCH_ERROR;

    /* Override config version to force update */
    _config_buf[0] = CONFIG_VERSION;

    /* Override resolution: X=800, Y=480 */
    _config_buf[X_RES_L - CONFIG_START] = X_RES_VAL & 0xFF;
    _config_buf[X_RES_H - CONFIG_START] = (X_RES_VAL >> 8) & 0x0F;
    _config_buf[Y_RES_L - CONFIG_START] = Y_RES_VAL & 0xFF;
    _config_buf[Y_RES_H - CONFIG_START] = (Y_RES_VAL >> 8) & 0x0F;

    /* Force MSW1: rising edge, no Y-reverse, no swap */
    _config_buf[MSW1_REG - CONFIG_START] = MSW1_VAL;

    /* Write patched config */
    if (_write_config(h) != TOUCH_OK)
        return TOUCH_ERROR;

    /* Calculate and write checksum from patched table */
    tmp = _calc_checksum(_config_buf, CONFIG_LEN);
    if (I2C_Mem_write(h->i2c, h->addr, GT911_REG_CONFIG_CHKSUM, &tmp, 1) != I2C_OK)
        return TOUCH_ERROR;

    /* Mark config fresh */
    tmp = 1;
    if (I2C_Mem_write(h->i2c, h->addr, GT911_REG_CONFIG_FRESH, &tmp, 1) != I2C_OK)
        return TOUCH_ERROR;

    /* Factory calibration (non-fatal — matches ST BSP behaviour) */
    I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
    tmp &= ~0x70;
    tmp |= (0x04 << 4);
    I2C_Mem_write(h->i2c, h->addr, 0x00, &tmp, 1);
    TIMER_Delay_ms(300);

    I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
    if (((tmp >> 4) & 0x07) == 0x04)
    {
        tmp = 0x04;
        I2C_Mem_write(h->i2c, h->addr, GT911_REG_TD_STATUS, &tmp, 1);
        TIMER_Delay_ms(300);

        for (uint16_t i = 0; i < 100; i++)
        {
            I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
            if (((tmp >> 4) & 0x07) == 0x00)
                break;
            TIMER_Delay_ms(200);
        }
    }

    tmp = MSW1_VAL;
    I2C_Mem_write(h->i2c, h->addr, GT911_REG_MSW1, &tmp, 1);

    tmp = 0;
    I2C_Mem_write(h->i2c, h->addr, GT911_REG_TD_STATUS, &tmp, 1);

    return TOUCH_OK;
}

void EXTI4_IRQHandler(void)
{
    if (EXTI->RPR1 & EXTI_RPR1_RPIF4)
    {
        EXTI->RPR1 = EXTI_RPR1_RPIF4;

        uint8_t status;
        I2C_Mem_read(I2C2, GT911_I2C_ADDR, GT911_REG_TD_STATUS, &status, 1);

        status &= 0x0F;
        if (status > 0 && status <= 5)
        {
            uint8_t buf[4];
            I2C_Mem_read(I2C2, GT911_I2C_ADDR, GT911_REG_TOUCH1_XL, buf, 4);
            _saved_x = (uint16_t)buf[0] | ((uint16_t)(buf[1] & 0x0F) << 8);
            _saved_y = (uint16_t)buf[2] | ((uint16_t)(buf[3] & 0x0F) << 8);
            _saved_pressed = 1;
        }
        else
        {
            _saved_pressed = 0;
        }

        uint8_t zero = 0;
        I2C_Mem_write(I2C2, GT911_I2C_ADDR, GT911_REG_TD_STATUS, &zero, 1);

        _touch_pending = 1;
    }
}

TOUCH_Status_TypeDef TOUCH_GetState(TOUCH_Handle_TypeDef *h,
                                    uint16_t *x, uint16_t *y,
                                    uint8_t *pressed)
{
    if (x == NULL || y == NULL || pressed == NULL)
        return TOUCH_ERROR;

    *x = _saved_x;
    *y = _saved_y;
    *pressed = _saved_pressed;

    _saved_pressed = 0;

    return TOUCH_OK;
}
