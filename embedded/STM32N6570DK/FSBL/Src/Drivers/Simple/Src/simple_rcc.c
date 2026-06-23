/*
 * simple_rcc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 *
 * Provides helper functions for:
 * - basic system clock setup
 * - reading derived bus clocks
 * - enabling and resetting selected peripherals
 * - configuring selected peripheral clock sources
 *
 * This module intentionally supports only the clock sources and
 * peripherals currently used by the project.
 *
 */

#include <stdint.h>

#include "simple_rcc.h"

#include "config.h"

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

/**
 * @brief Wait for HSI oscillator ready flag
 */
static void RCC_WaitHSIReady(void){
    while (!(RCC->SR & RCC_SR_HSIRDY)) { }
}

/**
 * @brief Decode AHB prescaler bits to divider value
 *
 * @param [in] hpre_bits HPRE field value
 *
 * @return Prescaler divider (1, 2, 4, 8, 16, 32, 64, 128)
 */
static uint32_t RCC_GetAHBPrescalerDiv(uint32_t hpre_bits){
    static const uint8_t _ahb_div[8] = {
        1, 2, 4, 8, 16, 32, 64, 128
    };

    return _ahb_div[hpre_bits & 0x7U];
}

/**
 * @brief Decode APB prescaler bits to divider value
 *
 * @param [in] ppre_bits PPRE field value
 *
 * @return Prescaler divider (1, 2, 4, 8, 16)
 */
static uint32_t RCC_GetAPBPrescalerDiv(uint32_t ppre_bits){
    static const uint8_t _apb_div[8] = {
        1, 1, 1, 1, 2, 4, 8, 16
    };

    return _apb_div[ppre_bits & 0x7U];
}

// -------------------------------------------------------------------------
// Clock configuration
// -------------------------------------------------------------------------

/**
 * @brief Configure a minimal system clock setup.
 *
 * Enables HSI and selects it as SYSCLK and CPUCLK.
 * All relevant AHB/APB prescalers are set to /1.
 *
 * Resulting clock tree:
 * - HSI    = 64 MHz
 * - SYSCLK = HSI
 * - CPUCLK = HSI
 * - HCLK   = SYSCLK / 1
 * - PCLK1  = HCLK / 1
 * - PCLK2  = HCLK / 1
 * - PCLK4  = HCLK / 1
 * - PCLK5  = HCLK / 1
 */
void RCC_SystemClock_Config(void){

    /* Enable HSI */
    RCC->CR |= RCC_CR_HSION;
    RCC_WaitHSIReady();

    /* Set prescalers to /1 */
    RCC->CFGR2 &= ~(RCC_CFGR2_HPRE  |
                    RCC_CFGR2_PPRE1 |
                    RCC_CFGR2_PPRE2 |
                    RCC_CFGR2_PPRE4 |
                    RCC_CFGR2_PPRE5);

    /*
     * Set HSI as SYSCLK and CPUCLK
     * For these fields, clearing the switch bits selects HSI.
     */
    RCC->CFGR1 &= ~(RCC_CFGR1_SYSSW |
                    RCC_CFGR1_CPUSW);

    while ((RCC->CFGR1 & RCC_CFGR1_SYSSWS) != 0U) { }
    while ((RCC->CFGR1 & RCC_CFGR1_CPUSWS) != 0U) { }
}

static const uint32_t _PLL_ON[4] = {
    RCC_CR_PLL1ON, RCC_CR_PLL2ON, RCC_CR_PLL3ON, RCC_CR_PLL4ON,
};
static const uint32_t _PLL_RDY[4] = {
    RCC_SR_PLL1RDY, RCC_SR_PLL2RDY, RCC_SR_PLL3RDY, RCC_SR_PLL4RDY,
};

static volatile uint32_t* const _PLL_CFGR1[4] = {
    &RCC->PLL1CFGR1, &RCC->PLL2CFGR1, &RCC->PLL3CFGR1, &RCC->PLL4CFGR1,
};
static volatile uint32_t* const _PLL_CFGR2[4] = {
    &RCC->PLL1CFGR2, &RCC->PLL2CFGR2, &RCC->PLL3CFGR2, &RCC->PLL4CFGR2,
};
static volatile uint32_t* const _PLL_CFGR3[4] = {
    &RCC->PLL1CFGR3, &RCC->PLL2CFGR3, &RCC->PLL3CFGR3, &RCC->PLL4CFGR3,
};

/**
 * @brief Program all PLLs from configuration table
 *
 * @param [in] pll Array of 4 PLL configurations
 */
void RCC_config_PLLs(const RCC_PLL_cfg_TypeDef pll[4]){
    for (uint32_t i = 0U; i < 4U; i++) {
        if (RCC->SR & _PLL_RDY[i]) continue;

        *_PLL_CFGR1[i] = pll[i].CFGR1;
        *_PLL_CFGR2[i] = pll[i].CFGR2;
        *_PLL_CFGR3[i] = pll[i].CFGR3;

        RCC->CR |= _PLL_ON[i];
        while (!(RCC->SR & _PLL_RDY[i])) { }
    }
}

static volatile uint32_t* const _IC_CFGR[20] = {
    &RCC->IC1CFGR,  &RCC->IC2CFGR,  &RCC->IC3CFGR,  &RCC->IC4CFGR,
    &RCC->IC5CFGR,  &RCC->IC6CFGR,  &RCC->IC7CFGR,  &RCC->IC8CFGR,
    &RCC->IC9CFGR,  &RCC->IC10CFGR, &RCC->IC11CFGR, &RCC->IC12CFGR,
    &RCC->IC13CFGR, &RCC->IC14CFGR, &RCC->IC15CFGR, &RCC->IC16CFGR,
    &RCC->IC17CFGR, &RCC->IC18CFGR, &RCC->IC19CFGR, &RCC->IC20CFGR,
};
static const uint32_t _IC_DIVEN[20] = {
    RCC_DIVENR_IC1EN,  RCC_DIVENR_IC2EN,  RCC_DIVENR_IC3EN,  RCC_DIVENR_IC4EN,
    RCC_DIVENR_IC5EN,  RCC_DIVENR_IC6EN,  RCC_DIVENR_IC7EN,  RCC_DIVENR_IC8EN,
    RCC_DIVENR_IC9EN,  RCC_DIVENR_IC10EN, RCC_DIVENR_IC11EN, RCC_DIVENR_IC12EN,
    RCC_DIVENR_IC13EN, RCC_DIVENR_IC14EN, RCC_DIVENR_IC15EN, RCC_DIVENR_IC16EN,
    RCC_DIVENR_IC17EN, RCC_DIVENR_IC18EN, RCC_DIVENR_IC19EN, RCC_DIVENR_IC20EN,
};

/**
 * @brief Program internal clock dividers (ICs) from configuration table
 *
 * Zero-valued entries are skipped.
 *
 * @param [in] ic Array of 20 IC configurations
 */
void RCC_config_ICs(const RCC_IC_cfg_TypeDef ic[20]){
    for (uint32_t i = 0U; i < 20U; i++) {
        if (ic[i].CFGR == 0U) continue;

        *_IC_CFGR[i] = ic[i].CFGR;
        RCC->DIVENR |= _IC_DIVEN[i];
    }
    (void)RCC->DIVENR;
}

/**
 * @brief Full board clock configuration
 */
void RCC_BoardClock_Config(void){
    RCC_SystemClock_Config();

    RCC_config_PLLs(RCC_PLL_cfg);

    RCC_config_ICs(RCC_IC_cfg);

    /*
     * RCC_config_ICs() skips zero-valued entries, therefore IC1 must be
     * enabled explicitly.
     */
    RCC->IC1CFGR = 0x00000000;
    RCC->DIVENR |= RCC_DIVENR_IC1EN;
    (void)RCC->DIVENR;

    RCC->CFGR2 = 0x00100000;
    (void)RCC->CFGR2;

    RCC->CFGR1 = 0x33330000;
    (void)RCC->CFGR1;
}

// -------------------------------------------------------------------------
// Clock getters
// -------------------------------------------------------------------------

/**
 * @brief Get the HSI oscillator frequency
 *
 * @param [out] hz HSI frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetHSI(uint32_t* hz){
    *hz = RCC_HSI_VALUE_HZ;
    return RCC_OK;
}

/**
 * @brief Compute PLL output frequency
 *
 * @param [in]  pll_idx PLL index (0-3)
 * @param [out] hz      PLL frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Divider zero – cannot compute
 */
static RCC_Status_TypeDef RCC_GetPLLFreq(uint32_t pll_idx, uint32_t* hz){
    volatile uint32_t* cfgr1 = _PLL_CFGR1[pll_idx];
    volatile uint32_t* cfgr3 = _PLL_CFGR3[pll_idx];

    uint32_t divm = (*cfgr1 >> 20) & 0x3F;
    uint32_t divn = (*cfgr1 >> 8) & 0xFFF;
    uint32_t pdiv1 = (*cfgr3 >> 27) & 0x7;
    uint32_t pdiv2 = (*cfgr3 >> 24) & 0x7;

    if (divm == 0 || (pdiv1 * pdiv2) == 0) {
        return RCC_ERROR;
    }

    uint32_t hsi;
    RCC_GetHSI(&hsi);

    uint32_t vco_in = hsi / divm;
    *hz = (vco_in * divn) / (pdiv1 * pdiv2);
    return RCC_OK;
}

/**
 * @brief Compute internal clock divider (IC) output frequency
 *
 * @param [in]  ic_idx IC index (0-19)
 * @param [out] hz     IC output frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR PLL not ready – frequency unknown
 */
static RCC_Status_TypeDef RCC_GetICFreq(uint32_t ic_idx, uint32_t* hz){
    uint32_t cfgr = *_IC_CFGR[ic_idx];
    uint32_t sel = (cfgr >> 28) & 0x3;
    uint32_t div = ((cfgr >> 16) & 0xFF) + 1;

    uint32_t pll_freq;
    RCC_Status_TypeDef status = RCC_GetPLLFreq(sel, &pll_freq);
    if (status != RCC_OK) {
        return status;
    }

    *hz = pll_freq / div;
    return RCC_OK;
}

/**
 * @brief Get the SYSCLK frequency
 *
 * @param [out] hz SYSCLK frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unknown clock source selected
 */
RCC_Status_TypeDef RCC_GetSYSCLK(uint32_t* hz){
    uint32_t syssws =
        (RCC->CFGR1 & RCC_CFGR1_SYSSWS) >> RCC_CFGR1_SYSSWS_Pos;

    switch (syssws) {
        case 0x0U:
            return RCC_GetHSI(hz);

        case 0x3U:
            return RCC_GetICFreq(0, hz);

        default:
            return RCC_ERROR;
    }
}

/**
 * @brief Get the CPU clock frequency
 *
 * @param [out] hz CPU clock frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unknown clock source selected
 */
RCC_Status_TypeDef RCC_GetCPUCLK(uint32_t* hz){
    uint32_t cpusws =
        (RCC->CFGR1 & RCC_CFGR1_CPUSWS) >> RCC_CFGR1_CPUSWS_Pos;

    switch (cpusws) {
        case 0x0U:
            return RCC_GetHSI(hz);

        case 0x3U:
            return RCC_GetICFreq(0, hz);

        default:
            return RCC_ERROR;
    }
}

/**
 * @brief Get the AXI bus clock frequency
 *
 * @param [out] hz AXI clock frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR PLL or IC not ready
 */
RCC_Status_TypeDef RCC_GetAXICLK(uint32_t* hz){
    return RCC_GetICFreq(1, hz);
}

/**
 * @brief Get the HCLK (AHB bus) frequency
 *
 * @param [out] hz HCLK frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Prescaler decode failure
 */
RCC_Status_TypeDef RCC_GetHCLK(uint32_t* hz){
    uint32_t axiclk;
    RCC_Status_TypeDef status = RCC_GetAXICLK(&axiclk);
    if (status != RCC_OK) {
        return status;
    }

    uint32_t hpre_bits =
        (RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos;

    uint32_t hpre_div = RCC_GetAHBPrescalerDiv(hpre_bits);
    if (hpre_div == 0U) {
        return RCC_ERROR;
    }

    *hz = axiclk / hpre_div;
    return RCC_OK;
}

/**
 * @brief Get the PCLK1 (APB1 bus) frequency
 *
 * @param [out] hz PCLK1 frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Prescaler decode failure
 */
RCC_Status_TypeDef RCC_GetPCLK1(uint32_t* hz){
    uint32_t hclk;
    RCC_Status_TypeDef status = RCC_GetHCLK(&hclk);
    if (status != RCC_OK) {
        return status;
    }

    uint32_t ppre1_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE1) >> RCC_CFGR2_PPRE1_Pos;

    uint32_t ppre1_div = RCC_GetAPBPrescalerDiv(ppre1_bits);
    if (ppre1_div == 0U) {
        return RCC_ERROR;
    }

    *hz = hclk / ppre1_div;
    return RCC_OK;
}

/**
 * @brief Get the PCLK2 (APB2 bus) frequency
 *
 * @param [out] hz PCLK2 frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Prescaler decode failure
 */
RCC_Status_TypeDef RCC_GetPCLK2(uint32_t* hz){
    uint32_t hclk;
    RCC_Status_TypeDef status = RCC_GetHCLK(&hclk);
    if (status != RCC_OK) {
        return status;
    }

    uint32_t ppre2_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE2) >> RCC_CFGR2_PPRE2_Pos;

    uint32_t ppre2_div = RCC_GetAPBPrescalerDiv(ppre2_bits);
    if (ppre2_div == 0U) {
        return RCC_ERROR;
    }

    *hz = hclk / ppre2_div;
    return RCC_OK;
}

/**
 * @brief Get the PCLK4 (APB4 bus) frequency
 *
 * @param [out] hz PCLK4 frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Prescaler decode failure
 */
RCC_Status_TypeDef RCC_GetPCLK4(uint32_t* hz){
    uint32_t hclk;
    RCC_Status_TypeDef status = RCC_GetHCLK(&hclk);
    if (status != RCC_OK) {
        return status;
    }

    uint32_t ppre4_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE4) >> RCC_CFGR2_PPRE4_Pos;

    uint32_t ppre4_div = RCC_GetAPBPrescalerDiv(ppre4_bits);
    if (ppre4_div == 0U) {
        return RCC_ERROR;
    }

    *hz = hclk / ppre4_div;
    return RCC_OK;
}

/**
 * @brief Get the PCLK5 (APB5 bus) frequency
 *
 * @param [out] hz PCLK5 frequency in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Prescaler decode failure
 */
RCC_Status_TypeDef RCC_GetPCLK5(uint32_t* hz){
    uint32_t hclk;
    RCC_Status_TypeDef status = RCC_GetHCLK(&hclk);
    if (status != RCC_OK) {
        return status;
    }

    uint32_t ppre5_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE5) >> RCC_CFGR2_PPRE5_Pos;

    uint32_t ppre5_div = RCC_GetAPBPrescalerDiv(ppre5_bits);
    if (ppre5_div == 0U) {
        return RCC_ERROR;
    }

    *hz = hclk / ppre5_div;
    return RCC_OK;
}

/**
 * @brief Get the timer input clock frequency
 *
 * TIM2..TIM7 are connected to APB1.
 * TIM1, TIM8 and TIM15..TIM17 are connected to APB2.
 *
 * @param [in]  TIMX Timer peripheral instance
 * @param [out] hz   Timer clock in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unsupported timer
 */
RCC_Status_TypeDef RCC_GetTIMClock(TIM_TypeDef* TIMX, uint32_t* hz){
    if ((TIMX == TIM2) || (TIMX == TIM3) || (TIMX == TIM4) ||
        (TIMX == TIM5) || (TIMX == TIM6) || (TIMX == TIM7)) {
        return RCC_GetPCLK1(hz);
    }

    if ((TIMX == TIM1) || (TIMX == TIM8) ||
        (TIMX == TIM15) || (TIMX == TIM16) || (TIMX == TIM17)) {
        return RCC_GetPCLK2(hz);
    }

    return RCC_ERROR;
}

/**
 * @brief Get the I2C peripheral clock frequency
 *
 * @param [in]  I2CX I2C peripheral instance
 * @param [out] hz   I2C clock in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unsupported I2C instance
 */
RCC_Status_TypeDef RCC_GetI2CClock(I2C_TypeDef* I2CX, uint32_t* hz){
    if ((I2CX == I2C1) || (I2CX == I2C2) || (I2CX == I2C3)) {
        return RCC_GetPCLK1(hz);
    }

    if (I2CX == I2C4) {
        return RCC_GetPCLK4(hz);
    }

    return RCC_ERROR;
}

// -------------------------------------------------------------------------
// Peripheral clock / reset
// -------------------------------------------------------------------------

/**
 * @brief Enable GPIO port clock
 *
 * @param [in] GPIOX GPIO port instance
 */
void RCC_enable_GPIO(GPIO_TypeDef* GPIOX){
    if (GPIOX == GPIOA) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOB) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOC) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOD) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOE) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOF) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOFEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOG) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOGEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOH) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOHEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPION) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIONEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOO) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOOEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOP) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOPEN;
        (void)RCC->AHB4ENR;
    } else if (GPIOX == GPIOQ) {
        RCC->AHB4ENR |= RCC_AHB4ENR_GPIOQEN;
        (void)RCC->AHB4ENR;
    }
}

void RCC_enable_I2C(I2C_TypeDef* I2CX){
    if (I2CX == I2C1) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
        (void)RCC->APB1ENR1;
    } else if (I2CX == I2C2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
        (void)RCC->APB1ENR1;
    } else if (I2CX == I2C3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C3EN;
        (void)RCC->APB1ENR1;
    } else if (I2CX == I2C4) {
        RCC->APB4ENR1 |= RCC_APB4ENR1_I2C4EN;
        (void)RCC->APB4ENR1;
    }
}

void RCC_reset_I2C(I2C_TypeDef* I2CX){
    if (I2CX == I2C1) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C1RST;
        (void)RCC->APB1RSTR1;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C1RST;
        (void)RCC->APB1RSTR1;
    } else if (I2CX == I2C2) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C2RST;
        (void)RCC->APB1RSTR1;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C2RST;
        (void)RCC->APB1RSTR1;
    } else if (I2CX == I2C3) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C3RST;
        (void)RCC->APB1RSTR1;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C3RST;
        (void)RCC->APB1RSTR1;
    } else if (I2CX == I2C4) {
        RCC->APB4RSTR1 |= RCC_APB4RSTR1_I2C4RST;
        (void)RCC->APB4RSTR1;
        RCC->APB4RSTR1 &= ~RCC_APB4RSTR1_I2C4RST;
        (void)RCC->APB4RSTR1;
    }
}

void RCC_setI2C_clock_source(I2C_TypeDef* I2CX, uint32_t source){
    source &= 0x7U;

    if (I2CX == I2C1) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C1SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C1SEL_Pos);
    } else if (I2CX == I2C2) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C2SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C2SEL_Pos);
    } else if (I2CX == I2C3) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C3SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C3SEL_Pos);
    } else if (I2CX == I2C4) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C4SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C4SEL_Pos);
    }

    (void)RCC->CCIPR4;
}

void RCC_enable_TIM(TIM_TypeDef* TIMX){
    if (TIMX == TIM1) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
        (void)RCC->APB2ENR;
    } else if (TIMX == TIM2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM4) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM5) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM6) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM7) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;
        (void)RCC->APB1ENR1;
    } else if (TIMX == TIM8) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
        (void)RCC->APB2ENR;
    } else if (TIMX == TIM15) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
        (void)RCC->APB2ENR;
    } else if (TIMX == TIM16) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;
        (void)RCC->APB2ENR;
    } else if (TIMX == TIM17) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
        (void)RCC->APB2ENR;
    }
}

// -------------------------------------------------------------------------
// LTDC
// -------------------------------------------------------------------------

/**
 * @brief Enable AXISRAM blocks used by the LTDC framebuffer
 */
void RCC_enable_LTDC_memory(void){
    RCC->MEMENR |= RCC_MEMENR_AXISRAM1EN
                |  RCC_MEMENR_AXISRAM2EN
                |  RCC_MEMENR_AXISRAM3EN
                |  RCC_MEMENR_AXISRAM4EN
                |  RCC_MEMENR_AXISRAM5EN
                |  RCC_MEMENR_AXISRAM6EN;

    (void)RCC->MEMENR;
}

void RCC_enable_LTDC(void){
    RCC->APB5ENSR |= RCC_APB5ENSR_LTDCENS;
    RCC->APB5LPENR |= RCC_APB5LPENR_LTDCLPEN;
    (void)RCC->APB5ENSR;
}

void RCC_reset_LTDC(void){
    RCC->APB5RSTSR |= RCC_APB5RSTSR_LTDCRSTS;
    (void)RCC->APB5RSTSR;

    RCC->APB5RSTCR |= RCC_APB5RSTCR_LTDCRSTC;
    (void)RCC->APB5RSTCR;
}

void RCC_setLTDC_clock_source(uint32_t source){
    source &= 0x3U;

    RCC->CCIPR4 &= ~RCC_CCIPR4_LTDCSEL;
    RCC->CCIPR4 |=  (source << RCC_CCIPR4_LTDCSEL_Pos);

    (void)RCC->CCIPR4;
}

/**
 * @brief Select IC16 as LTDC kernel clock source
 *
 * @note PLL4 and IC16 must already be configured before calling this.
 */
void RCC_config_LTDC_25MHz_clock(void){
    RCC->CCIPR4 = (RCC->CCIPR4 & ~RCC_CCIPR4_LTDCSEL) | RCC_CCIPR4_LTDCSEL_1;
    (void)RCC->CCIPR4;
}

// -------------------------------------------------------------------------
// DCMIPP / CSI
// -------------------------------------------------------------------------

void RCC_config_DCMIPP_clock_IC17(void){
    RCC->CCIPR1 = (RCC->CCIPR1 & ~RCC_CCIPR1_DCMIPPSEL_Msk)
                | (0x2UL << RCC_CCIPR1_DCMIPPSEL_Pos);
    (void)RCC->CCIPR1;
}

void RCC_enable_DCMIPP(void){
    RCC->APB5ENR |= RCC_APB5ENR_DCMIPPEN;
    (void)RCC->APB5ENR;

    RCC->APB5LPENR |= RCC_APB5LPENR_DCMIPPLPEN;
    (void)RCC->APB5LPENR;

    RCC->APB5ENSR |= RCC_APB5ENSR_DCMIPPENS;
    (void)RCC->APB5ENSR;
}

void RCC_reset_DCMIPP(void){
    RCC->APB5RSTSR = RCC_APB5RSTSR_DCMIPPRSTS;
    (void)RCC->APB5RSTSR;

    RCC->APB5RSTCR = RCC_APB5RSTCR_DCMIPPRSTC;
    (void)RCC->APB5RSTCR;
}

void RCC_enable_CSI(void){
    RCC->APB5ENR |= RCC_APB5ENR_CSIEN;
    (void)RCC->APB5ENR;

    RCC->APB5ENSR |= RCC_APB5ENSR_CSIENS;
    (void)RCC->APB5ENSR;

    RCC->APB5LPENR |= RCC_APB5LPENR_CSILPEN;
    (void)RCC->APB5LPENR;
}

void RCC_reset_CSI(void){
    RCC->APB5RSTSR |= RCC_APB5RSTSR_CSIRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_CSIRSTC;
    (void)RCC->APB5RSTCR;
}

// -------------------------------------------------------------------------
// PWR / XSPI / RIFSC
// -------------------------------------------------------------------------

void RCC_enable_PWR(void){
    RCC->AHB4ENR |= RCC_AHB4ENR_PWREN;
    (void)RCC->AHB4ENR;
}

void RCC_config_PWR(void){
    RCC_enable_PWR();

    PWR->DBPCR |= PWR_DBPCR_DBP;
    (void)PWR->DBPCR;

    PWR->VOSCR = PWR_VOSCR_ACTVOSRDY
               | PWR_VOSCR_VOSRDY;
    (void)PWR->VOSCR;

    PWR->CPUCR = PWR_CPUCR_SVOS;
    (void)PWR->CPUCR;

    PWR->SVMCR1 |= PWR_SVMCR1_VDDIO4SV;
    (void)PWR->SVMCR1;

    PWR->SVMCR2 |= PWR_SVMCR2_VDDIO5SV;
    (void)PWR->SVMCR2;

    PWR->SVMCR3 = PWR_SVMCR3_VDDIO3VRSEL
                | PWR_SVMCR3_VDDIO2VRSEL
                | PWR_SVMCR3_ARDY
                | PWR_SVMCR3_ASV
                | PWR_SVMCR3_VDDIO3SV
                | PWR_SVMCR3_VDDIO2SV
                | PWR_SVMCR3_AVMEN;
    (void)PWR->SVMCR3;
}

void RCC_enable_XSPI1(void){
    RCC->AHB5ENSR |= RCC_AHB5ENSR_XSPI1ENS;
    (void)RCC->AHB5ENSR;
}

void RCC_reset_XSPI1(void){
    RCC->AHB5RSTSR |= RCC_AHB5RSTSR_XSPI1RSTS;
    (void)RCC->AHB5RSTSR;

    RCC->AHB5RSTCR |= RCC_AHB5RSTCR_XSPI1RSTC;
    (void)RCC->AHB5RSTCR;
}

void RCC_enable_XSPI2(void){
    RCC->AHB5ENSR |= RCC_AHB5ENSR_XSPI2ENS;
    (void)RCC->AHB5ENSR;
}

void RCC_reset_XSPI2(void){
    RCC->AHB5RSTSR |= RCC_AHB5RSTSR_XSPI2RSTS;
    (void)RCC->AHB5RSTSR;

    RCC->AHB5RSTCR |= RCC_AHB5RSTCR_XSPI2RSTC;
    (void)RCC->AHB5RSTCR;
}

void RCC_enable_XSPIM(void){
    RCC->AHB5ENSR |= RCC_AHB5ENSR_XSPIMENS;
    (void)RCC->AHB5ENSR;
}

void RCC_reset_XSPIM(void){
    RCC->AHB5RSTSR |= RCC_AHB5RSTSR_XSPIMRSTS;
    (void)RCC->AHB5RSTSR;

    RCC->AHB5RSTCR |= RCC_AHB5RSTCR_XSPIMRSTC;
    (void)RCC->AHB5RSTCR;
}

void RCC_enable_RIFSC(void){
    RCC->AHB3ENR |= RCC_AHB3ENR_RIFSCEN;
    (void)RCC->AHB3ENR;
}

void RCC_setXSPI1_clock_source(uint32_t source){
    source &= 0x3U;

    RCC->CCIPR6 &= ~RCC_CCIPR6_XSPI1SEL_Msk;
    RCC->CCIPR6 |=  (source << RCC_CCIPR6_XSPI1SEL_Pos);

    (void)RCC->CCIPR6;
}
