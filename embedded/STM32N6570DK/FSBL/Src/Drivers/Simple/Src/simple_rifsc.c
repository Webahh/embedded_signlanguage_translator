#include "simple_rifsc.h"
#include "simple_rcc.h"
#include "stm32n657xx.h"

/**
 * @brief Return the bit position of the first set bit.
 *
 * This is used to convert a CID bit mask, e.g. RIF_CID_1,
 * into the numeric CID value required by the RIMC attribute register.
 *
 * Example:
 * 0b0010 -> 1
 * 0b0100 -> 2
 *
 * @param x Bit mask with one bit set.
 * @return Position of the first set bit.
 */
static uint32_t POSITION_VAL(uint32_t x){
    uint32_t pos = 0;
    while ((x & 1U) == 0U){
        x >>= 1U;
        pos++;
    }
    return pos;
}

/**
 * @brief Configure security attributes for a bus master.
 *
 * RIMC controls the attributes of bus masters, such as LTDC, DMA2D,
 * DCMIPP, NPU or GPU2D.
 *
 * Each master is assigned to a compartment ID and marked as
 * secure/non-secure and privileged/unprivileged.
 *
 * @param MasterId RIMC master index.
 * @param pConfig Master configuration containing CID and security attributes.
 */
static void RIMC_ConfigMasterAttributes(uint32_t MasterId, const RIMC_MasterConfig_t *pConfig){
    uint32_t master_cid = POSITION_VAL(pConfig->MasterCID);
    uint32_t rimc_attr_val = RIFSC->RIMC_ATTRx[MasterId];
    rimc_attr_val &= ~(RIFSC_RIMC_ATTRx_MCID | RIFSC_RIMC_ATTRx_MPRIV | RIFSC_RIMC_ATTRx_MSEC);
    rimc_attr_val |= (master_cid << RIFSC_RIMC_ATTRx_MCID_Pos) | (pConfig->SecPriv << RIFSC_RIMC_ATTRx_MSEC_Pos);
    RIFSC->RIMC_ATTRx[MasterId] = rimc_attr_val;
}

/**
 * @brief Configure security attributes for a peripheral slave.
 *
 * RISC controls the security and privilege attributes of peripheral
 * slaves. Each peripheral has one security bit and one privilege bit.
 *
 * @param PeriphId Peripheral index inside the RISC configuration tables.
 * @param SecPriv Security/privilege attribute mask.
 */
static void RISC_SetSlaveSecureAttributes(uint32_t PeriphId, uint32_t SecPriv){
    uint32_t sec_reg_val;

    sec_reg_val = RIFSC->RISC_SECCFGRx[PeriphId >> RIF_PERIPH_REG_SHIFT];
    sec_reg_val &= ~(1UL << (PeriphId & RIF_PERIPH_BIT_POSITION));
    sec_reg_val |= ((SecPriv & RIF_ATTRIBUTE_SEC) << (PeriphId & RIF_PERIPH_BIT_POSITION));
    RIFSC->RISC_SECCFGRx[PeriphId >> RIF_PERIPH_REG_SHIFT] = sec_reg_val;

    sec_reg_val = RIFSC->RISC_PRIVCFGRx[PeriphId >> RIF_PERIPH_REG_SHIFT];
    sec_reg_val &= ~(1UL << (PeriphId & RIF_PERIPH_BIT_POSITION));
    sec_reg_val |= (((SecPriv & RIF_ATTRIBUTE_PRIV) >> 1U) << (PeriphId & RIF_PERIPH_BIT_POSITION));
    RIFSC->RISC_PRIVCFGRx[PeriphId >> RIF_PERIPH_REG_SHIFT] = sec_reg_val;
}

/**
 * @brief Configure RIFSC security settings for display, camera and graphics peripherals.
 *
 * Enables the RIFSC peripheral clock and assigns selected bus masters
 * and peripheral slaves to secure privileged access.
 *
 * This is required so masters such as LTDC, DMA2D, DCMIPP, NPU and GPU2D
 * can access the needed peripherals and memories without being blocked
 * by the Resource Isolation Framework.
 *
 * @note This function currently configures all listed resources as
 *       secure and privileged using compartment ID 1.
 */
void Security_Config(void){
	RCC_enable_RIFSC();

    RIMC_MasterConfig_t RIMC_master = {0};
    RIMC_master.MasterCID = RIF_CID_1;
    RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DMA2D, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_DCMIPP, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC2, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_OTG1, &RIMC_master);
    RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_GPU2D, &RIMC_master);

    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_GFXMMU, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_GPU2D, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_ICACHE, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DMA2D, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CSI, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_DCMIPP, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDC, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL2, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_I2C1, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_OTG1HS, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
    RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_SPI5, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}
