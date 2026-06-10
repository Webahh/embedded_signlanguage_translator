/**
 * @file    simple_imx335.c
 * @brief   Register-level IMX335 (Sony Starvis) sensor driver.
 *
 *          Initialisation register tables are sourced from the STM32
 *          BSP driver and the Linux kernel driver (drivers/media/i2c/imx335.c)
 *
 *          All I2C transactions use 16-bit register addresses and 8-bit data
 *
 *  Created on: May 29, 2026
 *      Author: Groß
 */

#include "stddef.h"
#include "simple_imx335.h"
#include "simple_i2c.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// Register/value pair for table-based writes
struct regval {
    uint16_t addr; /**< 16-bit register address */
    uint8_t  val;  /**< 8-bit value to write    */
};

/** 2592 x 1944 (5 MP) initialisation table */
static const struct regval res_2592_1944_regs[] = {
    {0x3000, 0x01}, {0x3002, 0x00}, {0x300c, 0x3b}, {0x300d, 0x2a},
    {0x3018, 0x04}, {0x302c, 0x3c}, {0x302e, 0x20}, {0x3056, 0x98},
    {0x3074, 0xc8}, {0x3076, 0x30}, {0x304c, 0x00}, {0x314c, 0xc6},
    {0x315a, 0x02}, {0x3168, 0xa0}, {0x316a, 0x7e}, {0x31a1, 0x00},
    {0x3288, 0x21}, {0x328a, 0x02}, {0x3414, 0x05}, {0x3416, 0x18},
    {0x3648, 0x01}, {0x364a, 0x04}, {0x364c, 0x04}, {0x3678, 0x01},
    {0x367c, 0x31}, {0x367e, 0x31}, {0x3706, 0x10}, {0x3708, 0x03},
    {0x3714, 0x02}, {0x3715, 0x02}, {0x3716, 0x01}, {0x3717, 0x03},
    {0x371c, 0x3d}, {0x371d, 0x3f}, {0x372c, 0x00}, {0x372d, 0x00},
    {0x372e, 0x46}, {0x372f, 0x00}, {0x3730, 0x89}, {0x3731, 0x00},
    {0x3732, 0x08}, {0x3733, 0x01}, {0x3734, 0xfe}, {0x3735, 0x05},
    {0x3740, 0x02}, {0x375d, 0x00}, {0x375e, 0x00}, {0x375f, 0x11},
    {0x3760, 0x01}, {0x3768, 0x1b}, {0x3769, 0x1b}, {0x376a, 0x1b},
    {0x376b, 0x1b}, {0x376c, 0x1a}, {0x376d, 0x17}, {0x376e, 0x0f},
    {0x3776, 0x00}, {0x3777, 0x00}, {0x3778, 0x46}, {0x3779, 0x00},
    {0x377a, 0x89}, {0x377b, 0x00}, {0x377c, 0x08}, {0x377d, 0x01},
    {0x377e, 0x23}, {0x377f, 0x02}, {0x3780, 0xd9}, {0x3781, 0x03},
    {0x3782, 0xf5}, {0x3783, 0x06}, {0x3784, 0xa5}, {0x3788, 0x0f},
    {0x378a, 0xd9}, {0x378b, 0x03}, {0x378c, 0xeb}, {0x378d, 0x05},
    {0x378e, 0x87}, {0x378f, 0x06}, {0x3790, 0xf5}, {0x3792, 0x43},
    {0x3794, 0x7a}, {0x3796, 0xa1}, {0x37b0, 0x36}, {0x3a00, 0x01},
};

/** 2-lane / 10-bit MIPI mode selection */
static const struct regval mode_2l_10b_regs[] = {
    {0x3050, 0x00}, {0x319D, 0x00}, {0x341c, 0xff},
    {0x341d, 0x01}, {0x3a01, 0x01},
};

/** 30 fps frame-length (VMAX = 0x1194 = 4500) */
static const struct regval framerate_30fps_regs[] = {
    {0x3030, 0x94}, {0x3031, 0x11},
};

/** Mirror/flip: normal orientation */
static const struct regval mirrorflip_none_regs[] = {
    {0x3074, 0xc8}, {0x3075, 0x00},
    {0x304E, 0x00}, {0x304F, 0x00},
    {0x3081, 0x02}, {0x3083, 0x02},
    {0x30b6, 0x00}, {0x30b7, 0x00},
    {0x3116, 0x08}, {0x3117, 0x00},
};

/** Mirror/flip: horizontally mirrored */
static const struct regval mirrorflip_mirror_regs[] = {
    {0x3074, 0xc8}, {0x3075, 0x00},
    {0x304E, 0x01}, {0x304F, 0x00},
    {0x3081, 0x02}, {0x3083, 0x02},
    {0x30b6, 0x00}, {0x30b7, 0x00},
    {0x3116, 0x08}, {0x3117, 0x00},
};

/* ---------------------------------------------------------------------------
 * Low-level helpers
 * ------------------------------------------------------------------------- */

// Write one 8-bit value to a 16-bit register address
static int32_t write_reg(IMX335_Handle *h, uint16_t reg, uint8_t val)
{
    return I2C_Mem_Write(h->i2c, h->addr, reg, &val, 1) == I2C_OK ? 0 : -1;
}

// Read one 8-bit value from a 16-bit register address
static int32_t read_reg(IMX335_Handle *h, uint16_t reg, uint8_t *val)
{
    return I2C_Mem_Read(h->i2c, h->addr, reg, val, 1) == I2C_OK ? 0 : -1;
}

// Write an array of register/value pairs.  Stops and returns -1 on error
static int32_t write_table(IMX335_Handle *h, const struct regval *tbl,
                            uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        if (write_reg(h, tbl[i].addr, tbl[i].val))
            return -1;
    }
    return 0;
}

// Read back each register to confirm the value stuck
// Returns 0 if all match, -1 on write/read error or mismatch
static int32_t write_table_verify(IMX335_Handle *h, const struct regval *tbl, uint32_t size)
{
    uint8_t rb;
    for (uint32_t i = 0; i < size; i++) {
        if (read_reg(h, tbl[i].addr, &rb)) {
            return -1;
        }
        if (rb != tbl[i].val) {
            return -1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

int32_t IMX335_Probe(IMX335_Handle *h, I2C_TypeDef *i2c)
{
    h->i2c = i2c;
    h->addr = IMX335_I2C_ADDR;
    h->initialized = 0;

    uint32_t id;
    return IMX335_ReadID(h, &id);
}

int32_t IMX335_Init(IMX335_Handle *h)
{
    if (h->initialized)
        return 0;

    /* Write all config tables fast, then verify once at the end */
    if (write_table(h, res_2592_1944_regs, ARRAY_SIZE(res_2592_1944_regs)))
        return -1;

    if (write_table(h, mode_2l_10b_regs, ARRAY_SIZE(mode_2l_10b_regs)))
        return -1;

    if (write_table(h, framerate_30fps_regs, ARRAY_SIZE(framerate_30fps_regs)))
        return -1;

    if (IMX335_VerifyConfig(h))
        return -1;

    h->initialized = 1;
    return 0;
}

int32_t IMX335_Start(IMX335_Handle *h)
{
    return write_reg(h, IMX335_REG_MODE_SELECT, IMX335_MODE_STREAMING);
}

int32_t IMX335_Stop(IMX335_Handle *h)
{
    return write_reg(h, IMX335_REG_MODE_SELECT, IMX335_MODE_STANDBY);
}

int32_t IMX335_SetFramerate(IMX335_Handle *h, uint32_t fps)
{
    (void)fps;
    return write_table(h, framerate_30fps_regs, ARRAY_SIZE(framerate_30fps_regs));
}

int32_t IMX335_SetMirrorFlip(IMX335_Handle *h, uint32_t config)
{
    if (config) {
        return write_table(h, mirrorflip_mirror_regs, ARRAY_SIZE(mirrorflip_mirror_regs));
    }
    else {
        return write_table(h, mirrorflip_none_regs, ARRAY_SIZE(mirrorflip_none_regs));
    }
}

int32_t IMX335_EnableAutoExposure(IMX335_Handle *h)
{
    return write_reg(h, IMX335_REG_AEC, IMX335_AEC_ENABLE);
}

int32_t IMX335_SetHMax(IMX335_Handle *h, uint16_t hmax)
{
    uint8_t lsb = hmax & 0xFF;
    uint8_t msb = (hmax >> 8) & 0xFF;
    if (write_reg(h, IMX335_REG_HMAX + 0, lsb)) return -1;
    if (write_reg(h, IMX335_REG_HMAX + 1, msb)) return -1;
    return 0;
}

int32_t IMX335_ReadID(IMX335_Handle *h, uint32_t *id)
{
    uint8_t id_byte = 0;

    if (h == NULL || id == NULL)
        return -1;

    if (I2C_Mem_Read(h->i2c, h->addr, IMX335_REG_ID, &id_byte, 1) != I2C_OK)
        return -1;

    *id = id_byte;
    return 0;
}

int32_t IMX335_VerifyConfig(IMX335_Handle *h)
{
    if (!h) {
        return -1;
    }

    if (write_table_verify(h, res_2592_1944_regs, ARRAY_SIZE(res_2592_1944_regs))) {
        return -1;
    }

    if (write_table_verify(h, mode_2l_10b_regs, ARRAY_SIZE(mode_2l_10b_regs))) {
        return -1;
    }

    if (write_table_verify(h, framerate_30fps_regs, ARRAY_SIZE(framerate_30fps_regs))) {
        return -1;
    }

    return 0;
}

