#include <stddef.h>
#include "simple_dcmipp.h"
#include "simple_rcc.h"

static CSI_TypeDef *csi = CSI;
static DCMIPP_TypeDef *dcmipp = DCMIPP;

static const struct {
    uint32_t hsfreqrange;
    uint32_t osc_freq_target;
} dcmipp_phy_freqs[63] = {
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

static void CSI_WritePHYReg(uint8_t addr, uint8_t data)
{
    csi->PTCR0 = addr;
    csi->PTCR1 = data;
}

void DCMIPP_Init(void)
{
    RCC_enable_DCMIPP();
    RCC_reset_DCMIPP();

    RCC_enable_CSI();
    RCC_reset_CSI();

    dcmipp->CMCR = 0;
    dcmipp->CMCR = (1UL << DCMIPP_CMCR_PSFC_Pos);

    dcmipp->CMFCR = 0xFFFFFFFFU;

    NVIC_SetPriority(DCMIPP_IRQn, 7);
    NVIC_EnableIRQ(DCMIPP_IRQn);
    NVIC_SetPriority(CSI_IRQn, 7);
    NVIC_EnableIRQ(CSI_IRQn);
}

void DCMIPP_DeInit(void)
{
    DCMIPP_Pipe_Stop(DCMIPP_PIPE1);
    DCMIPP_Pipe_Stop(DCMIPP_PIPE2);

    NVIC_DisableIRQ(DCMIPP_IRQn);
    NVIC_DisableIRQ(CSI_IRQn);

    RCC->APB5RSTSR |= RCC_APB5RSTSR_DCMIPPRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_DCMIPPRSTC;
    (void)RCC->APB5RSTCR;

    RCC->APB5RSTSR |= RCC_APB5RSTSR_CSIRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_CSIRSTC;
    (void)RCC->APB5RSTCR;
}

void DCMIPP_CSI_Config(DCMIPP_CSI_Conf *conf)
{
    uint32_t hsfreqrange, osc_target, phy_idx;

    csi->CR &= ~CSI_CR_CSIEN;

    csi->LMCFGR = (conf->num_lanes << CSI_LMCFGR_LANENB_Pos)
                | (0UL << CSI_LMCFGR_DL0MAP_Pos)
                | (1UL << CSI_LMCFGR_DL1MAP_Pos);

    phy_idx = conf->phy_bitrate;
    if (phy_idx > 62) phy_idx = 62;
    hsfreqrange = dcmipp_phy_freqs[phy_idx].hsfreqrange;
    osc_target = dcmipp_phy_freqs[phy_idx].osc_freq_target;

    csi->VC0CFGR1 = (conf->dt_format << CSI_VC0CFGR1_CDTFT_Pos) | CSI_VC0CFGR1_ALLDT;

    csi->PRCR &= ~CSI_PRCR_PEN;
    csi->PCR = 0;

    csi->PTCR0 |= CSI_PTCR0_TCKEN;
    for (volatile int i = 0; i < 10; i++);
    csi->PTCR0 &= ~CSI_PTCR0_TCKEN;

    csi->PFCR = (0x28U << CSI_PFCR_CCFR_Pos)
              | (hsfreqrange << CSI_PFCR_HSFR_Pos);

    CSI_WritePHYReg(0x08, 0x38);
    CSI_WritePHYReg(0xE4, 0x11);
    CSI_WritePHYReg(0xE3, (uint8_t)(osc_target >> 8));
    CSI_WritePHYReg(0xE3, (uint8_t)(osc_target & 0xFF));

    csi->PFCR = (0x28U << CSI_PFCR_CCFR_Pos)
              | (hsfreqrange << CSI_PFCR_HSFR_Pos)
              | CSI_PFCR_DLD;

    if (conf->num_lanes == DCMIPP_CSI_ONE_DATA_LANE)
        csi->PCR = CSI_PCR_DL0EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;
    else
        csi->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;

    csi->PRCR |= CSI_PRCR_PEN;
    csi->PMCR = 0;
    csi->CR |= CSI_CR_CSIEN;
}

void DCMIPP_CSI_Pipe_Config(uint32_t pipe, uint32_t data_type)
{
    if (pipe == DCMIPP_PIPE0) {
        dcmipp->P0FSCR = (data_type << DCMIPP_P0FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (DCMIPP_VIRTUAL_CHANNEL0 << 19);
    } else if (pipe == DCMIPP_PIPE1) {
        dcmipp->P1FSCR = (dcmipp->P1FSCR & ~(DCMIPP_P1FSCR_DTIDA_Msk |
                          (0x3UL << 16) | (0x3UL << 19)))
                       | (data_type << DCMIPP_P1FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (DCMIPP_VIRTUAL_CHANNEL0 << 19);
    } else {
        dcmipp->P2FSCR = (dcmipp->P2FSCR & ~(DCMIPP_P2FSCR_DTIDA_Msk |
                          (0x3UL << 16) | (0x3UL << 19)))
                       | (data_type << DCMIPP_P2FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (DCMIPP_VIRTUAL_CHANNEL0 << 19);
    }

    dcmipp->PRCR &= ~DCMIPP_PRCR_ENABLE;
    dcmipp->CMCR |= DCMIPP_CMCR_INSEL;
}

void DCMIPP_Pipe_Config(uint32_t pipe, DCMIPP_Pipe_Conf *conf, uint32_t *out_pitch)
{
    uint32_t pitch;
    DCMIPP_Pipe_Conf zero_conf = {0};

    if (!conf) conf = &zero_conf;

    pitch = conf->output_width * conf->output_bpp;
    pitch = DCMIPP_AlignPitch(pitch);

    if (pipe == DCMIPP_PIPE1) {
        if (conf->enable_crop && conf->crop_width && conf->crop_height) {
            dcmipp->P1CRSTR = (conf->crop_x << DCMIPP_P1CRSTR_HSTART_Pos)
                            | (conf->crop_y << DCMIPP_P1CRSTR_VSTART_Pos);
            dcmipp->P1CRSZR = ((conf->crop_width - 1) << DCMIPP_P1CRSZR_HSIZE_Pos)
                            | ((conf->crop_height - 1) << DCMIPP_P1CRSZR_VSIZE_Pos)
                            | DCMIPP_P1CRSZR_ENABLE;
        } else {
            dcmipp->P1CRSZR &= ~DCMIPP_P1CRSZR_ENABLE;
        }

        if (conf->enable_downsize && conf->output_width && conf->output_height) {
            uint32_t in_w = conf->enable_crop ? conf->crop_width : conf->output_width;
            uint32_t in_h = conf->enable_crop ? conf->crop_height : conf->output_height;
            if (in_w > conf->output_width) {
                uint32_t ratio = ((in_w << 10) / conf->output_width) + 1;
                dcmipp->P1DSRTIOR = (ratio << DCMIPP_P1DSRTIOR_HRATIO_Pos)
                                  | (ratio << DCMIPP_P1DSRTIOR_VRATIO_Pos);
                dcmipp->P1DSSZR = ((conf->output_width - 1) << DCMIPP_P1DSSZR_HSIZE_Pos)
                                | ((conf->output_height - 1) << DCMIPP_P1DSSZR_VSIZE_Pos);
                dcmipp->P1DSCR |= DCMIPP_P1DSCR_ENABLE;
            } else {
                dcmipp->P1DSCR &= ~DCMIPP_P1DSCR_ENABLE;
            }
        } else {
            dcmipp->P1DSCR &= ~DCMIPP_P1DSCR_ENABLE;
        }

        if (conf->enable_swap)
            dcmipp->CMCR |= DCMIPP_CMCR_SWAPRB;
        else
            dcmipp->CMCR &= ~DCMIPP_CMCR_SWAPRB;

        dcmipp->P1PPCR = (dcmipp->P1PPCR & ~DCMIPP_P1PPCR_FORMAT_Msk)
                       | (conf->output_format << DCMIPP_P1PPCR_FORMAT_Pos);

        dcmipp->P1PPM0PR = pitch;

        dcmipp->P1FCTCR = (dcmipp->P1FCTCR & ~DCMIPP_P1FCTCR_FRATE_Msk) | 0;

    } else if (pipe == DCMIPP_PIPE2) {
        if (conf->enable_crop && conf->crop_width && conf->crop_height) {
            dcmipp->P2CRSTR = (conf->crop_x << DCMIPP_P2CRSTR_HSTART_Pos)
                            | (conf->crop_y << DCMIPP_P2CRSTR_VSTART_Pos);
            dcmipp->P2CRSZR = ((conf->crop_width - 1) << DCMIPP_P2CRSZR_HSIZE_Pos)
                            | ((conf->crop_height - 1) << DCMIPP_P2CRSZR_VSIZE_Pos)
                            | DCMIPP_P2CRSZR_ENABLE;
        } else {
            dcmipp->P2CRSZR &= ~DCMIPP_P2CRSZR_ENABLE;
        }

        if (conf->enable_downsize && conf->output_width && conf->output_height) {
            uint32_t in_w = conf->enable_crop ? conf->crop_width : conf->output_width;
            uint32_t in_h = conf->enable_crop ? conf->crop_height : conf->output_height;
            if (in_w > conf->output_width) {
                uint32_t ratio = ((in_w << 10) / conf->output_width) + 1;
                dcmipp->P2DSRTIOR = (ratio << DCMIPP_P2DSRTIOR_HRATIO_Pos)
                                  | (ratio << DCMIPP_P2DSRTIOR_VRATIO_Pos);
                dcmipp->P2DSSZR = ((conf->output_width - 1) << DCMIPP_P2DSSZR_HSIZE_Pos)
                                | ((conf->output_height - 1) << DCMIPP_P2DSSZR_VSIZE_Pos);
                dcmipp->P2DSCR |= DCMIPP_P2DSCR_ENABLE;
            } else {
                dcmipp->P2DSCR &= ~DCMIPP_P2DSCR_ENABLE;
            }
        } else {
            dcmipp->P2DSCR &= ~DCMIPP_P2DSCR_ENABLE;
        }

        dcmipp->P2PPCR = (dcmipp->P2PPCR & ~DCMIPP_P2PPCR_FORMAT_Msk)
                       | (conf->output_format << DCMIPP_P2PPCR_FORMAT_Pos);

        dcmipp->P2PPM0PR = pitch;

        dcmipp->P2FCTCR = (dcmipp->P2FCTCR & ~DCMIPP_P2FCTCR_FRATE_Msk) | 0;
    }

    if (out_pitch) *out_pitch = pitch;
}

void DCMIPP_Pipe_EnableShare(uint32_t pipe, uint32_t mode)
{
    (void)pipe;
    if (mode == DCMIPP_PIPE_SHARE_SAME) {
        dcmipp->P1FSCR &= ~DCMIPP_P1FSCR_PIPEDIFF;
    } else {
        dcmipp->P1FSCR |= DCMIPP_P1FSCR_PIPEDIFF;
    }
}

void DCMIPP_IPPlug_Config(DCMIPP_IPPlug_Conf *conf)
{
    if (!conf) return;

    volatile uint32_t *r1 = NULL, *r2 = NULL, *r3 = NULL;

    switch (conf->client_id) {
        case DCMIPP_CLIENT1: r1 = &dcmipp->IPC1R1; r2 = &dcmipp->IPC1R2; r3 = &dcmipp->IPC1R3; break;
        case DCMIPP_CLIENT2: r1 = &dcmipp->IPC2R1; r2 = &dcmipp->IPC2R2; r3 = &dcmipp->IPC2R3; break;
        case DCMIPP_CLIENT3: r1 = &dcmipp->IPC3R1; r2 = &dcmipp->IPC3R2; r3 = &dcmipp->IPC3R3; break;
        case DCMIPP_CLIENT4: r1 = &dcmipp->IPC4R1; r2 = &dcmipp->IPC4R2; r3 = &dcmipp->IPC4R3; break;
        case DCMIPP_CLIENT5: r1 = &dcmipp->IPC5R1; r2 = &dcmipp->IPC5R2; r3 = &dcmipp->IPC5R3; break;
        default: return;
    }

    *r1 = (conf->traffic & DCMIPP_IPC1R1_TRAFFIC_Msk)
        | ((conf->outstanding << DCMIPP_IPC1R1_OTR_Pos) & DCMIPP_IPC1R1_OTR_Msk);

    *r2 = (conf->wlru_ratio << DCMIPP_IPC1R2_WLRU_Pos) & DCMIPP_IPC1R2_WLRU_Msk;

    *r3 = ((conf->dpreg_start << DCMIPP_IPC1R3_DPREGSTART_Pos) & DCMIPP_IPC1R3_DPREGSTART_Msk)
        | ((conf->dpreg_end << DCMIPP_IPC1R3_DPREGEND_Pos) & DCMIPP_IPC1R3_DPREGEND_Msk);
}

void DCMIPP_Pipe_UpdateBufAddr(uint32_t pipe, uint32_t buf_addr)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1PPM0AR1 = buf_addr;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2PPM0AR1 = buf_addr;
}

void DCMIPP_Pipe_EnableISP(uint32_t pipe, uint32_t bayer_type)
{
    (void)pipe;
    dcmipp->P1DMCR = bayer_type
                   | (2U << DCMIPP_P1DMCR_PEAK_Pos)
                   | (4U << DCMIPP_P1DMCR_LINEV_Pos)
                   | (4U << DCMIPP_P1DMCR_LINEH_Pos)
                   | (6U << DCMIPP_P1DMCR_EDGE_Pos)
                   | DCMIPP_P1DMCR_ENABLE;
}

void DCMIPP_Pipe_SetBlackLevel(uint32_t pipe, uint32_t blk_r, uint32_t blk_g, uint32_t blk_b)
{
    (void)pipe;
    dcmipp->P1BLCCR = (blk_r << DCMIPP_P1BLCCR_BLCR_Pos)
                    | (blk_g << DCMIPP_P1BLCCR_BLCG_Pos)
                    | (blk_b << DCMIPP_P1BLCCR_BLCB_Pos);
}

void DCMIPP_Pipe_EnableBlackLevel(uint32_t pipe)
{
    (void)pipe;
    dcmipp->P1BLCCR |= DCMIPP_P1BLCCR_ENABLE;
}

void DCMIPP_Pipe_Start(uint32_t pipe, uint32_t buf_addr, uint32_t mode)
{
    if (pipe == DCMIPP_PIPE1) {
        dcmipp->P1PPM0AR1 = buf_addr;
        if (mode == 0)
            dcmipp->P1FCTCR &= ~DCMIPP_P1FCTCR_CPTMODE;
        else
            dcmipp->P1FCTCR |= DCMIPP_P1FCTCR_CPTMODE;
        dcmipp->P1FSCR |= DCMIPP_P1FSCR_PIPEN;

    } else if (pipe == DCMIPP_PIPE2) {
        dcmipp->P2PPM0AR1 = buf_addr;
        if (mode == 0)
            dcmipp->P2FCTCR &= ~DCMIPP_P2FCTCR_CPTMODE;
        else
            dcmipp->P2FCTCR |= DCMIPP_P2FCTCR_CPTMODE;
        dcmipp->P2FSCR |= DCMIPP_P2FSCR_PIPEN;
    }

    dcmipp->IPGR2 |= DCMIPP_IPGR2_PSTART;
}

void DCMIPP_Pipe_Stop(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1FSCR &= ~DCMIPP_P1FSCR_PIPEN;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2FSCR &= ~DCMIPP_P2FSCR_PIPEN;

    dcmipp->IPGR2 &= ~DCMIPP_IPGR2_PSTART;
}

void DCMIPP_Pipe_Suspend(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1FSCR &= ~DCMIPP_P1FSCR_PIPEN;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2FSCR &= ~DCMIPP_P2FSCR_PIPEN;
}

void DCMIPP_Pipe_Resume(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1FSCR |= DCMIPP_P1FSCR_PIPEN;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2FSCR |= DCMIPP_P2FSCR_PIPEN;
}

void DCMIPP_EnableInterrupts(uint32_t pipe, uint32_t it_mask)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1IER |= it_mask;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2IER |= it_mask;
}

void DCMIPP_DisableInterrupts(uint32_t pipe, uint32_t it_mask)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1IER &= ~it_mask;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2IER &= ~it_mask;
}

void DCMIPP_ClearInterrupt(uint32_t pipe, uint32_t it_mask)
{
    if (pipe == DCMIPP_PIPE1)
        dcmipp->P1FCR = it_mask;
    else if (pipe == DCMIPP_PIPE2)
        dcmipp->P2FCR = it_mask;
}

uint32_t DCMIPP_GetStatus(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1)
        return dcmipp->P1SR;
    else if (pipe == DCMIPP_PIPE2)
        return dcmipp->P2SR;
    return 0;
}

void DCMIPP_IRQHandler(void)
{
    uint32_t sr;

    sr = dcmipp->P1SR;
    if (sr) {
        if (sr & DCMIPP_P1SR_FRAMEF) {
            dcmipp->P1FCR = DCMIPP_P1FCR_CFRAMEF;
            DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE1);
        }
        if (sr & DCMIPP_P1SR_VSYNCF) {
            dcmipp->P1FCR = DCMIPP_P1FCR_CVSYNCF;
            DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE1);
        }
        if (sr & DCMIPP_P1SR_OVRF) {
            dcmipp->P1FCR = DCMIPP_P1FCR_COVRF;
        }
    }

    sr = dcmipp->P2SR;
    if (sr) {
        if (sr & DCMIPP_P2SR_FRAMEF) {
            dcmipp->P2FCR = DCMIPP_P2FCR_CFRAMEF;
            DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE2);
        }
        if (sr & DCMIPP_P2SR_VSYNCF) {
            dcmipp->P2FCR = DCMIPP_P2FCR_CVSYNCF;
            DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE2);
        }
        if (sr & DCMIPP_P2SR_OVRF) {
            dcmipp->P2FCR = DCMIPP_P2FCR_COVRF;
        }
    }
}

void CSI_IRQHandler(void)
{
    if (csi->SR0) {
    	csi->FCR0 = csi->SR0;
    }

    if (csi->SR1) {
    	csi->FCR1 = csi->SR1;
    }
}

__attribute__((weak)) void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_VsyncEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_ErrorCallback(uint32_t pipe) { (void)pipe; }
