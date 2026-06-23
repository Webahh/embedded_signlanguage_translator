/**
  ******************************************************************************
  * @file    simple_csi.c
  * @author  Groß
  * @brief   Register-level CSI-2 host controller driver
  ******************************************************************************
  */

#include <stdint.h>
#include <stddef.h>

#include "simple_csi.h"
#include "simple_rcc.h"
#include "simple_timer.h"

// ---- Private defines ----

#define _REG_MSB_ADDR      0x00
#define _REG_LSB_ADDR_1    0x08
#define _REG_LSB_ADDR_2    0xE4
#define _REG_LSB_ADDR_3    0xE3
#define _REG_VALUE_1       0x38
#define _REG_VALUE_2       0x11
#define _REG_VALUE_3_1     0x08
#define _REG_VALUE_3_2     0xFF

// ---- Private data ----

static CSI_TypeDef *_csi = CSI;

/**
 * @brief  CSI PHY frequency range look-up table (indexed by phy_bitrate)
 *         Each entry maps a bitrate index to its associated HS-FreqRange
 *         register value and target oscillator frequency (MHz * 10)
 */
static const struct {
    uint32_t hsfreqrange;
    uint32_t osc_freq_target;
} _csi_phy_freqs[63] = {
    {0x00, 460}, {0x10, 460}, {0x20, 460}, {0x30, 460},
    {0x01, 460}, {0x11, 460}, {0x21, 460}, {0x31, 460},
    {0x02, 460}, {0x12, 460}, {0x22, 460}, {0x32, 460},
    {0x03, 460}, {0x13, 460}, {0x23, 460}, {0x33, 460},
    {0x04, 460}, {0x14, 460}, {0x25, 460}, {0x35, 460},
    {0x05, 460}, {0x16, 460}, {0x26, 460}, {0x37, 460},
    {0x07, 460}, {0x18, 460}, {0x28, 460}, {0x39, 460},
    {0x09, 460}, {0x19, 460}, {0x29, 460}, {0x3A, 460},
    {0x0A, 460}, {0x1A, 460}, {0x2A, 460}, {0x3B, 460},
    {0x0B, 460}, {0x1B, 460}, {0x2B, 460}, {0x3C, 460},
    {0x0C, 460}, {0x1C, 460}, {0x2C, 460}, {0x3D, 285},
    {0x0D, 295}, {0x1D, 304}, {0x2E, 313}, {0x3E, 322},
    {0x0E, 331}, {0x1E, 341}, {0x2F, 350}, {0x3F, 359},
    {0x0F, 368}, {0x40, 377}, {0x41, 387}, {0x42, 396},
    {0x43, 405}, {0x44, 414}, {0x45, 423}, {0x46, 432},
    {0x47, 442}, {0x48, 451}, {0x49, 460},
};

// ---- Private helpers ----

/**
 * @brief  Write a MIPI CSI-2 PHY code register
 * @param [in] reg_msb | Register address MSB
 * @param [in] reg_lsb | Register address LSB
 * @param [in] val     | Value to write
 */
static void CSI_write_phy_reg(uint8_t reg_msb, uint8_t reg_lsb, uint8_t val){
    _csi->PTCR1 |= CSI_PTCR1_TWM;
    _csi->PTCR0 |= CSI_PTCR0_TCKEN;
    _csi->PTCR1 |= CSI_PTCR1_TWM;
    _csi->PTCR0 = 0;
    _csi->PTCR1 = 0;
    _csi->PTCR1 |= reg_msb;
    _csi->PTCR0 |= CSI_PTCR0_TCKEN;
    _csi->PTCR0 = 0;
    _csi->PTCR1 |= CSI_PTCR1_TWM;
    _csi->PTCR0 |= CSI_PTCR0_TCKEN;
    _csi->PTCR1 |= CSI_PTCR1_TWM | reg_lsb;
    _csi->PTCR0 = 0;
    _csi->PTCR1 = 0;
    _csi->PTCR1 |= val;
    _csi->PTCR0 |= CSI_PTCR0_TCKEN;
    _csi->PTCR0 = 0;
}

// ---- API ----

void CSI_Init(void){
    RCC_enable_CSI();
    RCC_reset_CSI();

    NVIC_SetPriority(CSI_IRQn, 7);
    NVIC_EnableIRQ(CSI_IRQn);
}

void CSI_Config(CSI_cfg_TypeDef *conf){
    uint32_t hsfreqrange, osc_target, phy_idx;

    phy_idx = conf->phy_bitrate;
    if (phy_idx > 62) phy_idx = 62;
    hsfreqrange = _csi_phy_freqs[phy_idx].hsfreqrange;
    osc_target = _csi_phy_freqs[phy_idx].osc_freq_target;

    /* Release CSI PHY from reset */
    _csi->PRCR |= CSI_PRCR_PEN;

    /* Configure PHY frequency - DLD=1 for RX mode (Synopsys: 1=RX, 0=TX) */
    _csi->PFCR = CSI_PFCR_DLD
               | (hsfreqrange << CSI_PFCR_HSFR_Pos)
               | (0x28U << CSI_PFCR_CCFR_Pos);

    _csi->PTCR0 |= CSI_PTCR0_TCKEN;
    TIMER_Delay_ms(TIMER_TIMEOUT_10_MS);
    _csi->PTCR0 = 0;

    CSI_write_phy_reg(_REG_MSB_ADDR, _REG_LSB_ADDR_1, _REG_VALUE_1);
    CSI_write_phy_reg(_REG_MSB_ADDR, _REG_LSB_ADDR_2, _REG_VALUE_1);
    CSI_write_phy_reg(_REG_MSB_ADDR, _REG_LSB_ADDR_3, (uint8_t)(osc_target >> _REG_VALUE_3_1));
    CSI_write_phy_reg(_REG_MSB_ADDR, _REG_LSB_ADDR_3, (uint8_t)(osc_target &  _REG_VALUE_3_2));

    /* Configure lane merger while CSI disabled and sensor not streaming */
    _csi->CR &= ~CSI_CR_CSIEN;

    _csi->LMCFGR = conf->num_lanes
                 | (CSI_DATA_LANE0 << CSI_LMCFGR_DL0MAP_Pos)
                 | (CSI_DATA_LANE1 << CSI_LMCFGR_DL1MAP_Pos);

    /* VC/data type filtering is configured via CSI_SetVCConfig() */

    /* Enable CSI host */
    _csi->CR |= CSI_CR_CSIEN;

    _csi->IER0 = CSI_IER0_CCFIFOFIE
               | CSI_IER0_SYNCERRIE
               | CSI_IER0_SPKTERRIE
               | CSI_IER0_IDERRIE
               | CSI_IER0_SPKTIE;

    if (conf->num_lanes == CSI_ONE_DATA_LANE) {
        _csi->IER1 = CSI_IER1_ESOTDL0IE | CSI_IER1_ESOTSYNCDL0IE
                   | CSI_IER1_EESCDL0IE | CSI_IER1_ESYNCESCDL0IE
                   | CSI_IER1_ECTRLDL0IE;
    } else {
        _csi->IER1 = CSI_IER1_ESOTDL0IE | CSI_IER1_ESOTSYNCDL0IE
                   | CSI_IER1_EESCDL0IE | CSI_IER1_ESYNCESCDL0IE
                   | CSI_IER1_ECTRLDL0IE
                   | CSI_IER1_ESOTDL1IE | CSI_IER1_ESOTSYNCDL1IE
                   | CSI_IER1_EESCDL1IE | CSI_IER1_ESYNCESCDL1IE
                   | CSI_IER1_ECTRLDL1IE;
    }

    /* Enable lanes */
    if (conf->num_lanes == CSI_ONE_DATA_LANE) {
        _csi->PCR = CSI_PCR_DL0EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;
    } else {
        _csi->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;
    }

    _csi->PMCR = 0;
}

void CSI_SetVirtualChannelConfig(uint32_t vc, uint32_t dt_format){
    uint32_t cfg = (dt_format << CSI_VC0CFGR1_CDTFT_Pos) | CSI_VC0CFGR1_ALLDT;

    if (vc == CSI_VIRTUAL_CHANNEL0)
        _csi->VC0CFGR1 = cfg;
    else if (vc == CSI_VIRTUAL_CHANNEL1)
        _csi->VC1CFGR1 = cfg;
    else if (vc == CSI_VIRTUAL_CHANNEL2)
        _csi->VC2CFGR1 = cfg;
    else
        _csi->VC3CFGR1 = cfg;
}

CSI_Status_TypeDef CSI_StartVirtualChannel(uint32_t vc){
    uint32_t mask;

    if (vc == CSI_VIRTUAL_CHANNEL0)
        _csi->CR |= CSI_CR_VC0START;
    else if (vc == CSI_VIRTUAL_CHANNEL1)
        _csi->CR |= CSI_CR_VC1START;
    else if (vc == CSI_VIRTUAL_CHANNEL2)
        _csi->CR |= CSI_CR_VC2START;
    else
        _csi->CR |= CSI_CR_VC3START;

    mask = CSI_SR0_VC0STATEF << vc;
    for (uint32_t t = 0; t < 100000; t++) {
        if (_csi->SR0 & mask) {
            _csi->IER0 |= CSI_IER0_SOF0IE << vc
                       |  CSI_IER0_EOF0IE << vc;
            return CSI_OK;
        }
    }
    return CSI_ERROR;
}

void CSI_DBG_IRQHandler(void){
    if (_csi->SR0) {
        _csi->FCR0 = _csi->SR0;
    }

    if (_csi->SR1) {
        _csi->FCR1 = _csi->SR1;
    }
}
