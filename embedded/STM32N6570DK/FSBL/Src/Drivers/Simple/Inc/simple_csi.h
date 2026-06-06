#ifndef SIMPLE_CSI_H
#define SIMPLE_CSI_H

#include "stm32n657xx.h"

#define CSI_VIRTUAL_CHANNEL0            0U
#define CSI_VIRTUAL_CHANNEL1            1U
#define CSI_VIRTUAL_CHANNEL2            2U
#define CSI_VIRTUAL_CHANNEL3            3U

#define CSI_DT_BPP6                     0U
#define CSI_DT_BPP7                     1U
#define CSI_DT_BPP8                     2U
#define CSI_DT_BPP10                    3U
#define CSI_DT_BPP12                    4U
#define CSI_DT_BPP14                    5U
#define CSI_DT_BPP16                    6U

#define CSI_PHY_BT_800                  28U
#define CSI_PHY_BT_1600                 44U

#define CSI_ONE_DATA_LANE               (1UL << CSI_LMCFGR_LANENB_Pos)
#define CSI_TWO_DATA_LANES              (2UL << CSI_LMCFGR_LANENB_Pos)

#define CSI_DATA_LANES_PHYSICAL         1U
#define CSI_DATA_LANES_INVERTED         2U

#define CSI_DATA_LANE0                  1UL
#define CSI_DATA_LANE1                  2UL

typedef struct CSI_Conf {
    uint32_t num_lanes;
    uint32_t data_lane_mapping;
    uint32_t phy_bitrate;
    uint32_t vc;
    uint32_t dt_format;
    uint32_t data_type;
} CSI_Conf;

void CSI_Init(void);
void CSI_DeInit(void);
void CSI_Config(CSI_Conf *conf);
void CSI_SetVCConfig(uint32_t vc, uint32_t dt_format);
uint32_t CSI_StartVC(uint32_t vc);
void CSI_DBG_IRQHandler(void);

#endif
