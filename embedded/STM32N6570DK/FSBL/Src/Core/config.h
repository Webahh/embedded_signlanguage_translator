/**
 * @file    config.h
 * @author  Weber
 * @date    05.06.2026
 * @brief   Global configuration data and extern declarations
 *
 * Usage
 * -----
 * Include to access board-level config data and extern declarations
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include "simple_gpio.h"
#include "simple_ltdc.h"
#include "simple_camera.h"
#include "simple_rcc.h"
#include "simple_xspi.h"
#include "simple_dcmipp.h"
#include "simple_csi.h"
#include "simple_debug_log.h"

extern const GPIO_cfg_TypeDef GPIO_default_cfg;
extern const GPIO_cfg_TypeDef GPIO_LTDC_cfg;
extern const GPIO_cfg_TypeDef GPIO_I2C_cfg;
extern const GPIO_cfg_TypeDef GPIO_XSPI_cfg;
extern const GPIO_cfg_TypeDef GPIO_USART_debug_cfg;

extern const Debug_log_cfg_TypeDef dbg_cfg;

extern const RCC_PLL_cfg_TypeDef RCC_PLL_cfg[4];
extern const RCC_IC_cfg_TypeDef RCC_IC_cfg[20];

extern const XSPI_cfg_TypeDef XSPI_psram_cfg;
extern const XSPI_cfg_TypeDef XSPI_nor_cfg;
extern const XSPI_CCR_cfg_TypeDef XSPI_write_reg_cfg;
extern const XSPI_CCR_cfg_TypeDef XSPI_PSRAM_memorymapped_cfg;
extern const XSPI_CCR_cfg_TypeDef XSPI_NOR_memorymapped_cfg;

extern const CSI_cfg_TypeDef CSI_cfg;
extern const DCMIPP_Pipe_cfg_TypeDef DCMIPP_display_pipe_cfg;
extern const DCMIPP_Pipe_cfg_TypeDef DCMIPP_nn_pipe_cfg;
extern const DCMIPP_IPPlug_cfg_TypeDef DCMIPP_IPPlug_client2_cfg;
extern const DCMIPP_IPPlug_cfg_TypeDef DCMIPP_IPPlug_client4_cfg;

extern CAM_Handle_TypeDef h_cam;
extern LTDC_LayerConfig_TypeDef LTDC_Layer1Config;
extern LTDC_LayerConfig_TypeDef LTDC_Layer2Config;

#endif /* CONFIG_H_ */
