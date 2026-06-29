/**
 * @file    simple_camera.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   Top-level camera subsystem source
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

// ---- Private helpers ----

/**
  * @brief  Board-level hardware initialisation for the camera subsystem
  *         Performed once at the start of CAM_Init():
  *           - Enable PWR clock and mark VDDIO4 supply valid (GPIOH I/Os)
  *           - Assert/de-assert camera regulator (PC8) and reset (PD2)
  *           - Configure I2C1 pins (PH9=SCL, PC1=SDA) as AF4 open-drain
  *           - Initialise I2C1 at 400 kHz
  * @note   TIMER_Delay_ms() must be usable (TIMER_Delay_init() called from main)
  */
static void _CAM_hw_init(void){
    GPIO_Config(GPIOC, 8, GPIO_default_cfg);
    GPIO_Config(GPIOD, 2, GPIO_default_cfg);

    GPIO_BSRR_reset(GPIOC, 8);
    TIMER_Delay_ms(TIMER_TIMEOUT_100_MS);
    GPIO_BSRR_reset(GPIOD, 2);
    TIMER_Delay_ms(TIMER_TIMEOUT_100_MS);
    GPIO_BSRR_set(GPIOC, 8);
    TIMER_Delay_ms(TIMER_TIMEOUT_100_MS);
    GPIO_BSRR_set(GPIOD, 2);
    TIMER_Delay_ms(TIMER_TIMEOUT_100_MS);

    RCC_enable_GPIO(GPIOH);
    RCC_enable_GPIO(GPIOC);
    GPIO_Config(GPIOH, 9, GPIO_I2C_cfg);
    GPIO_Config(GPIOC, 1, GPIO_I2C_cfg);

    RCC_enable_I2C(I2C1);
    RCC_setI2C_clock_source(I2C1, 0);

    I2C_Config(I2C1, 0, I2C_SENSOR_BUS_TIMING);
}

// ---- API ----

CAM_Status_TypeDef CAM_Init(CAM_Handle_TypeDef *h){
    CSI_cfg_TypeDef csi_cfg = CSI_cfg;
    DCMIPP_Pipe_cfg_TypeDef pipe_cfg;
    DCMIPP_IPPlug_cfg_TypeDef ipplug_cfg;

    h->display_buf      = (uint32_t)&ltdc_bg_buffer[0];
    h->nn_buf           = (uint32_t)&ltdc_fg_buffer;
    h->initialized      = 0;

    _CAM_hw_init();

    if (IMX335_Probe(&h->imx335, I2C1))
        return CAM_ERROR_ID;

    // -- CSI-2 --
    CSI_Init();
    CSI_Config(&csi_cfg);
    CSI_SetVirtualChannelConfig(csi_cfg.virtual_channel, csi_cfg.dt_format);

    // -- DCMIPP --

    DCMIPP_Init();
    DCMIPP_Pipe_EnableShare(CAM_PIPE_DISPLAY, DCMIPP_PIPE_SHARE_SAME);
    DCMIPP_CSI_Pipe_Config(CAM_PIPE_DISPLAY, csi_cfg.data_type);
    DCMIPP_CSI_Pipe_Config(CAM_PIPE_NN, csi_cfg.data_type);
    /* Enable VC - starts data flow after pipe config is complete */
    CSI_StartVirtualChannel(csi_cfg.virtual_channel);

    // -- Display pipe: 800 x 480 RGB888 --
    // IMX335 outputs 2592x1944 RAW10; crop full frame then downscale to 800x480
    pipe_cfg = DCMIPP_display_pipe_cfg;
    DCMIPP_Pipe_Config(CAM_PIPE_DISPLAY, &pipe_cfg, (uint32_t *)&h->display_pitch);

    // -- NN pipe: 192 x 144 RGB888 (downscaled) --
    // Crop full sensor frame then downscale to NN input size
    pipe_cfg = DCMIPP_nn_pipe_cfg;
    DCMIPP_Pipe_Config(CAM_PIPE_NN, &pipe_cfg, (uint32_t *)&h->nn_pitch);

    // -- IPPlug (DMA bus arbiter) --
    // IPC2 => CLIENT2 (NN):  R1=0x4  R2=0xf0000   R3=0x22f0000
    ipplug_cfg = DCMIPP_IPPlug_client2_cfg;
    DCMIPP_IPPlug_Config(&ipplug_cfg);

    // IPC5 => CLIENT4:        R1=0x024  R2=0x0   R3=0x27f0230
    ipplug_cfg = DCMIPP_IPPlug_client4_cfg;
    DCMIPP_IPPlug_Config(&ipplug_cfg);

    // -- Interrupts --
    DCMIPP_EnableInterrupts(CAM_PIPE_DISPLAY, DCMIPP_PIPE_IT_FRAMEIE);
    DCMIPP_EnableInterrupts(CAM_PIPE_NN, DCMIPP_PIPE_IT_FRAMEIE);

    // -- ISP --
    DCMIPP_Pipe_EnableISP(CAM_PIPE_DISPLAY, DCMIPP_RAWBAYER_RGGB);
    DCMIPP_Pipe_SetBlackLevel(CAM_PIPE_DISPLAY, 0xC, 0xC, 0xC);
    DCMIPP_Pipe_EnableBlackLevel(CAM_PIPE_DISPLAY);

    // Reduce spurious line events (reference workaround)
    DCMIPP_ReduceSpurious();

    // -- Sensor init --
    if (IMX335_Init(&h->imx335)) {
        return CAM_ERROR_INIT;
    }

    uint32_t ae_exposure;
    AE_Init(10000);
    AE_GetExposureUs(&ae_exposure);
    IMX335_SetExposureUs(&h->imx335, ae_exposure);

    h->initialized = 1;
    return CAM_OK;
}

CAM_Status_TypeDef CAM_DisplayPipe_Start(CAM_Handle_TypeDef *h){
    DCMIPP_Pipe_Start(CAM_PIPE_DISPLAY, 0, 0);

    ltdc_bg_buffer_disp_idx = 1;
    LTDC_Layer1Config.pixel_format = LTDC_PF_RGB888;
    LTDC_Layer1Config.fb = &ltdc_bg_buffer[ltdc_bg_buffer_disp_idx];
    LTDC_ConfigLayer1();

    return IMX335_Start(&h->imx335) ? CAM_ERROR : CAM_OK;
}

CAM_Status_TypeDef CAM_NNPipe_Start(CAM_Handle_TypeDef *h){
    DCMIPP_Pipe_Start(CAM_PIPE_NN, (uint32_t)&ltdc_fg_buffer[0], 0);

    //LTDC_Layer2Config.fb = ltdc_fg_buffer[1];
    //LTDC_Layer2Config.pixel_format = LTDC_PF_RGB888;
    //LTDC_Layer2Config.per_pixel_alpha = 0;
    //LTDC_ConfigLayer2();

    return IMX335_Start(&h->imx335) ? CAM_ERROR : CAM_OK;
}
