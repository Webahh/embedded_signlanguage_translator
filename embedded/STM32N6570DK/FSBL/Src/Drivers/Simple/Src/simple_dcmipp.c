/**
  ******************************************************************************
  * @file    simple_dcmipp.c
  * @author  Groß
  * @brief   Register-level DCMIPP + CSI-2 driver for STM32N6
  *          Supports two-pipe setup: PIPE1 (display) and PIPE2 (NN)
  ******************************************************************************
  */

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
                  | DCMIPP_CMIER_P2OVRIE;

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
    volatile uint32_t *crstr, *crszr, *dsrtior, *dsszr, *dscr, *gmcr, *ppcr, *ppm0pr, *fctcr;
    volatile uint32_t *decr, *dccr, *blccr, *excr1, *excr2, *st1cr, *st2cr, *st3cr;

    if (!conf) conf = &zero_conf;

    pitch = DCMIPP_AlignPitch(conf->output_width * conf->output_bpp);

    if (pipe == DCMIPP_PIPE1) {
        crstr   = &dcmipp->P1CRSTR;   crszr   = &dcmipp->P1CRSZR;
        dsrtior = &dcmipp->P1DSRTIOR; dsszr   = &dcmipp->P1DSSZR;
        dscr    = &dcmipp->P1DSCR;    gmcr    = &dcmipp->P1GMCR;
        ppcr    = &dcmipp->P1PPCR;    ppm0pr  = &dcmipp->P1PPM0PR;
        fctcr   = &dcmipp->P1FCTCR;   decr    = &dcmipp->P1DECR;
        dccr    = &dcmipp->P1DECR;
        blccr   = &dcmipp->P1BLCCR;   excr1   = &dcmipp->P1EXCR1;
        excr2   = &dcmipp->P1EXCR2;   st1cr   = &dcmipp->P1ST1CR;
        st2cr   = &dcmipp->P1ST2CR;   st3cr   = &dcmipp->P1ST3CR;
    } else {
        crstr   = &dcmipp->P2CRSTR;   crszr   = &dcmipp->P2CRSZR;
        dsrtior = &dcmipp->P2DSRTIOR; dsszr   = &dcmipp->P2DSSZR;
        dscr    = &dcmipp->P2DSCR;    gmcr    = &dcmipp->P2GMCR;
        ppcr    = &dcmipp->P2PPCR;    ppm0pr  = &dcmipp->P2PPM0PR;
        fctcr   = &dcmipp->P2FCTCR;   decr    = &dcmipp->P1DECR;
        dccr    = &dcmipp->P2DCCR;
        blccr   = &dcmipp->P1BLCCR;   excr1   = &dcmipp->P1EXCR1;
        excr2   = &dcmipp->P1EXCR2;   st1cr   = &dcmipp->P1ST1CR;
        st2cr   = &dcmipp->P1ST2CR;   st3cr   = &dcmipp->P1ST3CR;
    }

    if (conf->enable_crop && conf->crop_width && conf->crop_height) {
        *crstr = (conf->crop_x << DCMIPP_P1CRSTR_HSTART_Pos)
               | (conf->crop_y << DCMIPP_P1CRSTR_VSTART_Pos);
        *crszr = (conf->crop_width << DCMIPP_P1CRSZR_HSIZE_Pos)
               | (conf->crop_height << DCMIPP_P1CRSZR_VSIZE_Pos)
               | DCMIPP_P1CRSZR_ENABLE;
    } else {
        *crszr &= ~DCMIPP_P1CRSZR_ENABLE;
    }

    if (conf->enable_downsize && conf->output_width && conf->output_height) {
        uint32_t in_w = conf->enable_crop ? conf->crop_width : conf->output_width;
        uint32_t in_h = conf->enable_crop ? conf->crop_height : conf->output_height;
        if (conf->enable_decimate) {
            in_w >>= conf->decimate_h;
            in_h >>= conf->decimate_v;
        }
        if (in_w > conf->output_width || in_h > conf->output_height) {
            uint32_t hratio = ((uint64_t)in_w << 13) / conf->output_width;
            uint32_t vratio = ((uint64_t)in_h << 13) / conf->output_height;
            *dsrtior = (hratio << DCMIPP_P1DSRTIOR_HRATIO_Pos)
                     | (vratio << DCMIPP_P1DSRTIOR_VRATIO_Pos);
            *dsszr = (conf->output_width << DCMIPP_P1DSSZR_HSIZE_Pos)
                   | (conf->output_height << DCMIPP_P1DSSZR_VSIZE_Pos);
            *dscr = ((1024UL * 8192UL - 1UL) / hratio << DCMIPP_P1DSCR_HDIV_Pos)
                  | ((1024UL * 8192UL - 1UL) / vratio << DCMIPP_P1DSCR_VDIV_Pos)
                  | DCMIPP_P1DSCR_ENABLE;
        } else {
            *dscr &= ~DCMIPP_P1DSCR_ENABLE;
        }
    } else {
        *dscr &= ~DCMIPP_P1DSCR_ENABLE;
    }

    *decr  = DCMIPP_P1DECR_ENABLE;

<<<<<<< Updated upstream
    *blccr = DCMIPP_P1BLCCR_ENABLE
           | DCMIPP_P1BLCCR_BLCB
           | DCMIPP_P1BLCCR_BLCG
           | DCMIPP_P1BLCCR_BLCR;
=======
    	dcmipp->P1CCCR  = DCMIPP_P1CCCR_ENABLE;
    	dcmipp->P1CCRR1 = 0x7fb0188;
    	dcmipp->P1CCRR2 = 0x77d;
    	dcmipp->P1CCGR1 = 0x1e8079a;
    	dcmipp->P1CCGR2 = 0x77f;
    	dcmipp->P1CCBR1 = 0x79f07e3;
    	dcmipp->P1CCBR2 = 0x17e;
>>>>>>> Stashed changes

    *excr1 = DCMIPP_P1EXCR1_ENABLE
           | ((0x93U << DCMIPP_P1EXCR1_MULTR_Pos) & DCMIPP_P1EXCR1_MULTR_Msk)
           | ((0x1U  << DCMIPP_P1EXCR1_SHFR_Pos)  & DCMIPP_P1EXCR1_SHFR_Msk);

<<<<<<< Updated upstream
    *excr2 = ((0xCBU << DCMIPP_P1EXCR2_MULTB_Pos) & DCMIPP_P1EXCR2_MULTB_Msk)
           | ((0x0U  << DCMIPP_P1EXCR2_SHFB_Pos)  & DCMIPP_P1EXCR2_SHFB_Msk)
           | ((0x80U << DCMIPP_P1EXCR2_MULTG_Pos) & DCMIPP_P1EXCR2_MULTG_Msk)
           | ((0x0U  << DCMIPP_P1EXCR2_SHFG_Pos)  & DCMIPP_P1EXCR2_SHFG_Msk);
=======
        dcmipp->P1EXCR2 =  ((0xCBU << DCMIPP_P1EXCR2_MULTB_Pos) & DCMIPP_P1EXCR2_MULTB_Msk)
						|  ((0x0U  << DCMIPP_P1EXCR2_SHFB_Pos)  & DCMIPP_P1EXCR2_SHFB_Msk)
						|  ((0x80U << DCMIPP_P1EXCR2_MULTG_Pos) & DCMIPP_P1EXCR2_MULTG_Msk)
						|  ((0x0U  << DCMIPP_P1EXCR2_SHFG_Pos)  & DCMIPP_P1EXCR2_SHFG_Msk);
>>>>>>> Stashed changes

    *st1cr = DCMIPP_P1ST1CR_ENABLE
           | ((0x4U << DCMIPP_P1ST1CR_SRC_Pos) & DCMIPP_P1ST1CR_SRC_Msk);

    *st2cr = DCMIPP_P1ST2CR_ENABLE
           | ((0x5U << DCMIPP_P1ST2CR_SRC_Pos) & DCMIPP_P1ST2CR_SRC_Msk);

    *st3cr = DCMIPP_P1ST3CR_ENABLE
           | ((0x6U << DCMIPP_P1ST3CR_SRC_Pos) & DCMIPP_P1ST3CR_SRC_Msk);

    if (conf->enable_swap)
        dcmipp->CMCR |= DCMIPP_CMCR_SWAPRB;
    else
        dcmipp->CMCR &= ~DCMIPP_CMCR_SWAPRB;

    if (conf->enable_gamma)
        *gmcr |= DCMIPP_P1GMCR_ENABLE;
    else
        *gmcr &= ~DCMIPP_P1GMCR_ENABLE;

    /* --- Decimation (per-pipe: P1DECR for pipe1, P2DCCR for pipe2) --- */
    if (conf->enable_decimate) {
        *dccr = DCMIPP_P1DECR_ENABLE
              | (conf->decimate_h << DCMIPP_P1DECR_HDEC_Pos)
              | (conf->decimate_v << DCMIPP_P1DECR_VDEC_Pos);
    }

    *ppcr = (*ppcr & ~DCMIPP_P1PPCR_FORMAT_Msk)
          | (conf->output_format << DCMIPP_P1PPCR_FORMAT_Pos);
    if (conf->enable_dbm)
        *ppcr |= DCMIPP_P1PPCR_DBM;
    *ppm0pr = pitch;
    *fctcr = (*fctcr & ~DCMIPP_P1FCTCR_FRATE_Msk) | 0;

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

    *r1 = (conf->traffic & DCMIPP_IPC1R1_TRAFFIC_Msk)
        | ((conf->outstanding << DCMIPP_IPC1R1_OTR_Pos) & DCMIPP_IPC1R1_OTR_Msk);
    *r2 = (conf->wlru_ratio << DCMIPP_IPC1R2_WLRU_Pos) & DCMIPP_IPC1R2_WLRU_Msk;
    *r3 = ((conf->dpreg_start << DCMIPP_IPC1R3_DPREGSTART_Pos) & DCMIPP_IPC1R3_DPREGSTART_Msk)
        | ((conf->dpreg_end << DCMIPP_IPC1R3_DPREGEND_Pos) & DCMIPP_IPC1R3_DPREGEND_Msk);

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

void DCMIPP_ReduceSpurious(void)
{
    dcmipp->P1FCR = DCMIPP_P1FCR_CLINEF;
    dcmipp->P2FCR = DCMIPP_P2FCR_CLINEF;
    dcmipp->P1IER |= DCMIPP_P1IER_LINEIE;
    dcmipp->P2IER |= DCMIPP_P2IER_LINEIE;
    dcmipp->P1FCR = DCMIPP_P1FCR_CLINEF;
    dcmipp->P2FCR = DCMIPP_P2FCR_CLINEF;
    dcmipp->P1IER &= ~DCMIPP_P1IER_LINEIE;
    dcmipp->P2IER &= ~DCMIPP_P2IER_LINEIE;
}

void DCMIPP_Pipe_Start(uint32_t pipe, uint32_t buf_addr, uint32_t mode)
{
    if (pipe == DCMIPP_PIPE1) {
        if (!(dcmipp->P1PPCR & DCMIPP_P1PPCR_DBM)) {
            dcmipp->P1PPM0AR1 = buf_addr;
            dcmipp->P1PPM0AR2 = 0;
        }
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

void DCMIPP_IRQHandler(void){
    uint32_t cmsr1 = dcmipp->CMSR1;
    uint32_t cmsr2 = dcmipp->CMSR2;

    uint32_t p1sr = dcmipp->P1SR;
    uint32_t p2sr = dcmipp->P2SR;

    (void)cmsr1;
    (void)cmsr2;

    if (p1sr) {
        if (p1sr & DCMIPP_P1SR_LINEF)  { dcmipp->P1FCR = DCMIPP_P1FCR_CLINEF;  }
        if (p1sr & DCMIPP_P1SR_FRAMEF) { dcmipp->P1FCR = DCMIPP_P1FCR_CFRAMEF; DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE1); }
        if (p1sr & DCMIPP_P1SR_VSYNCF) { dcmipp->P1FCR = DCMIPP_P1FCR_CVSYNCF; DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE1); }
        if (p1sr & DCMIPP_P1SR_OVRF)   { dcmipp->P1FCR = DCMIPP_P1FCR_COVRF;   DCMIPP_PIPE_ErrorCallback(DCMIPP_PIPE1); }
    }

    if (p2sr) {
        if (p2sr & DCMIPP_P2SR_LINEF)  { dcmipp->P2FCR = DCMIPP_P2FCR_CLINEF;  }
        if (p2sr & DCMIPP_P2SR_FRAMEF) { dcmipp->P2FCR = DCMIPP_P2FCR_CFRAMEF; DCMIPP_PIPE_FrameEventCallback(DCMIPP_PIPE2); }
        if (p2sr & DCMIPP_P2SR_VSYNCF) { dcmipp->P2FCR = DCMIPP_P2FCR_CVSYNCF; DCMIPP_PIPE_VsyncEventCallback(DCMIPP_PIPE2); }
        if (p2sr & DCMIPP_P2SR_OVRF)   { dcmipp->P2FCR = DCMIPP_P2FCR_COVRF;   DCMIPP_PIPE_ErrorCallback(DCMIPP_PIPE2); }
    }

    dcmipp->CMFCR = 0xFFFFFFFFU;
}

__attribute__((weak)) void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_VsyncEventCallback(uint32_t pipe) { (void)pipe; }
__attribute__((weak)) void DCMIPP_PIPE_ErrorCallback(uint32_t pipe)     { (void)pipe; }
