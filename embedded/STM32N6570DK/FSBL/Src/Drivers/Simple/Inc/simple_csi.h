/**
  ******************************************************************************
  * @file    simple_csi.h
  * @author  Groß
  * @brief   Register-level CSI-2 host controller driver header
  ******************************************************************************
  */

#ifndef SIMPLE_CSI_H
#define SIMPLE_CSI_H

#include "stm32n657xx.h"

/* ---------------------------------------------------------------------------
 * Virtual channels
 * ------------------------------------------------------------------------- */

#define CSI_VIRTUAL_CHANNEL0            0U
#define CSI_VIRTUAL_CHANNEL1            1U
#define CSI_VIRTUAL_CHANNEL2            2U
#define CSI_VIRTUAL_CHANNEL3            3U

/* ---------------------------------------------------------------------------
 * Data type bit-width encoding (CSI_DT_BPP* -> CDTFT field)
 * ------------------------------------------------------------------------- */

#define CSI_DT_BPP6                     0U
#define CSI_DT_BPP7                     1U
#define CSI_DT_BPP8                     2U
#define CSI_DT_BPP10                    3U
#define CSI_DT_BPP12                    4U
#define CSI_DT_BPP14                    5U
#define CSI_DT_BPP16                    6U

/* ---------------------------------------------------------------------------
 * PHY bitrate indices (CSI_PHY_BT_* -> phy_bitrate field)
 * ------------------------------------------------------------------------- */

#define CSI_PHY_BT_800                  28U
#define CSI_PHY_BT_1600                 44U

/* ---------------------------------------------------------------------------
 * Lane configuration
 * ------------------------------------------------------------------------- */

#define CSI_ONE_DATA_LANE               (1UL << CSI_LMCFGR_LANENB_Pos)
#define CSI_TWO_DATA_LANES              (2UL << CSI_LMCFGR_LANENB_Pos)

#define CSI_DATA_LANES_PHYSICAL         1U
#define CSI_DATA_LANES_INVERTED         2U

#define CSI_DATA_LANE0                  1UL
#define CSI_DATA_LANE1                  2UL

/* ---------------------------------------------------------------------------
 * CSI configuration structure
 * ------------------------------------------------------------------------- */

/** CSI host configuration structure */
typedef struct {
    uint32_t num_lanes;          /**< Number of data lanes (CSI_ONE_DATA_LANE / CSI_TWO_DATA_LANES) */
    uint32_t data_lane_mapping;  /**< Physical or inverted lane mapping                             */
    uint32_t phy_bitrate;        /**< PHY bitrate index (CSI_PHY_BT_*)                              */
    uint32_t virtual_channel;    /**< Virtual channel (CSI_VIRTUAL_CHANNEL*)                        */
    uint32_t dt_format;          /**< Data type bit-width (CSI_DT_BPP*)                             */
    uint32_t data_type;          /**< MIPI CSI-2 data type ID (e.g. 0x2B for RAW10)                 */
} CSI_cfg_TypeDef;

/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

void     CSI_Init(void);
void     CSI_Config(CSI_cfg_TypeDef *conf);
void     CSI_SetVirtualChannelConfig(uint32_t vc, uint32_t dt_format);
uint32_t CSI_StartVirtualChannel(uint32_t vc);
void     CSI_DBG_IRQHandler(void);

#endif /* SIMPLE_CSI_H */
