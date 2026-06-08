#include <stddef.h>
#include "simple_csi.h"
#include "simple_rcc.h"

static CSI_TypeDef *csi = CSI;

static const struct {
    uint32_t hsfreqrange;
    uint32_t osc_freq_target;
} csi_phy_freqs[63] = {
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

static void CSI_WritePHYReg(uint8_t reg_msb, uint8_t reg_lsb, uint8_t val)
{
    csi->PTCR1 |= CSI_PTCR1_TWM;
    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    csi->PTCR1 |= CSI_PTCR1_TWM;
    csi->PTCR0 = 0;
    csi->PTCR1 = 0;
    csi->PTCR1 |= reg_msb;
    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    csi->PTCR0 = 0;
    csi->PTCR1 |= CSI_PTCR1_TWM;
    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    csi->PTCR1 |= CSI_PTCR1_TWM | reg_lsb;
    csi->PTCR0 = 0;
    csi->PTCR1 = 0;
    csi->PTCR1 |= val;
    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    csi->PTCR0 = 0;
}

void CSI_Init(void)
{
    RCC_config_CSI_clock_IC18();

    RCC_enable_CSI();
    RCC_reset_CSI();

    NVIC_SetPriority(CSI_IRQn, 7);
    NVIC_EnableIRQ(CSI_IRQn);
}

void CSI_DeInit(void)
{
    NVIC_DisableIRQ(CSI_IRQn);

    RCC->APB5RSTSR |= RCC_APB5RSTSR_CSIRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_CSIRSTC;
    (void)RCC->APB5RSTCR;
}

void CSI_Config(CSI_Conf *conf)
{
    uint32_t hsfreqrange, osc_target, phy_idx;

    csi->CR &= ~CSI_CR_CSIEN;

    csi->LMCFGR = conf->num_lanes
                | (CSI_DATA_LANE0 << CSI_LMCFGR_DL0MAP_Pos)
                | (CSI_DATA_LANE1 << CSI_LMCFGR_DL1MAP_Pos);

    csi->CR |= CSI_CR_CSIEN;

    csi->IER0 = CSI_IER0_CCFIFOFIE
              | CSI_IER0_SYNCERRIE
              | CSI_IER0_SPKTERRIE
              | CSI_IER0_IDERRIE
              | CSI_IER0_SPKTIE
              | CSI_IER0_CRCERRIE;

    if (conf->num_lanes == CSI_ONE_DATA_LANE)
    {
        csi->IER1 = CSI_IER1_ESOTDL0IE | CSI_IER1_ESOTSYNCDL0IE
                  | CSI_IER1_EESCDL0IE | CSI_IER1_ESYNCESCDL0IE
                  | CSI_IER1_ECTRLDL0IE;
    }
    else
    {
        csi->IER1 = CSI_IER1_ESOTDL0IE | CSI_IER1_ESOTSYNCDL0IE
                  | CSI_IER1_EESCDL0IE | CSI_IER1_ESYNCESCDL0IE
                  | CSI_IER1_ECTRLDL0IE
                  | CSI_IER1_ESOTDL1IE | CSI_IER1_ESOTSYNCDL1IE
                  | CSI_IER1_EESCDL1IE | CSI_IER1_ESYNCESCDL1IE
                  | CSI_IER1_ECTRLDL1IE;
    }

    phy_idx = conf->phy_bitrate;
    if (phy_idx > 62) phy_idx = 62;
    hsfreqrange = csi_phy_freqs[phy_idx].hsfreqrange;
    osc_target = csi_phy_freqs[phy_idx].osc_freq_target;

    csi->PRCR &= ~CSI_PRCR_PEN;
    csi->PCR = 0;

    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    for (volatile uint32_t d = 0; d < 400000; d++);
    csi->PTCR0 = 0;

    csi->PFCR = (0x28U << CSI_PFCR_CCFR_Pos)
              | (hsfreqrange << CSI_PFCR_HSFR_Pos);

    CSI_WritePHYReg(0x00, 0x08, 0x38);
    CSI_WritePHYReg(0x00, 0xE4, 0x11);
    CSI_WritePHYReg(0x00, 0xE3, (uint8_t)(osc_target >> 8));
    CSI_WritePHYReg(0x00, 0xE3, (uint8_t)(osc_target & 0xFF));

    csi->PFCR = (0x28U << CSI_PFCR_CCFR_Pos)
              | (hsfreqrange << CSI_PFCR_HSFR_Pos)
              | CSI_PFCR_DLD;

    if (conf->num_lanes == CSI_ONE_DATA_LANE)
        csi->PCR = CSI_PCR_DL0EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;
    else
        csi->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;

    csi->PRCR |= CSI_PRCR_PEN;
    csi->PMCR = 0;
}

void CSI_SetVCConfig(uint32_t vc, uint32_t dt_format)
{
    uint32_t cfg = (dt_format << CSI_VC0CFGR1_CDTFT_Pos) | CSI_VC0CFGR1_ALLDT;

    if (vc == CSI_VIRTUAL_CHANNEL0)
        csi->VC0CFGR1 = cfg;
    else if (vc == CSI_VIRTUAL_CHANNEL1)
        csi->VC1CFGR1 = cfg;
    else if (vc == CSI_VIRTUAL_CHANNEL2)
        csi->VC2CFGR1 = cfg;
    else
        csi->VC3CFGR1 = cfg;
}

uint32_t CSI_StartVC(uint32_t vc)
{
    uint32_t mask;

    if (vc == CSI_VIRTUAL_CHANNEL0)
        csi->CR |= CSI_CR_VC0START;
    else if (vc == CSI_VIRTUAL_CHANNEL1)
        csi->CR |= CSI_CR_VC1START;
    else if (vc == CSI_VIRTUAL_CHANNEL2)
        csi->CR |= CSI_CR_VC2START;
    else
        csi->CR |= CSI_CR_VC3START;

    mask = CSI_SR0_VC0STATEF << vc;
    for (uint32_t t = 0; t < 100000; t++)
    {
        if (csi->SR0 & mask)
        {
            csi->IER0 |= CSI_IER0_SOF0IE << vc
                      |  CSI_IER0_EOF0IE << vc;
            return 1;
        }
    }
    return 0;
}

void CSI_DBG_IRQHandler(void)
{
    if (csi->SR0) {
        csi->FCR0 = csi->SR0;
    }

    if (csi->SR1) {
        csi->FCR1 = csi->SR1;
    }
}
