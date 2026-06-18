/**
  ******************************************************************************
  * @file    simple_camera.c
  * @author  Groß
  * @brief   Top-level camera subsystem
  *
  *          Call sequence:
  *            CAM_Init()     -- DCMIPP pipes + sensor registers
  *            CAM_DisplayPipe_Start() / CAM_NNPipe_Start()
  ******************************************************************************
  */

#include <stddef.h>
#include "simple_camera.h"
#include "simple_rcc.h"
#include "simple_ltdc.h"
#include "simple_csi.h"
#include "simple_dcmipp.h"
#include "simple_gpio.h"
#include "simple_i2c.h"
#include "simple_timer.h"
#include "simple_ae.h"
#include "config.h"

/* ---------------------------------------------------------------------------
 * Board-level hardware initialisation
 * ------------------------------------------------------------------------- */

/**
  * @brief  Board-level hardware initialisation for the camera subsystem
  *         Performed once at the start of CAM_Init():
  *           - Enable PWR clock and mark VDDIO4 supply valid (GPIOH I/Os)
  *           - Assert/de-assert camera regulator (PC8) and reset (PD2)
  *           - Configure I2C1 pins (PH9=SCL, PC1=SDA) as AF4 open-drain
  *           - Initialise I2C1 at 400 kHz
  * @note   delay_ms() must be usable (delay_init() called from main)
  */
static void CAM_HwInit(void)
{
    RCC_enable_PWR();
    PWR->SVMCR1 |= PWR_SVMCR1_VDDIO4SV;

    GPIO_Config(GPIOC, 8, GPIO_default_cfg);
    GPIO_Config(GPIOD, 2, GPIO_default_cfg);

    GPIO_BSRR_reset(GPIOC, 8);
    delay_ms(100);
    GPIO_BSRR_reset(GPIOD, 2);
    delay_ms(100);
    GPIO_BSRR_set(GPIOC, 8);
    delay_ms(100);
    GPIO_BSRR_set(GPIOD, 2);
    delay_ms(100);

    RCC_enable_GPIO(GPIOH);
    RCC_enable_GPIO(GPIOC);
    GPIO_Config(GPIOH, 9, GPIO_I2C_cfg);
    GPIO_Config(GPIOC, 1, GPIO_I2C_cfg);

    RCC_enable_I2C(I2C1);
    RCC_setI2C_clock_source(I2C1, 0);

    I2C_Config(I2C1, 0, 0x01B11628);
}

/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

CAM_Status CAM_Init(CAM_Handle *h)
{
    CSI_Conf csi_conf;
    DCMIPP_Pipe_Conf pipe_conf = {0};
    DCMIPP_IPPlug_Conf ipplug_conf;

    h->display_buf      = (uint32_t)&lcd_bg_buffer[0];
    h->nn_buf           = (uint32_t)&lcd_fg_buffer;
    h->initialized      = 0;

    CAM_HwInit();

    if (IMX335_Probe(&h->imx335, I2C1))
        return CAM_ERROR_ID;

    /* ------ CSI-2 ------ */

    CSI_Init();

    csi_conf.num_lanes         = CSI_TWO_DATA_LANES;
    csi_conf.data_lane_mapping = CSI_DATA_LANES_PHYSICAL;
    csi_conf.phy_bitrate       = CSI_PHY_BT_1600;
    csi_conf.virtual_channel   = CSI_VIRTUAL_CHANNEL0;
    csi_conf.dt_format         = CSI_DT_BPP10;
    csi_conf.data_type         = 0x2B;
    CSI_Config(&csi_conf);
    CSI_SetVirtualChannelConfig(csi_conf.virtual_channel, csi_conf.dt_format);

    /* ------ DCMIPP ------ */

    DCMIPP_Init();

    DCMIPP_Pipe_EnableShare(CAM_PIPE_DISPLAY, DCMIPP_PIPE_SHARE_SAME);

    DCMIPP_CSI_Pipe_Config(CAM_PIPE_DISPLAY, csi_conf.data_type);
    DCMIPP_CSI_Pipe_Config(CAM_PIPE_NN, csi_conf.data_type);

    /* Enable VC - starts data flow after pipe config is complete */
    CSI_StartVirtualChannel(csi_conf.virtual_channel);

    /* ------ Display pipe: 800 x 480 RGB888 ------ */
    /* IMX335 outputs 2592x1944 RAW10; crop full frame then downscale to 800x480 */

    pipe_conf.output_width   = CAM_DISPLAY_WIDTH;
    pipe_conf.output_height  = CAM_DISPLAY_HEIGHT;
    pipe_conf.output_format  = DCMIPP_PP_FORMAT_RGB888;
    pipe_conf.output_bpp     = 3;
    pipe_conf.enable_crop    = 1;
    pipe_conf.crop_x         = 0;
    pipe_conf.crop_y         = (CAM_SENSOR_HEIGHT - (CAM_DISPLAY_HEIGHT * CAM_SENSOR_WIDTH / CAM_DISPLAY_WIDTH) + 1) / 2;
    pipe_conf.crop_width     = CAM_SENSOR_WIDTH;
    pipe_conf.crop_height    = CAM_DISPLAY_HEIGHT * CAM_SENSOR_WIDTH / CAM_DISPLAY_WIDTH;
    pipe_conf.enable_downsize = 1;
    pipe_conf.enable_swap    = 0;
    pipe_conf.enable_gamma	 = 1;
    pipe_conf.enable_dbm     = 1;
    DCMIPP_Pipe_Config(CAM_PIPE_DISPLAY, &pipe_conf, (uint32_t *)&h->display_pitch);
    DCMIPP->P1PPM0AR1 = (uint32_t)&lcd_bg_buffer[0];
    DCMIPP->P1PPM0AR2 = (uint32_t)&lcd_bg_buffer[1];

    /* ------ NN pipe: 192 x 144 RGB888 (downscaled) ------ */
    /* Crop full sensor frame then downscale to NN input size */

    pipe_conf.output_width   = CAM_NN_WIDTH;
    pipe_conf.output_height  = CAM_NN_HEIGHT;
    pipe_conf.output_format  = DCMIPP_PP_FORMAT_RGB888;
    pipe_conf.output_bpp     = 3;
    pipe_conf.enable_crop    = 1;
    pipe_conf.crop_x         = 0;
    pipe_conf.crop_y         = 0;
    pipe_conf.crop_width     = CAM_SENSOR_WIDTH;
    pipe_conf.crop_height    = CAM_SENSOR_HEIGHT;
    pipe_conf.enable_downsize  = 1;
    pipe_conf.enable_decimate  = 1;
    pipe_conf.decimate_h       = 1;
    pipe_conf.decimate_v       = 1;
    pipe_conf.enable_dbm      = 1;
    pipe_conf.enable_swap      = 0;
    pipe_conf.enable_gamma     = 1;
    DCMIPP_Pipe_Config(CAM_PIPE_NN, &pipe_conf, (uint32_t *)&h->nn_pitch);
    DCMIPP->P2PPM0AR1 = (uint32_t)&lcd_fg_buffer[0];
    DCMIPP->P2PPM0AR2 = (uint32_t)&lcd_fg_buffer[1];

    /* ------ Statistics window (Pipe1) ------ */
    /* Half-resolution window over full sensor, clipped by CROPEN to crop region */

    DCMIPP->P1STSZR = ((CAM_SENSOR_WIDTH / 2) << DCMIPP_P1STSZR_HSIZE_Pos)
                    | ((CAM_SENSOR_HEIGHT / 2) << DCMIPP_P1STSZR_VSIZE_Pos)
                    | DCMIPP_P1STSZR_CROPEN;

    /* ------ IPPlug (DMA bus arbiter) ------ */

    /* IPC2 => CLIENT2 (NN):  R1=0x4  R2=0xf0000   R3=0x22f0000 */
    ipplug_conf.client_id     = DCMIPP_CLIENT2;
    ipplug_conf.traffic       = DCMIPP_TRAFFIC_128B;
    ipplug_conf.outstanding   = 0x0;
    ipplug_conf.wlru_ratio    = 0xF;
    ipplug_conf.dpreg_start   = 0x000;
    ipplug_conf.dpreg_end     = 0x22F;
    DCMIPP_IPPlug_Config(&ipplug_conf);

    /* IPC5 => CLIENT4:        R1=0x024  R2=0x0   R3=0x27f0230 */
    ipplug_conf.client_id     = CAM_CLIENT_DISPLAY;
    ipplug_conf.traffic       = DCMIPP_TRAFFIC_128B;
    ipplug_conf.outstanding   = 0x0;
    ipplug_conf.wlru_ratio    = 0x0;
    ipplug_conf.dpreg_start   = 0x230;
    ipplug_conf.dpreg_end     = 0x27F;
    DCMIPP_IPPlug_Config(&ipplug_conf);

    DCMIPP->IPGR1 = (DCMIPP->IPGR1 & ~DCMIPP_IPGR1_MEMORYPAGE_Msk) | DCMIPP_MEM_PAGE_512B;

    /* ------ Interrupts ------ */

    DCMIPP_EnableInterrupts(CAM_PIPE_DISPLAY, DCMIPP_PIPE_IT_FRAMEIE);
    DCMIPP_EnableInterrupts(CAM_PIPE_NN, DCMIPP_PIPE_IT_FRAMEIE);

    /* ------ ISP ------ */

    DCMIPP_Pipe_EnableISP(CAM_PIPE_DISPLAY, DCMIPP_RAWBAYER_RGGB);
    DCMIPP_Pipe_SetBlackLevel(CAM_PIPE_DISPLAY, 0xC, 0xC, 0xC);
    DCMIPP_Pipe_EnableBlackLevel(CAM_PIPE_DISPLAY);

    /* Reduce spurious line events (reference workaround) */
    DCMIPP_ReduceSpurious();

    /* ------ Sensor init ------ */

    if (IMX335_Init(&h->imx335)) {
        return CAM_ERROR_INIT;
    }

    AE_Init(10000U);
    IMX335_SetExposureUs(&h_cam.imx335, AE_GetExposureUs());

    h->initialized = 1;
    return CAM_OK;
}

CAM_Status CAM_DisplayPipe_Start(CAM_Handle *h)
{
    DCMIPP_Pipe_Start(CAM_PIPE_DISPLAY, 0, 0);

    lcd_bg_buffer_disp_idx = 1;
    LCD_Layer1Config.pixel_format = LCD_PF_RGB888;
    LCD_Layer1Config.fb            = &lcd_bg_buffer[lcd_bg_buffer_disp_idx];
    LCD_ConfigLayer1();

    return IMX335_Start(&h->imx335) ? CAM_ERROR : CAM_OK;
}

CAM_Status CAM_NNPipe_Start(CAM_Handle *h)
{
    DCMIPP_Pipe_Start(CAM_PIPE_NN, (uint32_t)&lcd_fg_buffer[0], 0);

    LCD_Layer2Config.fb = lcd_fg_buffer[1];
    LCD_Layer2Config.pixel_format = LCD_PF_RGB888;
    LCD_Layer2Config.per_pixel_alpha = 0;
    LCD_ConfigLayer2();

    return IMX335_Start(&h->imx335) ? CAM_ERROR : CAM_OK;
}
