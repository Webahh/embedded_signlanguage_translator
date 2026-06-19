/*
 * config.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include "config.h"

const GPIO_cfg_TypeDef GPIO_default_cfg = {
		.mode  	= GPIO_MODE_OUTPUT,
		.otyp 	= GPIO_OTYPE_PP,
		.pupdr	= GPIO_PUPD_NONE,
		.af		= GPIO_AF_NONE,
		.speed  = GPIO_SPEED_LOW
};

const GPIO_cfg_TypeDef GPIO_LTDC_cfg = {
		.mode	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_PP,
	    .pupdr  = GPIO_PUPD_NONE,
	    .af		= GPIO_AF_LTDC,
		.speed  = GPIO_SPEED_VERY_HIGH
};

const GPIO_cfg_TypeDef GPIO_I2C_cfg = {
		.mode	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_OD,
		.pupdr	= GPIO_PUPD_UP,
		.af		= GPIO_AF_I2C,
		.speed  = GPIO_SPEED_HIGH
};

const GPIO_cfg_TypeDef GPIO_XSPI_cfg = {
		.mode 	= GPIO_MODE_AF,
		.otyp	= GPIO_OTYPE_PP,
		.pupdr	= GPIO_PUPD_UP,
		.af		= GPIO_AF_XSPI,
		.speed 	= GPIO_SPEED_VERY_HIGH
};

const RCC_PLL_cfg_TypeDef RCC_PLL_cfg[4] = {
    { .CFGR1 = 0x201900, .CFGR2 = 0x0, .CFGR3 = 0x49000005 },
    { .CFGR1 = 0x807D00, .CFGR2 = 0x0, .CFGR3 = 0x49000005 },
    { .CFGR1 = 0x80E100, .CFGR2 = 0x0, .CFGR3 = 0x4A000005 },
    { .CFGR1 = 0x80E100, .CFGR2 = 0x0, .CFGR3 = 0x76000005 },
};

const RCC_IC_cfg_TypeDef RCC_IC_cfg[20] = {
    [0]  = { .CFGR = 0x00000000 },  /* IC1:  PLL1 / 1  - CPU */
    [1]  = { .CFGR = 0x00010000 },  /* IC2:  PLL1 / 2  - SYSB/AXI */
	[2]  = { .CFGR = 0x00030000 },  /* IC3:  PLL1 / 4  - XSPI */
    [5]  = { .CFGR = 0x10000000 },  /* IC6:  PLL2 / 1 */
    [10] = { .CFGR = 0x20000000 },  /* IC11: PLL3 / 1 */
    [15] = { .CFGR = 0x30010000 },  /* IC16: PLL4 / 2  - LTDC */
    [16] = { .CFGR = 0x10020000 },  /* IC17: PLL2 / 3  - DCMIPP */
    [17] = { .CFGR = 0x00270000 },  /* IC18: PLL1 / 40 - CSI */
};

const XSPI_cfg_TypeDef XSPI_psram_cfg = {
		.memory_type = 6,
		.devsize = 24,
		.chipselect_high_time = 4,
		.prescaler = 3,
		.chipselect_boundary = 11,
		.maxtran_value = 0,
		.refresh_cycles = 129
};

const XSPI_cfg_TypeDef XSPI_nor_cfg = {
		.memory_type = 1,
		.devsize = 24,
		.chipselect_high_time = 1,
		.prescaler = 0,
		.chipselect_boundary = 0,
		.maxtran_value = 0,
		.refresh_cycles = 0
};

const XSPI_CCR_cfg_TypeDef XSPI_write_reg_cfg = {
		.instruction_mode 	= 4,
		.instruction_dtr  	= 0,
		.instruction_size	= 0,
		.address_mode		= 4,
		.address_dtr		= 1,
		.address_size		= 3,
		.data_mode			= 4,
		.data_dtr			= 1,
		.data_qse			= 0
};

const XSPI_CCR_cfg_TypeDef XSPI_memorymapped_cfg = {
		.instruction_mode 	= 4,
		.instruction_dtr  	= 0,
		.instruction_size	= 0,
		.address_mode		= 4,
		.address_dtr		= 1,
		.address_size		= 3,
		.data_mode			= 5,
		.data_dtr			= 1,
		.data_qse			= 1
};

const CSI_cfg_TypeDef CSI_cfg = {
		.num_lanes 			= CSI_TWO_DATA_LANES,
		.data_lane_mapping  = CSI_DATA_LANES_PHYSICAL,
		.phy_bitrate		= CSI_PHY_BT_1600,
		.virtual_channel	= CSI_VIRTUAL_CHANNEL0,
		.dt_format			= CSI_DT_BPP10,
		.data_type			= 0x2B
};

const DCMIPP_Pipe_cfg_TypeDef DCMIPP_display_pipe_cfg = {
		.output_width		= CAM_DISPLAY_WIDTH,
		.output_height		= CAM_DISPLAY_HEIGHT,
		.output_format		= DCMIPP_PP_FORMAT_RGB888,
		.output_bpp			= 3,
		.enable_crop		= 1,
		.crop_x				= 0,
		.crop_y				= (CAM_SENSOR_HEIGHT - (CAM_DISPLAY_HEIGHT * CAM_SENSOR_WIDTH / CAM_DISPLAY_WIDTH) + 1) / 2,
		.crop_width			= CAM_SENSOR_WIDTH,
		.crop_height		= CAM_DISPLAY_HEIGHT * CAM_SENSOR_WIDTH / CAM_DISPLAY_WIDTH,
		.enable_downsize 	= 1,
		.enable_swap		= 0,
		.enable_gamma		= 1,
		.enable_dbm			= 1
};

const DCMIPP_Pipe_cfg_TypeDef DCMIPP_nn_pipe_cfg = {
		.output_width		= CAM_NN_WIDTH,
		.output_height		= CAM_NN_HEIGHT,
		.output_format		= DCMIPP_PP_FORMAT_RGB888,
		.output_bpp			= 3,
		.enable_crop		= 1,
		.crop_x				= 0,
		.crop_y				= 0,
		.crop_width			= CAM_SENSOR_WIDTH,
		.crop_height		= CAM_SENSOR_HEIGHT,
		.enable_downsize 	= 1,
		.enable_decimate	= 1,
	    .decimate_h      	= 1,
	    .decimate_v       	= 1,
		.enable_swap		= 0,
		.enable_gamma		= 1,
		.enable_dbm			= 1
};

const DCMIPP_IPPlug_cfg_TypeDef DCMIPP_IPPlug_client2_cfg = {
		.client_id			= DCMIPP_CLIENT2,
		.traffic			= DCMIPP_TRAFFIC_128B,
		.outstanding		= 0x0,
		.wlru_ratio			= 0xF,
		.dpreg_start		= 0x0,
		.dpreg_end			= 0x22F
};

const DCMIPP_IPPlug_cfg_TypeDef DCMIPP_IPPlug_client4_cfg = {
		.client_id			= CAM_CLIENT_DISPLAY,
		.traffic			= DCMIPP_TRAFFIC_128B,
		.outstanding		= 0x0,
		.wlru_ratio			= 0x0,
		.dpreg_start		= 0x230,
		.dpreg_end			= 0x27F
};


CAM_Handle h_cam;

LCD_LayerConfig LCD_Layer1Config = {
    .regs            = LTDC_Layer1,
    .fb              = lcd_bg_buffer[0],
    .x               = 0,
    .y               = 0,
    .width           = LCD_BG_WIDTH,
    .height          = LCD_BG_HEIGHT,
    .buf_width       = LCD_BG_WIDTH,
    .pixel_format    = LCD_PF_RGB888,
    .const_alpha     = 0xFF,
    .per_pixel_alpha = 0,
    .default_color   = 0,
    .blending_order  = 0,
};


static const LCD_Layer_FlexiblePixelFormat LCD_FPF_ARGB4444 = LCD_FPF_ARGB4444_INIT;
LCD_LayerConfig LCD_Layer2Config = {
    .regs            = LTDC_Layer2,
    .fb              = lcd_fg_buffer[0],
    .x               = 0,
    .y               = 0,
    .width           = LCD_FG_WIDTH,
    .height          = LCD_FG_HEIGHT,
    .buf_width       = LCD_FG_WIDTH,
    .pixel_format    = LCD_PF_RGB888,
    .flexible_fmt    = NULL,
    .const_alpha     = 0xFF,
    .per_pixel_alpha = 0,
    .default_color   = 0x00,
    .blending_order  = 1,
};



