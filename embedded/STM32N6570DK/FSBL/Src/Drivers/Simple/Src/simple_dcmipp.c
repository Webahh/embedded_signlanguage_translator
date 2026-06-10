#include <stddef.h>
#include "simple_dcmipp.h"
#include "simple_rcc.h"

static DCMIPP_TypeDef *dcmipp = DCMIPP;

void DCMIPP_Init(void)
{
    RCC_config_DCMIPP_clock_IC17();
    RCC_enable_DCMIPP();
    RCC_reset_DCMIPP();

    dcmipp->CMCR = 0;
    dcmipp->CMFCR = 0xFFFFFFFFU;

    dcmipp->CMIER = DCMIPP_CMIER_ATXERRIE
    			  | DCMIPP_CMIER_P1FRAMEIE
				  | DCMIPP_CMIER_P1VSYNCIE
				  | DCMIPP_CMIER_P1OVRIE
    			  | DCMIPP_CMIER_P2FRAMEIE
				  | DCMIPP_CMIER_P2VSYNCIE
				  | DCMIPP_CMIER_P2OVRIE
				  ;

    NVIC_SetPriority(DCMIPP_IRQn, 7);
    NVIC_EnableIRQ(DCMIPP_IRQn);
}

void DCMIPP_DeInit(void)
{
    DCMIPP_Pipe_Stop(DCMIPP_PIPE1);
    DCMIPP_Pipe_Stop(DCMIPP_PIPE2);

    NVIC_DisableIRQ(DCMIPP_IRQn);

    RCC->APB5RSTSR |= RCC_APB5RSTSR_DCMIPPRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_DCMIPPRSTC;
    (void)RCC->APB5RSTCR;
}

void DCMIPP_CSI_Pipe_Config(uint32_t pipe, uint32_t data_type)
{
    if (pipe == DCMIPP_PIPE0) {
        dcmipp->P0FSCR = (data_type << DCMIPP_P0FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (CSI_VIRTUAL_CHANNEL0 << 19);
    } else if (pipe == DCMIPP_PIPE1) {
        dcmipp->P1FSCR = (dcmipp->P1FSCR & ~(DCMIPP_P1FSCR_DTIDA_Msk |
                          (0x3UL << 16) | (0x3UL << 19)))
                       | (data_type << DCMIPP_P1FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (CSI_VIRTUAL_CHANNEL0 << 19);
    } else {
        dcmipp->P2FSCR = (dcmipp->P2FSCR & ~(DCMIPP_P2FSCR_DTIDA_Msk |
                          (0x3UL << 16) | (0x3UL << 19)))
                       | (data_type << DCMIPP_P2FSCR_DTIDA_Pos)
                       | (0UL << 16)
                       | (CSI_VIRTUAL_CHANNEL0 << 19);
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
            dcmipp->P1CRSZR = (conf->crop_width << DCMIPP_P1CRSZR_HSIZE_Pos)
                            | (conf->crop_height << DCMIPP_P1CRSZR_VSIZE_Pos)
                            | DCMIPP_P1CRSZR_ENABLE;
        } else {
            dcmipp->P1CRSZR &= ~DCMIPP_P1CRSZR_ENABLE;
        }

//        dcmipp->P1DECR |= DCMIPP_P1DECR_ENABLE;

        if (conf->enable_downsize && conf->output_width && conf->output_height) {
            uint32_t in_w = conf->enable_crop ? conf->crop_width : conf->output_width;
            uint32_t in_h = conf->enable_crop ? conf->crop_height : conf->output_height;
            if (in_w > conf->output_width || in_h > conf->output_height) {
                uint32_t hratio = ((uint64_t)in_w << 13) / conf->output_width;
                uint32_t vratio = ((uint64_t)in_h << 13) / conf->output_height;
                uint32_t hdiv = (1024UL * 8192UL - 1UL) / hratio;
                uint32_t vdiv = (1024UL * 8192UL - 1UL) / vratio;
                dcmipp->P1DSRTIOR = (hratio << DCMIPP_P1DSRTIOR_HRATIO_Pos)
                                  | (vratio << DCMIPP_P1DSRTIOR_VRATIO_Pos);
                dcmipp->P1DSSZR = (conf->output_width << DCMIPP_P1DSSZR_HSIZE_Pos)
                                | (conf->output_height << DCMIPP_P1DSSZR_VSIZE_Pos);
                dcmipp->P1DSCR = (hdiv << DCMIPP_P1DSCR_HDIV_Pos)
                               | (vdiv << DCMIPP_P1DSCR_VDIV_Pos)
                               | DCMIPP_P1DSCR_ENABLE;
            } else {
                dcmipp->P1DSCR &= ~DCMIPP_P1DSCR_ENABLE;
            }
        } else {
            dcmipp->P1DSCR &= ~DCMIPP_P1DSCR_ENABLE;
        }

        dcmipp->P1GMCR |= DCMIPP_P1GMCR_ENABLE;

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
            dcmipp->P2CRSZR = (conf->crop_width << DCMIPP_P2CRSZR_HSIZE_Pos)
                            | (conf->crop_height << DCMIPP_P2CRSZR_VSIZE_Pos)
                            | DCMIPP_P2CRSZR_ENABLE;
        } else {
            dcmipp->P2CRSZR &= ~DCMIPP_P2CRSZR_ENABLE;
        }

        if (conf->enable_downsize && conf->output_width && conf->output_height) {
            uint32_t in_w = conf->enable_crop ? conf->crop_width : conf->output_width;
            uint32_t in_h = conf->enable_crop ? conf->crop_height : conf->output_height;
            if (in_w > conf->output_width || in_h > conf->output_height) {
                uint32_t hratio = ((uint64_t)in_w << 13) / conf->output_width;
                uint32_t vratio = ((uint64_t)in_h << 13) / conf->output_height;
                uint32_t hdiv = (1024UL * 8192UL - 1UL) / hratio;
                uint32_t vdiv = (1024UL * 8192UL - 1UL) / vratio;
                dcmipp->P2DSRTIOR = (hratio << DCMIPP_P2DSRTIOR_HRATIO_Pos)
                                  | (vratio << DCMIPP_P2DSRTIOR_VRATIO_Pos);
                dcmipp->P2DSSZR = (conf->output_width << DCMIPP_P2DSSZR_HSIZE_Pos)
                                | (conf->output_height << DCMIPP_P2DSSZR_VSIZE_Pos);
                dcmipp->P2DSCR = (hdiv << DCMIPP_P2DSCR_HDIV_Pos)
                               | (vdiv << DCMIPP_P2DSCR_VDIV_Pos)
                               | DCMIPP_P2DSCR_ENABLE;
            } else {
                dcmipp->P2DSCR &= ~DCMIPP_P2DSCR_ENABLE;
            }
        } else {
            dcmipp->P2DSCR &= ~DCMIPP_P2DSCR_ENABLE;
        }

        dcmipp->P2GMCR &= ~DCMIPP_P2GMCR_ENABLE;

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

    dcmipp->IPGR2 |= DCMIPP_IPGR2_PSTART;

    volatile uint32_t *r1 = NULL, *r2 = NULL, *r3 = NULL;

    switch (conf->client_id) {
        case DCMIPP_CLIENT1: r1 = &dcmipp->IPC1R1; r2 = &dcmipp->IPC1R2; r3 = &dcmipp->IPC1R3; break;
        case DCMIPP_CLIENT2: r1 = &dcmipp->IPC2R1; r2 = &dcmipp->IPC2R2; r3 = &dcmipp->IPC2R3; break;
        case DCMIPP_CLIENT3: r1 = &dcmipp->IPC3R1; r2 = &dcmipp->IPC3R2; r3 = &dcmipp->IPC3R3; break;
        case DCMIPP_CLIENT4: r1 = &dcmipp->IPC4R1; r2 = &dcmipp->IPC4R2; r3 = &dcmipp->IPC4R3; break;
        case DCMIPP_CLIENT5: r1 = &dcmipp->IPC5R1; r2 = &dcmipp->IPC5R2; r3 = &dcmipp->IPC5R3; break;
        default:
            dcmipp->IPGR2 &= ~DCMIPP_IPGR2_PSTART;
        	return;
    }

    *r1 = (conf->traffic & DCMIPP_IPC1R1_TRAFFIC_Msk) | ((conf->outstanding << DCMIPP_IPC1R1_OTR_Pos) & DCMIPP_IPC1R1_OTR_Msk);
    *r2 = (conf->wlru_ratio << DCMIPP_IPC1R2_WLRU_Pos) & DCMIPP_IPC1R2_WLRU_Msk;
    *r3 = ((conf->dpreg_start << DCMIPP_IPC1R3_DPREGSTART_Pos) & DCMIPP_IPC1R3_DPREGSTART_Msk) | ((conf->dpreg_end << DCMIPP_IPC1R3_DPREGEND_Pos) & DCMIPP_IPC1R3_DPREGEND_Msk);

    dcmipp->IPGR2 &= ~DCMIPP_IPGR2_PSTART;
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
        dcmipp->P1PPM0AR2 = 0;
        if (mode == 0)
            dcmipp->P1FCTCR &= ~DCMIPP_P1FCTCR_CPTMODE;
        else
            dcmipp->P1FCTCR |= DCMIPP_P1FCTCR_CPTMODE;
        dcmipp->P1FSCR |= DCMIPP_P1FSCR_PIPEN;
        dcmipp->P1FCTCR |= DCMIPP_P1FCTCR_CPTREQ;

    } else if (pipe == DCMIPP_PIPE2) {
        dcmipp->P2PPM0AR1 = buf_addr;
        if (mode == 0)
            dcmipp->P2FCTCR &= ~DCMIPP_P2FCTCR_CPTMODE;
        else
            dcmipp->P2FCTCR |= DCMIPP_P2FCTCR_CPTMODE;
        dcmipp->P2FSCR |= DCMIPP_P2FSCR_PIPEN;
        dcmipp->P2FCTCR |= DCMIPP_P2FCTCR_CPTREQ;
    }
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
    dcmipp->CMFCR = 0xFFFFFFFFU;

    uint32_t sr = dcmipp->P1SR;
    if (sr) {
        if (sr & DCMIPP_P1SR_LINEF)  { dcmipp->P1FCR = DCMIPP_P1FCR_CLINEF;  DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE1); }
        if (sr & DCMIPP_P1SR_FRAMEF) { dcmipp->P1FCR = DCMIPP_P1FCR_CFRAMEF; DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE1); }
        if (sr & DCMIPP_P1SR_VSYNCF) { dcmipp->P1FCR = DCMIPP_P1FCR_CVSYNCF; }
        if (sr & DCMIPP_P1SR_OVRF)   { dcmipp->P1FCR = DCMIPP_P1FCR_COVRF;   DCMIPP_PIPE_ErrorCallback(DCMIPP_PIPE1); }
    }

    sr = dcmipp->P2SR;
    if (sr) {
        if (sr & DCMIPP_P2SR_LINEF)  { dcmipp->P2FCR = DCMIPP_P2FCR_CLINEF;  DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE2); }
        if (sr & DCMIPP_P2SR_FRAMEF) { dcmipp->P2FCR = DCMIPP_P2FCR_CFRAMEF; DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE2); }
        if (sr & DCMIPP_P2SR_VSYNCF) { dcmipp->P2FCR = DCMIPP_P2FCR_CVSYNCF; }
        if (sr & DCMIPP_P2SR_OVRF)   { dcmipp->P2FCR = DCMIPP_P2FCR_COVRF;   DCMIPP_PIPE_ErrorCallback(DCMIPP_PIPE2); }
    }
}

__attribute__((weak)) void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_VsyncEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_ErrorCallback(uint32_t pipe) { (void)pipe; }
