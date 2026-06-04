#include <stddef.h>
#include "simple_camera.h"
#include "simple_rcc.h"
#include "simple_ltdc.h"
#include "simple_clock.h"
#include "simple_dcmipp.h"
#include "simple_gpio.h"
#include "simple_i2c.h"
#include "simple_timer.h"

static CAM_Handle *g_cam_h = NULL;

static void Camera_DoubleBufSwap(void)
{
    CAM_Handle *h = g_cam_h;
    if (!h) return;

    uint32_t done_buf = h->active_write_buf;
    h->active_write_buf = h->active_display_buf;
    h->active_display_buf = done_buf;

    DCMIPP_Pipe_UpdateBufAddr(CAM_PIPE_DISPLAY, h->active_write_buf);

    LTDC_Layer1->CFBAR = h->active_display_buf;
    LTDC->SRCR = LTDC_SRCR_VBR;
}

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe)
{
    if (pipe == CAM_PIPE_DISPLAY) {
        Camera_DoubleBufSwap();
    }
}

static void CAM_HwInit(void)
{
    /* Enable PWR clock and VDDIO4 supply (required for GPIOH I/Os) */
    RCC_enable_PWR();
    PWR->SVMCR1 |= PWR_SVMCR1_VDDIO4SV;

    /* Camera power pins: PC8 = 2V8 regulator enable, PD2 = NRST */
    GPIO_Config(GPIOC, 8, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE, GPIO_SPEED_LOW);
    GPIO_Config(GPIOD, 2, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE, GPIO_SPEED_LOW);

    GPIO_BSRR_reset(GPIOC, 8);
    delay_ms(1);
    GPIO_BSRR_reset(GPIOD, 2);
    delay_ms(1);
    GPIO_BSRR_set(GPIOC, 8);
    delay_ms(1);
    GPIO_BSRR_set(GPIOD, 2);
    delay_ms(1);

    /* I2C1 pins: PH9=SCL, PC1=SDA (AF4, open-drain, pull-up) */
    RCC_enable_GPIO(GPIOH);
    RCC_enable_GPIO(GPIOC);
    GPIO_Config(GPIOH, 9, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_PUPD_UP, GPIO_I2C, GPIO_SPEED_HIGH);
    GPIO_Config(GPIOC, 1, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_PUPD_UP, GPIO_I2C, GPIO_SPEED_HIGH);

    /* I2C1 @ 400 kHz (PCLK1=64MHz, PRESC=0, SCLL=75, SCLH=46, SDADEL=0, SCLDEL=6) */
    RCC_enable_I2C(I2C1);
    RCC_setI2C_clock_source(I2C1, 0);
    I2C_Config(I2C1, 0, 0x00602E4B);
}

CAM_Status CAM_Init(CAM_Handle *h, uint32_t nn_buf)
{
    DCMIPP_CSI_Conf csi_conf;
    DCMIPP_Pipe_Conf pipe_conf;
    DCMIPP_IPPlug_Conf ipplug_conf;
    h->display_buf0 = CAM_DISPLAY_BUF0;
    h->display_buf1 = CAM_DISPLAY_BUF1;
    h->nn_buf = nn_buf ? nn_buf : CAM_NN_BUF;
    h->active_write_buf = h->display_buf0;
    h->active_display_buf = h->display_buf1;
    h->initialized = 0;
    g_cam_h = h;

    /* One-time hardware init */
    CAM_HwInit();

    /* Probe and power-on IMX335 */
    if (IMX335_Probe(&h->imx335, I2C1)){
        return CAM_ERROR_ID;
    }

    /* Init DCMIPP + CSI */
    DCMIPP_Init();

    /* CSI-2 config: 2 lanes, 1600 Mbps, RAW10, VC0 */
    csi_conf.num_lanes = DCMIPP_CSI_TWO_DATA_LANES;
    csi_conf.phy_bitrate = DCMIPP_CSI_PHY_BT_1600;
    csi_conf.vc = DCMIPP_VIRTUAL_CHANNEL0;
    csi_conf.dt_format = DCMIPP_CSI_DT_BPP10;
    csi_conf.data_type = 0x2B; /* RAW10 */
    DCMIPP_CSI_Config(&csi_conf);

    /* Enable pipe sharing: PIPE2 gets same data source as PIPE1 */
    DCMIPP_Pipe_EnableShare(CAM_PIPE_DISPLAY, DCMIPP_PIPE_SHARE_SAME);

    /* Config CSI pipe DT for both pipes */
    DCMIPP_CSI_Pipe_Config(CAM_PIPE_DISPLAY, csi_conf.data_type);
    DCMIPP_CSI_Pipe_Config(CAM_PIPE_NN, csi_conf.data_type);

    /* PIPE1: display path (800x480, RGB565 into LCD framebuffer) */
    pipe_conf.output_width = CAM_DISPLAY_WIDTH;
    pipe_conf.output_height = CAM_DISPLAY_HEIGHT;
    pipe_conf.output_format = DCMIPP_PP_FORMAT_RGB565;
    pipe_conf.output_bpp = 2;
    pipe_conf.enable_crop = 1;
    pipe_conf.crop_x = 0;
    pipe_conf.crop_y = 0;
    pipe_conf.crop_width = CAM_DISPLAY_WIDTH;
    pipe_conf.crop_height = CAM_DISPLAY_HEIGHT;
    pipe_conf.enable_downsize = 0;
    pipe_conf.enable_swap = 0;
    DCMIPP_Pipe_Config(CAM_PIPE_DISPLAY, &pipe_conf, (uint32_t*)&h->display_pitch);

    /* PIPE2: NN path (192x144, RGB888) */
    pipe_conf.output_width = CAM_NN_WIDTH;
    pipe_conf.output_height = CAM_NN_HEIGHT;
    pipe_conf.output_format = DCMIPP_PP_FORMAT_RGB888;
    pipe_conf.output_bpp = 3;
    pipe_conf.enable_crop = 1;
    pipe_conf.crop_x = 0;
    pipe_conf.crop_y = 0;
    pipe_conf.crop_width = CAM_NN_WIDTH;
    pipe_conf.crop_height = CAM_NN_HEIGHT;
    pipe_conf.enable_downsize = 1;
    pipe_conf.enable_swap = 0;
    DCMIPP_Pipe_Config(CAM_PIPE_NN, &pipe_conf, (uint32_t*)&h->nn_pitch);

    /* IPPlug client 2 (NN pipe): 512B pages, traffic 128B, BW 16/16, lines 0-559 */
    ipplug_conf.client_id = CAM_CLIENT_NN;
    ipplug_conf.traffic = DCMIPP_TRAFFIC_128B;
    ipplug_conf.outstanding = 0;
    ipplug_conf.wlru_ratio = 16;
    ipplug_conf.dpreg_start = 0;
    ipplug_conf.dpreg_end = 559;
    DCMIPP_IPPlug_Config(&ipplug_conf);

    /* IPPlug client 5 (display pipe): 512B pages, traffic 128B, BW 1/16, outstd 3, lines 560-639 */
    ipplug_conf.client_id = CAM_CLIENT_DISPLAY;
    ipplug_conf.traffic = DCMIPP_TRAFFIC_128B;
    ipplug_conf.outstanding = 3;
    ipplug_conf.wlru_ratio = 1;
    ipplug_conf.dpreg_start = 560;
    ipplug_conf.dpreg_end = 639;
    DCMIPP_IPPlug_Config(&ipplug_conf);

    /* Set memory page size to 512B */
    DCMIPP->IPGR1 = (DCMIPP->IPGR1 & ~DCMIPP_IPGR1_MEMORYPAGE_Msk) | DCMIPP_MEM_PAGE_512B;

    /* Enable frame-end interrupts for both pipes */
    DCMIPP_EnableInterrupts(CAM_PIPE_DISPLAY, DCMIPP_PIPE_IT_FRAMEIE);
    DCMIPP_EnableInterrupts(CAM_PIPE_NN, DCMIPP_PIPE_IT_FRAMEIE);

    /* ISP: demosaic RGGB + black level 64 + neutral color gains */
    DCMIPP_Pipe_EnableISP(CAM_PIPE_DISPLAY, DCMIPP_RAWBAYER_RGGB);
    DCMIPP_Pipe_SetBlackLevel(CAM_PIPE_DISPLAY, 64, 64, 64);
    DCMIPP_Pipe_EnableBlackLevel(CAM_PIPE_DISPLAY);

    /* Init sensor */
    if (IMX335_Init(&h->imx335))
        return CAM_ERROR_INIT;

    if (IMX335_EnableAutoExposure(&h->imx335))
        return CAM_ERROR;

    h->initialized = 1;
    return CAM_OK;
}

CAM_Status CAM_DisplayPipe_Start(CAM_Handle *h)
{
    DCMIPP_Pipe_Start(CAM_PIPE_DISPLAY, h->active_write_buf, 0);
    return IMX335_Start(&h->imx335) ? CAM_ERROR : CAM_OK;
}

CAM_Status CAM_NNPipe_Start(CAM_Handle *h)
{
    DCMIPP_Pipe_Start(CAM_PIPE_NN, h->nn_buf, 0);
    return CAM_OK;
}


