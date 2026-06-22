#include <stdint.h>
#include <stddef.h>

#include "simple_rifsc.h"

#include "simple_rcc.h"
#include "stm32n657xx.h"

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

/**
 * @brief Return the bit position of the first set bit
 *
 * Converts a CID bit mask into the numeric CID value.
 *
 * @param [in] x Bit mask with one bit set
 *
 * @return Position of the first set bit
 */
static uint32_t _POSITION_VAL(uint32_t x){
	uint32_t pos = 0;

	while ((x & 1U) == 0U) {
		x >>= 1U;
		pos++;
	}

	return pos;
}

/**
 * @brief Configure security attributes for a bus master
 *
 * @param [in] MasterId RIMC master index
 * @param [in] pConfig  Master configuration containing CID and security attributes
 */
static void _RIFSC_ConfigMasterAttributes(uint32_t MasterId, const RIFSC_MasterConfig_TypeDef* pConfig){
	uint32_t master_cid = _POSITION_VAL(pConfig->MasterCID);
	uint32_t rimc_attr_val = RIFSC->RIMC_ATTRx[MasterId];

	rimc_attr_val &= ~(RIFSC_RIMC_ATTRx_MCID | RIFSC_RIMC_ATTRx_MPRIV | RIFSC_RIMC_ATTRx_MSEC);
	rimc_attr_val |= (master_cid << RIFSC_RIMC_ATTRx_MCID_Pos) | (pConfig->SecPriv << RIFSC_RIMC_ATTRx_MSEC_Pos);
	RIFSC->RIMC_ATTRx[MasterId] = rimc_attr_val;
}

/**
 * @brief Configure security attributes for a peripheral slave
 *
 * @param [in] PeriphId Peripheral index inside the RISC configuration tables
 * @param [in] SecPriv  Security / privilege attribute mask
 */
static void _RIFSC_SetSlaveSecureAttributes(uint32_t PeriphId, uint32_t SecPriv){
	uint32_t sec_reg_val;

	sec_reg_val = RIFSC->RISC_SECCFGRx[PeriphId >> RIFSC_PERIPH_REG_SHIFT];
	sec_reg_val &= ~(1UL << (PeriphId & RIFSC_PERIPH_BIT_POSITION));
	sec_reg_val |= ((SecPriv & RIFSC_ATTRIBUTE_SEC) << (PeriphId & RIFSC_PERIPH_BIT_POSITION));
	RIFSC->RISC_SECCFGRx[PeriphId >> RIFSC_PERIPH_REG_SHIFT] = sec_reg_val;

	sec_reg_val = RIFSC->RISC_PRIVCFGRx[PeriphId >> RIFSC_PERIPH_REG_SHIFT];
	sec_reg_val &= ~(1UL << (PeriphId & RIFSC_PERIPH_BIT_POSITION));
	sec_reg_val |= (((SecPriv & RIFSC_ATTRIBUTE_PRIV) >> 1U) << (PeriphId & RIFSC_PERIPH_BIT_POSITION));
	RIFSC->RISC_PRIVCFGRx[PeriphId >> RIFSC_PERIPH_REG_SHIFT] = sec_reg_val;
}

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * @brief Configure RIFSC security settings for display, camera and graphics peripherals
 *
 * Enables the RIFSC peripheral clock and assigns selected bus masters
 * and peripheral slaves to secure privileged access.
 */
void RIFSC_Config(void){
	RCC_enable_RIFSC();

	RIFSC_MasterConfig_TypeDef master_cfg = {
		.MasterCID = RIFSC_CID_1,
		.SecPriv   = RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV
	};

	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_NPU,    &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_DMA2D,  &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_DCMIPP, &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_LTDC1,  &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_LTDC2,  &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_OTG1,   &master_cfg);
	_RIFSC_ConfigMasterAttributes(RIFSC_MASTER_INDEX_GPU2D,  &master_cfg);

	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_GFXMMU,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_GPU2D,   RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_ICACHE,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_NPU,     RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_DMA2D,   RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_CSI,     RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_DCMIPP,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_LTDC,    RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_LTDCL1,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_LTDCL2,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_I2C1,    RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_OTG1HS,  RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
	_RIFSC_SetSlaveSecureAttributes(RIFSC_RISC_PERIPH_INDEX_SPI5,    RIFSC_ATTRIBUTE_SEC | RIFSC_ATTRIBUTE_PRIV);
}
