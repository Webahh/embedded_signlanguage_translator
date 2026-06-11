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

#include "stddef.h"
#include "simple_imx335.h"
#include "simple_i2c.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/** Register/value pair for table-based writes */
struct regval {
    uint16_t addr; /**< 16-bit register address */
    uint8_t  val;  /**< 8-bit value to write    */
};

/**
  * @brief  2592 x 1944 (5 MP) initialisation table
  *         Groups: mode/clock, MIPI timing, PLL, analog tuning, AEC
  */
static const struct regval res_2592_1944_regs[] = {
    /* ---- Mode / standby ---- */
    {0x3000, 0x01},  /* MODE_SELECT       -> Standby                     */
    {0x3002, 0x00},  /* MASTER_MODE       -> Master mode                 */

    /* ---- MIPI timing ---- */
    {0x300c, 0x3b},  /* BCWAIT_TIME       -> 0x3B clocks                 */
    {0x300d, 0x2a},  /* CPWAIT_TIME       -> 0x2A clocks                 */

    /* ---- Window / output size ---- */
    {0x3018, 0x04},  /* WINMODE           -> 2592x1944 all-pixel         */
    {0x302c, 0x3c},  /* HTRIMMING_START   -> 0x003C (horizontal offset)  */
    {0x302e, 0x20},  /* HNUM              -> pixel count related         */
    {0x3056, 0x98},  /* Y_OUT_SIZE LSB    -> output vertical size        */
    {0x3074, 0xc8},  /* AREA3_ST_ADR_1 LSB-> readout start address       */
    {0x3076, 0x30},  /* AREA3_WIDTH_1 LSB -> readout width               */
    {0x304c, 0x00},  /* OPB_SIZE_V        -> optical black vertical size */

    /* ---- PLL / clock tree ---- */
    {0x314c, 0xc6},  /* INCLKSEL1 LSB     -> PLL multiplier (24 MHz in)  */
    {0x315a, 0x02},  /* INCLKSEL2         -> PLL divider                 */
    {0x3168, 0xa0},  /* INCLKSEL3         -> ADC clock divider           */
    {0x316a, 0x7e},  /* INCLKSEL4         -> additional clock divider    */
    {0x31a1, 0x00},  /* XVS_XHS_DRV       -> VSYNC/HSYNC drive strength  */

    /* ---- BLC (black level calibration) ---- */
    {0x3288, 0x21},  /* BLC mode          -> digital clamp               */
    {0x328a, 0x02},  /* BLC target level  -> target black level          */

    /* ---- Data order ---- */
    {0x3414, 0x05},  /* output order LSB  -> data order setting          */
    {0x3416, 0x18},  /* output order MSB  -> data order setting          */

    /* ---- Analog gain ---- */
    {0x3648, 0x01},  /* analog gain ctl 1 -> gain mode                   */
    {0x364a, 0x04},  /* analog gain ctl 2 -> coarse gain                 */
    {0x364c, 0x04},  /* analog gain ctl 3 -> fine gain                   */
    {0x3678, 0x01},  /* digital gain ctl 1-> gain mode                   */
    {0x367c, 0x31},  /* digital gain ctl 2-> coarse digital gain         */
    {0x367e, 0x31},  /* digital gain ctl 3-> fine digital gain           */

    /* ---- Analog tuning (vendor-specific) ---- */
    {0x3706, 0x10},  /* analog tuning  1                                 */
    {0x3708, 0x03},  /* analog tuning  2                                 */
    {0x3714, 0x02},  /* analog tuning  3                                 */
    {0x3715, 0x02},  /* analog tuning  4                                 */
    {0x3716, 0x01},  /* analog tuning  5                                 */
    {0x3717, 0x03},  /* analog tuning  6                                 */
    {0x371c, 0x3d},  /* analog tuning  7                                 */
    {0x371d, 0x3f},  /* analog tuning  8                                 */
    {0x372c, 0x00},  /* analog tuning  9                                 */
    {0x372d, 0x00},  /* analog tuning 10                                 */
    {0x372e, 0x46},  /* analog tuning 11                                 */
    {0x372f, 0x00},  /* analog tuning 12                                 */
    {0x3730, 0x89},  /* analog tuning 13                                 */
    {0x3731, 0x00},  /* analog tuning 14                                 */
    {0x3732, 0x08},  /* analog tuning 15                                 */
    {0x3733, 0x01},  /* analog tuning 16                                 */
    {0x3734, 0xfe},  /* analog tuning 17                                 */
    {0x3735, 0x05},  /* analog tuning 18                                 */
    {0x3740, 0x02},  /* analog tuning 19                                 */
    {0x375d, 0x00},  /* analog tuning 20                                 */
    {0x375e, 0x00},  /* analog tuning 21                                 */
    {0x375f, 0x11},  /* analog tuning 22                                 */
    {0x3760, 0x01},  /* analog tuning 23                                 */
    {0x3768, 0x1b},  /* analog tuning 24                                 */
    {0x3769, 0x1b},  /* analog tuning 25                                 */
    {0x376a, 0x1b},  /* analog tuning 26                                 */
    {0x376b, 0x1b},  /* analog tuning 27                                 */
    {0x376c, 0x1a},  /* analog tuning 28                                 */
    {0x376d, 0x17},  /* analog tuning 29                                 */
    {0x376e, 0x0f},  /* analog tuning 30                                 */
    {0x3776, 0x00},  /* analog tuning 31                                 */
    {0x3777, 0x00},  /* analog tuning 32                                 */
    {0x3778, 0x46},  /* analog tuning 33                                 */
    {0x3779, 0x00},  /* analog tuning 34                                 */
    {0x377a, 0x89},  /* analog tuning 35                                 */
    {0x377b, 0x00},  /* analog tuning 36                                 */
    {0x377c, 0x08},  /* analog tuning 37                                 */
    {0x377d, 0x01},  /* analog tuning 38                                 */
    {0x377e, 0x23},  /* analog tuning 39                                 */
    {0x377f, 0x02},  /* analog tuning 40                                 */
    {0x3780, 0xd9},  /* analog tuning 41                                 */
    {0x3781, 0x03},  /* analog tuning 42                                 */
    {0x3782, 0xf5},  /* analog tuning 43                                 */
    {0x3783, 0x06},  /* analog tuning 44                                 */
    {0x3784, 0xa5},  /* analog tuning 45                                 */
    {0x3788, 0x0f},  /* analog tuning 46                                 */
    {0x378a, 0xd9},  /* analog tuning 47                                 */
    {0x378b, 0x03},  /* analog tuning 48                                 */
    {0x378c, 0xeb},  /* analog tuning 49                                 */
    {0x378d, 0x05},  /* analog tuning 50                                 */
    {0x378e, 0x87},  /* analog tuning 51                                 */
    {0x378f, 0x06},  /* analog tuning 52                                 */
    {0x3790, 0xf5},  /* analog tuning 53                                 */
    {0x3792, 0x43},  /* analog tuning 54                                 */
    {0x3794, 0x7a},  /* analog tuning 55                                 */
    {0x3796, 0xa1},  /* analog tuning 56                                 */
    {0x37b0, 0x36},  /* analog tuning 57                                 */

    /* ---- AEC enable (Auto Exposure) ---- */
    {0x3a00, 0x01},  /* AEC            									  */
};

/** 2-lane / 10-bit MIPI mode selection */
static const struct regval mode_2l_10b_regs[] = {
    {0x3050, 0x00},  /* ADBIT             -> 10-bit AD conversion         */
    {0x319D, 0x00},  /* MDBIT             -> 10-bit MIPI data output      */
    {0x341c, 0xff},  /* ADBIT1 LSB        -> 10-bit AD config (0x01FF)    */
    {0x341d, 0x01},  /* ADBIT1 MSB        -> 10-bit AD config (0x01FF)    */
    {0x3a01, 0x01},  /* LANEMODE          -> 2-lane MIPI                  */
};

/** 10 fps frame-length (VMAX = 0x34C0 = 13504 lines) */
static const struct regval framerate_10fps_regs[] = {
    {0x3030, 0xC0},  /* VMAX LSB          -> 0x34C0 total lines/frame     */
    {0x3031, 0x34},  /* VMAX MSB          -> 0x34C0 total lines/frame     */
};

/** 15 fps frame-length (VMAX = 0x232A = 9002 lines) */
static const struct regval framerate_15fps_regs[] = {
    {0x3030, 0x2A},  /* VMAX LSB          -> 0x232A total lines/frame     */
    {0x3031, 0x23},  /* VMAX MSB          -> 0x232A total lines/frame     */
};

/** 20 fps frame-length (VMAX = 0x1A60 = 6752 lines) */
static const struct regval framerate_20fps_regs[] = {
    {0x3030, 0x60},  /* VMAX LSB          -> 0x1A60 total lines/frame     */
    {0x3031, 0x1A},  /* VMAX MSB          -> 0x1A60 total lines/frame     */
};

/** 25 fps frame-length (VMAX = 0x151A = 5402 lines) */
static const struct regval framerate_25fps_regs[] = {
    {0x3030, 0x1A},  /* VMAX LSB          -> 0x151A total lines/frame     */
    {0x3031, 0x15},  /* VMAX MSB          -> 0x151A total lines/frame     */
};

/** 30 fps frame-length (VMAX = 0x1194 = 4500 lines) */
static const struct regval framerate_30fps_regs[] = {
    {0x3030, 0x94},  /* VMAX LSB          -> 0x1194 total lines/frame     */
    {0x3031, 0x11},  /* VMAX MSB          -> 0x1194 total lines/frame     */
};

/** Mirror/flip: normal orientation */
static const struct regval mirrorflip_none_regs[] = {
    {0x3074, 0xc8},  /* AREA3_ST_ADR_1 LSB-> readout start = 0x00C8      */
    {0x3075, 0x00},  /* AREA3_ST_ADR_1 MSB-> readout start = 0x00C8      */
    {0x304E, 0x00},  /* HREVERSE          -> normal (no horizontal flip)  */
    {0x304F, 0x00},  /* VREVERSE          -> normal (no vertical flip)    */
    {0x3081, 0x02},  /* reserved (v-flip related)                         */
    {0x3083, 0x02},  /* reserved (v-flip related)                         */
    {0x30b6, 0x00},  /* reserved (v-flip related) LSB                     */
    {0x30b7, 0x00},  /* reserved (v-flip related) MSB                     */
    {0x3116, 0x08},  /* reserved (v-flip related) LSB                     */
    {0x3117, 0x00},  /* reserved (v-flip related) MSB                     */
};

/** Mirror/flip: horizontally mirrored */
static const struct regval mirrorflip_mirror_regs[] = {
    {0x3074, 0xc8},  /* AREA3_ST_ADR_1 LSB-> readout start = 0x00C8      */
    {0x3075, 0x00},  /* AREA3_ST_ADR_1 MSB-> readout start = 0x00C8      */
    {0x304E, 0x01},  /* HREVERSE          -> horizontal mirror            */
    {0x304F, 0x00},  /* VREVERSE          -> normal (no vertical flip)    */
    {0x3081, 0x02},  /* reserved (v-flip related)                         */
    {0x3083, 0x02},  /* reserved (v-flip related)                         */
    {0x30b6, 0x00},  /* reserved (v-flip related) LSB                     */
    {0x30b7, 0x00},  /* reserved (v-flip related) MSB                     */
    {0x3116, 0x08},  /* reserved (v-flip related) LSB                     */
    {0x3117, 0x00},  /* reserved (v-flip related) MSB                     */
};

/* ---------------------------------------------------------------------------
 * Low-level helpers
 * ------------------------------------------------------------------------- */

/**
  * @brief  Write one 8-bit value to a 16-bit register address
  * @param  h   Sensor handle
  * @param  reg 16-bit register address
  * @param  val 8-bit value to write
  * @retval 0 on success, -1 on I2C error
  */
static int32_t write_reg(IMX335_Handle *h, uint16_t reg, uint8_t val)
{
    return I2C_Mem_Write(h->i2c, h->addr, reg, &val, 1) == I2C_OK ? 0 : -1;
}

/**
  * @brief  Read one 8-bit value from a 16-bit register address
  * @param  h   Sensor handle
  * @param  reg 16-bit register address
  * @param  val Output: 8-bit value read
  * @retval 0 on success, -1 on I2C error
  */
static int32_t read_reg(IMX335_Handle *h, uint16_t reg, uint8_t *val)
{
    return I2C_Mem_Read(h->i2c, h->addr, reg, val, 1) == I2C_OK ? 0 : -1;
}

/**
  * @brief  Write an array of register/value pairs
  * @param  h    Sensor handle
  * @param  tbl  Table of register/value pairs
  * @param  size Number of entries in the table
  * @retval 0 on success, -1 on first I2C error (write stops)
  */
static int32_t write_table(IMX335_Handle *h, const struct regval *tbl,
                           uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        if (write_reg(h, tbl[i].addr, tbl[i].val)) {
            return -1;
        }
    }
    return 0;
}

/**
  * @brief  Read back each register and compare against written value
  * @param  h    Sensor handle
  * @param  tbl  Table of register/value pairs
  * @param  size Number of entries in the table
  * @retval 0 if all registers match, -1 on read error or mismatch
  */
static int32_t write_table_verify(IMX335_Handle *h, const struct regval *tbl,
                                  uint32_t size)
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
    if (h->initialized) {
        return 0;
    }

    if (write_table(h, res_2592_1944_regs, ARRAY_SIZE(res_2592_1944_regs))) {
        return -1;
    }

    if (write_table(h, mode_2l_10b_regs, ARRAY_SIZE(mode_2l_10b_regs))) {
        return -1;
    }

    if (write_table(h, framerate_30fps_regs, ARRAY_SIZE(framerate_30fps_regs))) {
        return -1;
    }

    if (IMX335_VerifyConfig(h)) {
        return -1;
    }

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
    switch (fps) {
        case 10:
            return write_table(h, framerate_10fps_regs,
                               ARRAY_SIZE(framerate_10fps_regs));
        case 15:
            return write_table(h, framerate_15fps_regs,
                               ARRAY_SIZE(framerate_15fps_regs));
        case 20:
            return write_table(h, framerate_20fps_regs,
                               ARRAY_SIZE(framerate_20fps_regs));
        case 25:
            return write_table(h, framerate_25fps_regs,
                               ARRAY_SIZE(framerate_25fps_regs));
        default:
            return write_table(h, framerate_30fps_regs,
                               ARRAY_SIZE(framerate_30fps_regs));
    }
}

int32_t IMX335_SetMirrorFlip(IMX335_Handle *h, uint32_t config)
{
    if (config) {
        return write_table(h, mirrorflip_mirror_regs,
                           ARRAY_SIZE(mirrorflip_mirror_regs));
    } else {
        return write_table(h, mirrorflip_none_regs,
                           ARRAY_SIZE(mirrorflip_none_regs));
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

    if (h == NULL || id == NULL) {
        return -1;
    }

    if (I2C_Mem_Read(h->i2c, h->addr, IMX335_REG_ID, &id_byte, 1) != I2C_OK) {
        return -1;
    }

    *id = id_byte;
    return 0;
}

int32_t IMX335_VerifyConfig(IMX335_Handle *h)
{
    if (!h)
        return -1;

    if (write_table_verify(h, res_2592_1944_regs,
                           ARRAY_SIZE(res_2592_1944_regs))) {
        return -1;
    }

    if (write_table_verify(h, mode_2l_10b_regs,
                           ARRAY_SIZE(mode_2l_10b_regs))) {
        return -1;
    }

    if (write_table_verify(h, framerate_30fps_regs,
                           ARRAY_SIZE(framerate_30fps_regs))) {
        return -1;
    }

    return 0;
}
