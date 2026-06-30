/**
 * @file    simple_csi.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   Register-level CSI-2 host controller driver header
 *
 * Usage
 * -----
 * 1. CSI_Init()                   	- reset CSI-2 host
 * 2. CSI_Config()                 	- apply configuration
 * 3. CSI_SetVirtualChannelConfig() - VC format config
 * 4. CSI_StartVirtualChannel()		- enable data flow
 */

#ifndef SIMPLE_CSI_H
#define SIMPLE_CSI_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

// ---- Virtual channels ----

#define CSI_VIRTUAL_CHANNEL0            0U
#define CSI_VIRTUAL_CHANNEL1            1U
#define CSI_VIRTUAL_CHANNEL2            2U
#define CSI_VIRTUAL_CHANNEL3            3U

// ---- Data type bit-width encoding (CSI_DT_BPP* -> CDTFT field) ----

#define CSI_DT_BPP6                     0U
#define CSI_DT_BPP7                     1U
#define CSI_DT_BPP8                     2U
#define CSI_DT_BPP10                    3U
#define CSI_DT_BPP12                    4U
#define CSI_DT_BPP14                    5U
#define CSI_DT_BPP16                    6U

// ---- PHY bitrate indices (CSI_PHY_BT_* -> phy_bitrate field) ----

#define CSI_PHY_BT_800                  28U
#define CSI_PHY_BT_1600                 44U

// ---- Lane configuration ----

#define CSI_ONE_DATA_LANE               (1UL << CSI_LMCFGR_LANENB_Pos)
#define CSI_TWO_DATA_LANES              (2UL << CSI_LMCFGR_LANENB_Pos)

#define CSI_DATA_LANES_PHYSICAL         1U
#define CSI_DATA_LANES_INVERTED         2U

#define CSI_DATA_LANE0                  1UL
#define CSI_DATA_LANE1                  2UL

// ---- Types ----

/** CSI host configuration structure */
typedef struct {
    uint32_t num_lanes;          /**< Number of data lanes (CSI_ONE_DATA_LANE / CSI_TWO_DATA_LANES) */
    uint32_t data_lane_mapping;  /**< Physical or inverted lane mapping                             */
    uint32_t phy_bitrate;        /**< PHY bitrate index (CSI_PHY_BT_*)                              */
    uint32_t virtual_channel;    /**< Virtual channel (CSI_VIRTUAL_CHANNEL*)                        */
    uint32_t dt_format;          /**< Data type bit-width (CSI_DT_BPP*)                             */
    uint32_t data_type;          /**< MIPI CSI-2 data type ID (e.g. 0x2B for RAW10)                 */
} CSI_cfg_TypeDef;

/** CSI operation status codes */
typedef enum {
    CSI_OK    = 0,
    CSI_ERROR = 1,
} CSI_Status_TypeDef;

// ---- API ----

void CSI_Init(void);

/**
 * @brief  Configure CSI-2 host controller and PHY
 * @param [in] conf | CSI configuration parameters
 */
void CSI_Config(CSI_cfg_TypeDef *conf);

/**
 * @brief  Configure data-type filtering for a virtual channel
 * @param [in] vc        | Virtual channel index (CSI_VIRTUAL_CHANNEL*)
 * @param [in] dt_format | Data type bit-width (CSI_DT_BPP*)
 */
void CSI_SetVirtualChannelConfig(uint32_t vc, uint32_t dt_format);

/**
 * @brief  Start a virtual channel and wait for ready
 * @param [in] vc | Virtual channel index (CSI_VIRTUAL_CHANNEL*)
 * @retval CSI_OK    Channel started and ready
 * @retval CSI_ERROR Timeout waiting for ready
 */
CSI_Status_TypeDef CSI_StartVirtualChannel(uint32_t vc);

/** Debug interrupt handler - clears all status flags */
void CSI_DBG_IRQHandler(void);

#endif /* SIMPLE_CSI_H */
