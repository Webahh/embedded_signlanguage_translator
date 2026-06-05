#ifndef SIMPLE_RIFSC_H
#define SIMPLE_RIFSC_H

#include <stdint.h>

#define RIF_MASTER_INDEX_NPU               1U
#define RIF_MASTER_INDEX_OTG1              4U
#define RIF_MASTER_INDEX_GPU2D             7U
#define RIF_MASTER_INDEX_DMA2D             8U
#define RIF_MASTER_INDEX_DCMIPP            9U
#define RIF_MASTER_INDEX_LTDC1             10U
#define RIF_MASTER_INDEX_LTDC2             11U

#define RIF_PERIPH_REG_SHIFT               28U

#define RIF_PERIPH_REG0                    0x00000000U
#define RIF_PERIPH_REG1                    0x10000000U
#define RIF_PERIPH_REG2                    0x20000000U
#define RIF_PERIPH_REG3                    0x30000000U
#define RIF_PERIPH_REG4                    0x40000000U
#define RIF_PERIPH_REG5                    0x50000000U

#define RIF_PERIPH_BIT_POSITION            0x0000001FU

#define RIF_RISC_PERIPH_INDEX_GFXMMU      (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC4_Pos)
#define RIF_RISC_PERIPH_INDEX_GPU2D       (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC3_Pos)
#define RIF_RISC_PERIPH_INDEX_ICACHE      (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC2_Pos)
#define RIF_RISC_PERIPH_INDEX_NPU         (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC10_Pos)
#define RIF_RISC_PERIPH_INDEX_DMA2D       (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC5_Pos)
#define RIF_RISC_PERIPH_INDEX_CSI         (RIF_PERIPH_REG2 | RIFSC_RISC_SECCFGRx_SEC28_Pos)
#define RIF_RISC_PERIPH_INDEX_DCMIPP      (RIF_PERIPH_REG2 | RIFSC_RISC_SECCFGRx_SEC29_Pos)
#define RIF_RISC_PERIPH_INDEX_LTDC        (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC6_Pos)
#define RIF_RISC_PERIPH_INDEX_LTDCL1      (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC7_Pos)
#define RIF_RISC_PERIPH_INDEX_LTDCL2      (RIF_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC8_Pos)
#define RIF_RISC_PERIPH_INDEX_OTG1HS      (RIF_PERIPH_REG1 | RIFSC_RISC_SECCFGRx_SEC24_Pos)
#define RIF_RISC_PERIPH_INDEX_I2C1        (RIF_PERIPH_REG0 | RIFSC_RISC_SECCFGRx_SEC9_Pos)
#define RIF_RISC_PERIPH_INDEX_SPI5        (RIF_PERIPH_REG0 | RIFSC_RISC_SECCFGRx_SEC4_Pos)

#define RIF_CID_NONE                       0x00000000U
#define RIF_CID_0                          0x00000001U
#define RIF_CID_1                          0x00000002U
#define RIF_CID_2                          0x00000004U
#define RIF_CID_3                          0x00000008U
#define RIF_CID_4                          0x00000010U
#define RIF_CID_5                          0x00000020U
#define RIF_CID_6                          0x00000040U
#define RIF_CID_7                          0x00000080U

#define RIF_ATTRIBUTE_NSEC                 0x00000000U
#define RIF_ATTRIBUTE_SEC                  0x00000001U
#define RIF_ATTRIBUTE_NPRIV                0x00000000U
#define RIF_ATTRIBUTE_PRIV                 0x00000002U

typedef struct
{
  uint32_t MasterCID;
  uint32_t SecPriv;
} RIMC_MasterConfig_t;

void Security_Config(void);

#endif
