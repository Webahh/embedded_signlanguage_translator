/**
  ******************************************************************************
  * @file    simple_imx335.c
  * @author  Groß
  * @brief   Register-level IMX335 (Sony Starvis) sensor driver
  *
  *          Initialisation register tables are sourced from the STM32
  *          BSP driver
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stddef.h>

#include "simple_imx335.h"

#include "simple_i2c.h"

//==============================================================================
// Defines and Datatypes
//==============================================================================

#define _ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define _VMAX_30FPS               0x1194U
#define _1H_PERIOD_USEC_X1000     14800U

struct _regval {
    uint16_t addr;
    uint8_t  val;
};

static const struct _regval res_2592_1944_regs[] = {
    {0x3000, 0x01},
    {0x3002, 0x00},
    {0x300c, 0x3b},
    {0x300d, 0x2a},
    {0x3018, 0x04},
    {0x302c, 0x3c},
    {0x302e, 0x20},
    {0x3056, 0x98},
    {0x3074, 0xc8},
    {0x3076, 0x30},
    {0x304c, 0x00},
    {0x314c, 0xc6},
    {0x314d, 0x00},
    {0x315a, 0x02},
    {0x3168, 0xa0},
    {0x316a, 0x7e},
    {0x31a1, 0x00},
    {0x3288, 0x21},
    {0x328a, 0x02},
    {0x3414, 0x05},
    {0x3416, 0x18},
    {0x3648, 0x01},
    {0x364a, 0x04},
    {0x364c, 0x04},
    {0x3678, 0x01},
    {0x367c, 0x31},
    {0x367e, 0x31},
    {0x3706, 0x10},
    {0x3708, 0x03},
    {0x3714, 0x02},
    {0x3715, 0x02},
    {0x3716, 0x01},
    {0x3717, 0x03},
    {0x371c, 0x3d},
    {0x371d, 0x3f},
    {0x372c, 0x00},
    {0x372d, 0x00},
    {0x372e, 0x46},
    {0x372f, 0x00},
    {0x3730, 0x89},
    {0x3731, 0x00},
    {0x3732, 0x08},
    {0x3733, 0x01},
    {0x3734, 0xfe},
    {0x3735, 0x05},
    {0x3740, 0x02},
    {0x375d, 0x00},
    {0x375e, 0x00},
    {0x375f, 0x11},
    {0x3760, 0x01},
    {0x3768, 0x1b},
    {0x3769, 0x1b},
    {0x376a, 0x1b},
    {0x376b, 0x1b},
    {0x376c, 0x1a},
    {0x376d, 0x17},
    {0x376e, 0x0f},
    {0x3776, 0x00},
    {0x3777, 0x00},
    {0x3778, 0x46},
    {0x3779, 0x00},
    {0x377a, 0x89},
    {0x377b, 0x00},
    {0x377c, 0x08},
    {0x377d, 0x01},
    {0x377e, 0x23},
    {0x377f, 0x02},
    {0x3780, 0xd9},
    {0x3781, 0x03},
    {0x3782, 0xf5},
    {0x3783, 0x06},
    {0x3784, 0xa5},
    {0x3788, 0x0f},
    {0x378a, 0xd9},
    {0x378b, 0x03},
    {0x378c, 0xeb},
    {0x378d, 0x05},
    {0x378e, 0x87},
    {0x378f, 0x06},
    {0x3790, 0xf5},
    {0x3792, 0x43},
    {0x3794, 0x7a},
    {0x3796, 0xa1},
    {0x37b0, 0x36},
    {0x3a00, 0x00},
};

static const struct _regval mode_2l_10b_regs[] = {
    {0x3050, 0x00},
    {0x319D, 0x00},
    {0x341c, 0xff},
    {0x341d, 0x01},
    {0x3a01, 0x01},
};

static const struct _regval framerate_10fps_regs[] = {
    {0x3030, 0xC0},
    {0x3031, 0x34},
};

static const struct _regval framerate_15fps_regs[] = {
    {0x3030, 0x2A},
    {0x3031, 0x23},
};

static const struct _regval framerate_20fps_regs[] = {
    {0x3030, 0x60},
    {0x3031, 0x1A},
};

static const struct _regval framerate_25fps_regs[] = {
    {0x3030, 0x1A},
    {0x3031, 0x15},
};

static const struct _regval framerate_30fps_regs[] = {
    {0x3030, 0x94},
    {0x3031, 0x11},
};

static const struct _regval mirrorflip_none_regs[] = {
    {0x3074, 0xc8},
    {0x3075, 0x00},
    {0x304E, 0x00},
    {0x304F, 0x00},
    {0x3081, 0x02},
    {0x3083, 0x02},
    {0x30b6, 0x00},
    {0x30b7, 0x00},
    {0x3116, 0x08},
    {0x3117, 0x00},
};

static const struct _regval mirrorflip_mirror_regs[] = {
    {0x3074, 0xc8},
    {0x3075, 0x00},
    {0x304E, 0x01},
    {0x304F, 0x00},
    {0x3081, 0x02},
    {0x3083, 0x02},
    {0x30b6, 0x00},
    {0x30b7, 0x00},
    {0x3116, 0x08},
    {0x3117, 0x00},
};

// ---- Low-level helpers ----

/**
 * @brief  Write one 8-bit value to a 16-bit register address
 * @param  h   Sensor handle
 * @param  reg 16-bit register address
 * @param  val 8-bit value to write
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
static IMX335_Status_TypeDef _write_reg(IMX335_Handle_TypeDef *h, uint16_t reg, uint8_t val){
    return I2C_Mem_Write(h->i2c, h->addr, reg, &val, 1) == I2C_OK ? IMX335_OK : IMX335_ERROR;
}

/**
 * @brief  Write a 24-bit value across three consecutive 8-bit registers
 * @param  h     Sensor handle
 * @param  reg   Base 16-bit register address
 * @param  value 24-bit value to write
 */
static void _write_reg24(IMX335_Handle_TypeDef *h, uint16_t reg, uint32_t value){
    _write_reg(h, reg + 0U, (uint8_t)(value & 0xFFU));
    _write_reg(h, reg + 1U, (uint8_t)((value >> 8) & 0xFFU));
    _write_reg(h, reg + 2U, (uint8_t)((value >> 16) & 0x0FU));
}

/**
 * @brief  Read one 8-bit value from a 16-bit register address
 * @param  h   Sensor handle
 * @param  reg 16-bit register address
 * @param  val Output: 8-bit value read
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on I2C error
 */
static IMX335_Status_TypeDef _read_reg(IMX335_Handle_TypeDef *h, uint16_t reg, uint8_t *val){
    return I2C_Mem_Read(h->i2c, h->addr, reg, val, 1) == I2C_OK ? IMX335_OK : IMX335_ERROR;
}

/**
 * @brief  Write an array of register/value pairs
 * @param  h    Sensor handle
 * @param  tbl  Table of register/value pairs
 * @param  size Number of entries in the table
 * @retval IMX335_OK    on success
 * @retval IMX335_ERROR on first I2C error (write stops)
 */
static IMX335_Status_TypeDef _write_table(IMX335_Handle_TypeDef *h, const struct _regval *tbl,
                                          uint32_t size){
    for (uint32_t i = 0; i < size; i++) {
        if (_write_reg(h, tbl[i].addr, tbl[i].val)) {
            return IMX335_ERROR;
        }
    }
    return IMX335_OK;
}

/**
 * @brief  Read back each register and compare against written value
 * @param  h    Sensor handle
 * @param  tbl  Table of register/value pairs
 * @param  size Number of entries in the table
 * @retval IMX335_OK    if all registers match
 * @retval IMX335_ERROR on read error or mismatch
 */
static IMX335_Status_TypeDef _write_table_verify(IMX335_Handle_TypeDef *h, const struct _regval *tbl,
                                                 uint32_t size){
    uint8_t rb;
    for (uint32_t i = 0; i < size; i++) {
        if (_read_reg(h, tbl[i].addr, &rb)) {
            return IMX335_ERROR;
        }

        if (rb != tbl[i].val) {
            return IMX335_ERROR;
        }
    }
    return IMX335_OK;
}

// ---- API ----

IMX335_Status_TypeDef IMX335_ReadReg(IMX335_Handle_TypeDef *h, uint16_t reg, uint8_t *val){
    if (!h || !val) {
        return IMX335_ERROR;
    }

    return _read_reg(h, reg, val);
}

typedef struct {
    uint16_t reg;
    uint8_t  val;
    int32_t  ok;
} _IMX335_RegDumpEntry;

typedef struct {
    uint32_t reg;
    uint32_t val;
    uint32_t ok;
} _IMX335_RegDump32;

static volatile _IMX335_RegDumpEntry _dump[] = {
    {0x3000, 0, 0}, {0x3002, 0, 0}, {0x3004, 0, 0},
    {0x300C, 0, 0}, {0x300D, 0, 0},
    {0x3030, 0, 0}, {0x3031, 0, 0}, {0x3032, 0, 0},
    {0x3034, 0, 0}, {0x3035, 0, 0}, {0x3036, 0, 0}, {0x3037, 0, 0},
    {0x304C, 0, 0}, {0x304D, 0, 0},
    {0x304E, 0, 0}, {0x304F, 0, 0},
    {0x3050, 0, 0}, {0x3051, 0, 0},
    {0x3052, 0, 0}, {0x3053, 0, 0},
    {0x3054, 0, 0}, {0x3055, 0, 0},
    {0x3056, 0, 0}, {0x3057, 0, 0},
    {0x3058, 0, 0}, {0x3059, 0, 0},
    {0x314C, 0, 0}, {0x314D, 0, 0}, {0x315A, 0, 0},
    {0x3168, 0, 0}, {0x319D, 0, 0}, {0x319E, 0, 0}, {0x31A1, 0, 0},
    {0x3288, 0, 0}, {0x328A, 0, 0},
    {0x3414, 0, 0}, {0x3416, 0, 0}, {0x3418, 0, 0},
    {0x341A, 0, 0}, {0x341C, 0, 0}, {0x341D, 0, 0},
    {0x3A00, 0, 0}, {0x3A01, 0, 0},
};

#define _DUMP_COUNT (sizeof(_dump) / sizeof(_dump[0]))

static volatile _IMX335_RegDump32 _dump32[_DUMP_COUNT];

void IMX335_DumpDebugRegs(IMX335_Handle_TypeDef *h){
    uint8_t value = 0;

    for (uint32_t i = 0; i < _DUMP_COUNT; i++) {
        int32_t ret = _read_reg(h, _dump[i].reg, &value);

        _dump[i].ok = ret;

        if (ret == 0) {
            _dump[i].val = value;
        } else {
            _dump[i].val = 0xFF;
        }

        _dump32[i].reg = _dump[i].reg;
        _dump32[i].val = _dump[i].val;
        _dump32[i].ok  = _dump[i].ok;
    }
}

IMX335_Status_TypeDef IMX335_Probe(IMX335_Handle_TypeDef *h, I2C_TypeDef *i2c){
    h->i2c = i2c;
    h->addr = IMX335_I2C_ADDR;
    h->initialized = 0;

    uint32_t id;
    return IMX335_ReadID(h, &id);
}

IMX335_Status_TypeDef IMX335_Init(IMX335_Handle_TypeDef *h){
    if (h->initialized) {
        return IMX335_OK;
    }

    if (_write_table(h, res_2592_1944_regs, _ARRAY_SIZE(res_2592_1944_regs))) {
        return IMX335_ERROR;
    }

    if (_write_table(h, mode_2l_10b_regs, _ARRAY_SIZE(mode_2l_10b_regs))) {
        return IMX335_ERROR;
    }

    if (_write_table(h, framerate_30fps_regs, _ARRAY_SIZE(framerate_30fps_regs))) {
        return IMX335_ERROR;
    }

    if (IMX335_VerifyConfig(h)) {
        return IMX335_ERROR;
    }

    h->initialized = 1;
    return IMX335_OK;
}

IMX335_Status_TypeDef IMX335_Start(IMX335_Handle_TypeDef *h){
    return _write_reg(h, IMX335_REG_MODE_SELECT, IMX335_MODE_STREAMING);
}

IMX335_Status_TypeDef IMX335_Stop(IMX335_Handle_TypeDef *h){
    return _write_reg(h, IMX335_REG_MODE_SELECT, IMX335_MODE_STANDBY);
}

IMX335_Status_TypeDef IMX335_SetFramerate(IMX335_Handle_TypeDef *h, uint32_t fps){
    switch (fps) {
        case 10:
            return _write_table(h, framerate_10fps_regs, _ARRAY_SIZE(framerate_10fps_regs));
        case 15:
            return _write_table(h, framerate_15fps_regs, _ARRAY_SIZE(framerate_15fps_regs));
        case 20:
            return _write_table(h, framerate_20fps_regs, _ARRAY_SIZE(framerate_20fps_regs));
        case 25:
            return _write_table(h, framerate_25fps_regs, _ARRAY_SIZE(framerate_25fps_regs));
        default:
            return _write_table(h, framerate_30fps_regs, _ARRAY_SIZE(framerate_30fps_regs));
    }
}

IMX335_Status_TypeDef IMX335_SetMirrorFlip(IMX335_Handle_TypeDef *h, uint32_t config){
    if (config) {
        return _write_table(h, mirrorflip_mirror_regs, _ARRAY_SIZE(mirrorflip_mirror_regs));
    } else {
        return _write_table(h, mirrorflip_none_regs, _ARRAY_SIZE(mirrorflip_none_regs));
    }
}

IMX335_Status_TypeDef IMX335_EnableAutoExposure(IMX335_Handle_TypeDef *h){
    return _write_reg(h, IMX335_REG_AEC, IMX335_AEC_ENABLE);
}

IMX335_Status_TypeDef IMX335_ReadID(IMX335_Handle_TypeDef *h, uint32_t *id){
    uint8_t id_byte = 0;

    if (h == NULL || id == NULL) {
        return IMX335_ERROR;
    }

    if (I2C_Mem_Read(h->i2c, h->addr, IMX335_REG_ID, &id_byte, 1) != I2C_OK) {
        return IMX335_ERROR;
    }

    *id = id_byte;
    return IMX335_OK;
}

IMX335_Status_TypeDef IMX335_VerifyConfig(IMX335_Handle_TypeDef *h){
    if (!h)
        return IMX335_ERROR;

    if (_write_table_verify(h, res_2592_1944_regs, _ARRAY_SIZE(res_2592_1944_regs))) {
        return IMX335_ERROR;
    }

    if (_write_table_verify(h, mode_2l_10b_regs, _ARRAY_SIZE(mode_2l_10b_regs))) {
        return IMX335_ERROR;
    }

    if (_write_table_verify(h, framerate_30fps_regs, _ARRAY_SIZE(framerate_30fps_regs))) {
        return IMX335_ERROR;
    }

    return IMX335_OK;
}

void IMX335_SetExposureUs(IMX335_Handle_TypeDef *h, uint32_t exposure_us){
    if (!h) {
        return;
    }

    uint32_t exposure_lines =
        (exposure_us * 1000U) / _1H_PERIOD_USEC_X1000;

    if (exposure_lines < 1U) {
        exposure_lines = 1U;
    }

    if (exposure_lines > (_VMAX_30FPS - 2U)) {
        exposure_lines = _VMAX_30FPS - 2U;
    }

    uint32_t shs1 = _VMAX_30FPS - exposure_lines;

    _write_reg(h, IMX335_REG_HOLD, 0x01);
    _write_reg24(h, IMX335_REG_SHUTTER, shs1);
    _write_reg(h, IMX335_REG_HOLD, 0x00);
}

IMX335_Status_TypeDef IMX335_SetGainMdB(IMX335_Handle_TypeDef *h, uint32_t gain_mdb){
    if (!h) {
        return IMX335_ERROR;
    }

    gain_mdb = (gain_mdb < IMX335_GAIN_MIN) ? IMX335_GAIN_MIN : gain_mdb;
    gain_mdb = (gain_mdb > IMX335_GAIN_MAX) ? IMX335_GAIN_MAX : gain_mdb;

    uint32_t gain_reg = gain_mdb / IMX335_GAIN_UNIT_MDB;

    _write_reg(h, IMX335_REG_HOLD, 0x01);

    IMX335_Status_TypeDef ret0 = _write_reg(h, IMX335_REG_GAIN + 0U, (uint8_t)(gain_reg & 0xFFU));
    IMX335_Status_TypeDef ret1 = _write_reg(h, IMX335_REG_GAIN + 1U, (uint8_t)((gain_reg >> 8) & 0xFFU));

    _write_reg(h, IMX335_REG_HOLD, 0x00);

    if (ret0 != IMX335_OK || ret1 != IMX335_OK) {
        return IMX335_ERROR;
    }

    return IMX335_OK;
}
