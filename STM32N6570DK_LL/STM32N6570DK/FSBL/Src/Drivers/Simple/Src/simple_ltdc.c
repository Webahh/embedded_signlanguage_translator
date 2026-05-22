/*
 * simple_ltdc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */

#include "simple_ltdc.h"
#include "simple_rcc.h"

static void LTDC_ConfigTiming(const LTDC_ConfigTypeDef* cfg){
    uint32_t accumulated_hbp;
    uint32_t accumulated_vbp;
    uint32_t accumulated_active_w;
    uint32_t accumulated_active_h;
    uint32_t total_w;
    uint32_t total_h;

    accumulated_hbp      = cfg->hsync + cfg->hbp;
    accumulated_vbp      = cfg->vsync + cfg->vbp;
    accumulated_active_w = accumulated_hbp + cfg->width;
    accumulated_active_h = accumulated_vbp + cfg->height;
    total_w              = accumulated_active_w + cfg->hfp;
    total_h              = accumulated_active_h + cfg->vfp;

    LTDC->SSCR =
        ((uint32_t)(cfg->hsync - 1U) << LTDC_SSCR_HSW_Pos) |
        ((uint32_t)(cfg->vsync - 1U) << LTDC_SSCR_VSH_Pos);

    LTDC->BPCR =
        ((uint32_t)(accumulated_hbp - 1U) << LTDC_BPCR_AHBP_Pos) |
        ((uint32_t)(accumulated_vbp - 1U) << LTDC_BPCR_AVBP_Pos);

    LTDC->AWCR =
        ((uint32_t)(accumulated_active_w - 1U) << LTDC_AWCR_AAW_Pos) |
        ((uint32_t)(accumulated_active_h - 1U) << LTDC_AWCR_AAH_Pos);

    LTDC->TWCR =
        ((uint32_t)(total_w - 1U) << LTDC_TWCR_TOTALW_Pos) |
        ((uint32_t)(total_h - 1U) << LTDC_TWCR_TOTALH_Pos);
}

static void LTDC_ConfigLayer1(const LTDC_ConfigTypeDef* cfg){
    uint32_t accumulated_hbp = cfg->hsync + cfg->hbp;
    uint32_t accumulated_vbp = cfg->vsync + cfg->vbp;

    uint32_t window_x0 = accumulated_hbp;
    uint32_t window_x1 = accumulated_hbp + cfg->width - 1U;

    uint32_t window_y0 = accumulated_vbp;
    uint32_t window_y1 = accumulated_vbp + cfg->height - 1U;

    uint32_t bytes_per_pixel = 2U; // RGB565
    uint32_t line_pitch      = cfg->width * bytes_per_pixel;
    uint32_t line_length     = line_pitch + 7U;

    LTDC_Layer1->CR &= ~LTDC_LxCR_LEN;

    LTDC_Layer1->WHPCR =
        (window_x0 << LTDC_LxWHPCR_WHSTPOS_Pos) |
        (window_x1 << LTDC_LxWHPCR_WHSPPOS_Pos);

    LTDC_Layer1->WVPCR =
        (window_y0 << LTDC_LxWVPCR_WVSTPOS_Pos) |
        (window_y1 << LTDC_LxWVPCR_WVSPPOS_Pos);

    LTDC_Layer1->PFCR = cfg->pixel_format;

    LTDC_Layer1->CACR = 255U;

    LTDC_Layer1->DCCR = 0x00000000U;

    LTDC_Layer1->BFCR =
        (6U << LTDC_LxBFCR_BF1_Pos) |
        (7U << LTDC_LxBFCR_BF2_Pos);

    LTDC_Layer1->CFBAR = cfg->framebuffer;

    LTDC_Layer1->CFBLR =
        (line_length << LTDC_LxCFBLR_CFBLL_Pos) |
        (line_pitch  << LTDC_LxCFBLR_CFBP_Pos);

    LTDC_Layer1->CFBLNR = cfg->height;

    LTDC_Layer1->CR |= LTDC_LxCR_LEN;
}

void LTDC_Config(const LTDC_ConfigTypeDef* cfg){
    RCC_enable_LTDC();
    RCC_reset_LTDC();

    LTDC->GCR &= ~LTDC_GCR_LTDCEN;

    LTDC_ConfigTiming(cfg);

    LTDC->BCCR = 0x00000000U;

    LTDC_ConfigLayer1(cfg);

    LTDC_Reload();

    LTDC_Enable();
}

void LTDC_Reload(void){
    LTDC->SRCR = LTDC_SRCR_IMR;
}

void LTDC_Enable(void){
    LTDC->GCR |= LTDC_GCR_LTDCEN;
}



