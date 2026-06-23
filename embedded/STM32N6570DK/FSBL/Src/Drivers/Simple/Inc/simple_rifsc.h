/**
 * @file    simple_rifsc.h
 * @author  Weber
 * @date    05.06.2026
 * @brief   RIFSC security / access control driver header
 *
 * Usage
 * -----
 * 1. RIFSC_Config()   – configure IL / RL at early boot
 */

#ifndef SIMPLE_RIFSC_H
#define SIMPLE_RIFSC_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#define RIFSC_MASTER_INDEX_NPU              1U
#define RIFSC_MASTER_INDEX_OTG1             4U
#define RIFSC_MASTER_INDEX_GPU2D            7U
#define RIFSC_MASTER_INDEX_DMA2D            8U
#define RIFSC_MASTER_INDEX_DCMIPP           9U
#define RIFSC_MASTER_INDEX_LTDC1            10U
#define RIFSC_MASTER_INDEX_LTDC2            11U

#define RIFSC_PERIPH_REG_SHIFT              28U

#define RIFSC_PERIPH_REG0                   0x00000000U
#define RIFSC_PERIPH_REG1                   0x10000000U
#define RIFSC_PERIPH_REG2                   0x20000000U
#define RIFSC_PERIPH_REG3                   0x30000000U
#define RIFSC_PERIPH_REG4                   0x40000000U
#define RIFSC_PERIPH_REG5                   0x50000000U

#define RIFSC_PERIPH_BIT_POSITION           0x0000001FU

#define RIFSC_RISC_PERIPH_INDEX_GFXMMU     (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC4_Pos)
#define RIFSC_RISC_PERIPH_INDEX_GPU2D      (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC3_Pos)
#define RIFSC_RISC_PERIPH_INDEX_ICACHE     (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC2_Pos)
#define RIFSC_RISC_PERIPH_INDEX_NPU        (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC10_Pos)
#define RIFSC_RISC_PERIPH_INDEX_DMA2D      (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC5_Pos)
#define RIFSC_RISC_PERIPH_INDEX_CSI        (RIFSC_PERIPH_REG2 | RIFSC_RISC_SECCFGRx_SEC28_Pos)
#define RIFSC_RISC_PERIPH_INDEX_DCMIPP     (RIFSC_PERIPH_REG2 | RIFSC_RISC_SECCFGRx_SEC29_Pos)
#define RIFSC_RISC_PERIPH_INDEX_LTDC       (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC6_Pos)
#define RIFSC_RISC_PERIPH_INDEX_LTDCL1     (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC7_Pos)
#define RIFSC_RISC_PERIPH_INDEX_LTDCL2     (RIFSC_PERIPH_REG3 | RIFSC_RISC_SECCFGRx_SEC8_Pos)
#define RIFSC_RISC_PERIPH_INDEX_OTG1HS     (RIFSC_PERIPH_REG1 | RIFSC_RISC_SECCFGRx_SEC24_Pos)
#define RIFSC_RISC_PERIPH_INDEX_I2C1       (RIFSC_PERIPH_REG0 | RIFSC_RISC_SECCFGRx_SEC9_Pos)
#define RIFSC_RISC_PERIPH_INDEX_SPI5       (RIFSC_PERIPH_REG0 | RIFSC_RISC_SECCFGRx_SEC4_Pos)

#define RIFSC_CID_NONE                      0x00000000U
#define RIFSC_CID_0                         0x00000001U
#define RIFSC_CID_1                         0x00000002U
#define RIFSC_CID_2                         0x00000004U
#define RIFSC_CID_3                         0x00000008U
#define RIFSC_CID_4                         0x00000010U
#define RIFSC_CID_5                         0x00000020U
#define RIFSC_CID_6                         0x00000040U
#define RIFSC_CID_7                         0x00000080U

#define RIFSC_ATTRIBUTE_NSEC                0x00000000U
#define RIFSC_ATTRIBUTE_SEC                 0x00000001U
#define RIFSC_ATTRIBUTE_NPRIV               0x00000000U
#define RIFSC_ATTRIBUTE_PRIV                0x00000002U

typedef struct {
	uint32_t MasterCID;
	uint32_t SecPriv;
} RIFSC_MasterConfig_TypeDef;

/**
 * @brief Configure RIFSC security settings for all required peripherals
 *
 * Assigns bus masters and peripheral slaves to secure privileged access
 * so that LTDC, DMA2D, DCMIPP, NPU, GPU2D and others can access
 * the needed peripherals and memories.
 */
void RIFSC_Config(void);

#endif /* SIMPLE_RIFSC_H */
