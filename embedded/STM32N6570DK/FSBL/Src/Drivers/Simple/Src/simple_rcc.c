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
#include "simple_rcc.h"

static void RCC_WaitHSIReady(void){
    while (!(RCC->SR & RCC_SR_HSIRDY)) {
        /* wait */
    }
}

static uint32_t RCC_GetAHBPrescalerDiv(uint32_t hpre_bits){
    /*
     * STM32N6 RCC_CFGR2_HPRE is 3 bits wide:
     *
     * 0xx: /1
     * 100: /2
     * 101: /4
     * 110: /8
     * 111: /16
     */

    static const uint8_t ahb_div_table[8] = {
        1, 1, 1, 1,
        2, 4, 8, 16
    };

    return ahb_div_table[hpre_bits & 0x7U];
}

static uint32_t RCC_GetAPBPrescalerDiv(uint32_t ppre_bits){
    /*
     * Standard STM32 APB prescaler encoding:
     *
     * 0xx: /1
     * 100: /2
     * 101: /4
     * 110: /8
     * 111: /16
     */

    static const uint8_t apb_div_table[8] = {
        1, 1, 1, 1,
        2, 4, 8, 16
    };

    return apb_div_table[ppre_bits & 0x7U];
}

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

    while ((RCC->CFGR1 & RCC_CFGR1_SYSSWS) != 0U) {
        /* wait */
    }

    while ((RCC->CFGR1 & RCC_CFGR1_CPUSWS) != 0U) {
        /* wait */
    }
}

uint32_t RCC_GetHSI(void){
    return RCC_HSI_VALUE_HZ;
}

uint32_t RCC_GetSYSCLK(void){
    uint32_t syssws =
        (RCC->CFGR1 & RCC_CFGR1_SYSSWS) >> RCC_CFGR1_SYSSWS_Pos;

    switch (syssws) {
        case 0x0U:
            /* SYSCLK = HSI */
            return RCC_GetHSI();

        default:
        	/*
        	 * for now others are not needed!
        	 */

            return 0U;
    }
}

uint32_t RCC_GetCPUCLK(void){
    uint32_t cpusws =
        (RCC->CFGR1 & RCC_CFGR1_CPUSWS) >> RCC_CFGR1_CPUSWS_Pos;

    switch (cpusws) {
        case 0x0U:
            /* CPUCLK = HSI */
            return RCC_GetHSI();

        default:
        	/*
        	 * for now others are not needed!
        	 */
            return 0U;
    }
}

uint32_t RCC_GetHCLK(void){
    uint32_t sysclk = RCC_GetSYSCLK();

    uint32_t hpre_bits =
        (RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos;

    uint32_t hpre_div = RCC_GetAHBPrescalerDiv(hpre_bits);

    if (hpre_div == 0U) {
        return 0U;
    }

    return sysclk / hpre_div;
}

uint32_t RCC_GetPCLK1(void){
    uint32_t hclk = RCC_GetHCLK();

    uint32_t ppre1_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE1) >> RCC_CFGR2_PPRE1_Pos;

    uint32_t ppre1_div = RCC_GetAPBPrescalerDiv(ppre1_bits);

    if (ppre1_div == 0U) {
        return 0U;
    }

    return hclk / ppre1_div;
}

uint32_t RCC_GetPCLK2(void){
    uint32_t hclk = RCC_GetHCLK();

    uint32_t ppre2_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE2) >> RCC_CFGR2_PPRE2_Pos;

    uint32_t ppre2_div = RCC_GetAPBPrescalerDiv(ppre2_bits);

    if (ppre2_div == 0U) {
        return 0U;
    }

    return hclk / ppre2_div;
}

uint32_t RCC_GetPCLK4(void){
    uint32_t hclk = RCC_GetHCLK();

    uint32_t ppre4_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE4) >> RCC_CFGR2_PPRE4_Pos;

    uint32_t ppre4_div = RCC_GetAPBPrescalerDiv(ppre4_bits);

    if (ppre4_div == 0U) {
        return 0U;
    }

    return hclk / ppre4_div;
}

uint32_t RCC_GetPCLK5(void){
    uint32_t hclk = RCC_GetHCLK();

    uint32_t ppre5_bits =
        (RCC->CFGR2 & RCC_CFGR2_PPRE5) >> RCC_CFGR2_PPRE5_Pos;

    uint32_t ppre5_div = RCC_GetAPBPrescalerDiv(ppre5_bits);

    if (ppre5_div == 0U) {
        return 0U;
    }

    return hclk / ppre5_div;
}

/**
 * @brief Return the timer input clock for the selected timer.
 *
 * TIM2..TIM7 are connected to APB1.
 * TIM1, TIM8 and TIM15..TIM17 are connected to APB2.
 *
 * @note This implementation currently returns PCLK directly.
 *       If APB prescalers are changed later this function
 *       needs a rework!
 *
 * @param TIMX Timer instance.
 * @return Timer input clock in Hz, or 0 if the timer is unsupported.
 */
uint32_t RCC_GetTIMClock(TIM_TypeDef *TIMX){
    /*
     * TIM on APB1 -> PCLK1
     * TIM on APB2 -> PCLK2
     */

    if ((TIMX == TIM2) ||
        (TIMX == TIM3) ||
        (TIMX == TIM4) ||
        (TIMX == TIM5) ||
        (TIMX == TIM6) ||
        (TIMX == TIM7)) {
        return RCC_GetPCLK1();
    }

    if ((TIMX == TIM1) ||
        (TIMX == TIM8) ||
        (TIMX == TIM15) ||
        (TIMX == TIM16) ||
        (TIMX == TIM17)) {
        return RCC_GetPCLK2();
    }

    return 0U;
}

uint32_t RCC_GetI2CClock(I2C_TypeDef *I2CX){
    if ((I2CX == I2C1) || (I2CX == I2C2) || (I2CX == I2C3)) {
        return RCC_GetPCLK1();
    }

    if (I2CX == I2C4) {
        return RCC_GetPCLK4();
    }

    return 0U;
}

void RCC_enable_GPIO(GPIO_TypeDef* GPIOX){
	if (GPIOX == GPIOA){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
        /*
         * Dummy read after enabling the clock.
         * This gives the bus clock enable write time to take effect
         * before the peripheral registers are accessed.
         */
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOB){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOC){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOD){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOE){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOF){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOFEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOG){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOGEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOH){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOHEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPION){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIONEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOO){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOOEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOP){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOPEN;
		(void)RCC->AHB4ENR;
	}
	else if (GPIOX == GPIOQ){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOQEN;
		(void)RCC->AHB4ENR;
	}
}

void RCC_enable_I2C(I2C_TypeDef* I2CX){
    if (I2CX == I2C1) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
        (void)RCC->APB1ENR1;
    }
    else if (I2CX == I2C2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
        (void)RCC->APB1ENR1;
    }
    else if (I2CX == I2C3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C3EN;
        (void)RCC->APB1ENR1;
    }
    else if (I2CX == I2C4) {
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
    }
    else if (I2CX == I2C2) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C2RST;
        (void)RCC->APB1RSTR1;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C2RST;
        (void)RCC->APB1RSTR1;
    }
    else if (I2CX == I2C3) {
        RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C3RST;
        (void)RCC->APB1RSTR1;
        RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C3RST;
        (void)RCC->APB1RSTR1;
    }
    else if (I2CX == I2C4) {
        RCC->APB4RSTR1 |= RCC_APB4RSTR1_I2C4RST;
        (void)RCC->APB4RSTR1;
        RCC->APB4RSTR1 &= ~RCC_APB4RSTR1_I2C4RST;
        (void)RCC->APB4RSTR1;
    }
}

void RCC_setI2C_clock_source(I2C_TypeDef* I2CX, uint32_t source){
    source &= 0x7U;   // I2CSEL field are 3 bits wide

    if (I2CX == I2C1) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C1SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C1SEL_Pos);
    }
    else if (I2CX == I2C2) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C2SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C2SEL_Pos);
    }
    else if (I2CX == I2C3) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C3SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C3SEL_Pos);
    }
    else if (I2CX == I2C4) {
        RCC->CCIPR4 &= ~RCC_CCIPR4_I2C4SEL;
        RCC->CCIPR4 |=  (source << RCC_CCIPR4_I2C4SEL_Pos);
    }

    (void)RCC->CCIPR4;
}

void RCC_enable_TIM(TIM_TypeDef *TIMX){
    if (TIMX == TIM1) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
        (void)RCC->APB2ENR;
    }
    else if (TIMX == TIM2) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM3) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM4) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM5) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM6) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM7) {
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;
        (void)RCC->APB1ENR1;
    }
    else if (TIMX == TIM8) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
        (void)RCC->APB2ENR;
    }
    else if (TIMX == TIM15) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
        (void)RCC->APB2ENR;
    }
    else if (TIMX == TIM16) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;
        (void)RCC->APB2ENR;
    }
    else if (TIMX == TIM17) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
        (void)RCC->APB2ENR;
    }
}

/**
 * @brief Enable AXISRAM blocks used by the LTDC framebuffer.
 *
 * The framebuffer is stored in AXI SRAM. These memory blocks must be
 * clock-enabled before the LTDC or CPU can reliably access the framebuffer.
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
    source &= 0x3U;   // 2 bits width

    RCC->CCIPR4 &= ~RCC_CCIPR4_LTDCSEL;
    RCC->CCIPR4 |=  (source << RCC_CCIPR4_LTDCSEL_Pos);

    (void)RCC->CCIPR4;
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

void RCC_config_PLL1_800MHz(void)
{
    if (!(RCC->SR & RCC_SR_PLL1RDY)) {
        RCC->PLL1CFGR1 &= ~RCC_PLL1CFGR1_PLL1SEL;      // source = HSI (default)
        RCC->PLL1CFGR1 = (RCC->PLL1CFGR1 & ~(RCC_PLL1CFGR1_PLL1BYP | RCC_PLL1CFGR1_PLL1DIVM | RCC_PLL1CFGR1_PLL1DIVN))
                        | (4UL  << RCC_PLL1CFGR1_PLL1DIVM_Pos)
                        | (75UL << RCC_PLL1CFGR1_PLL1DIVN_Pos);
        RCC->PLL1CFGR2 &= ~RCC_PLL1CFGR2_PLL1DIVNFRAC;

        RCC->PLL1CFGR3 = (RCC->PLL1CFGR3 & ~(RCC_PLL1CFGR3_PLL1PDIV1 | RCC_PLL1CFGR3_PLL1PDIV2))
                        | (1UL << RCC_PLL1CFGR3_PLL1PDIV1_Pos)
                        | (1UL << RCC_PLL1CFGR3_PLL1PDIV2_Pos);
        RCC->PLL1CFGR3 |= RCC_PLL1CFGR3_PLL1PDIVEN;
        (void)RCC->PLL1CFGR3;

        /* Enable PLL1 and wait for lock */
        RCC->CR |= RCC_CR_PLL1ON;
        while (!(RCC->SR & RCC_SR_PLL1RDY)) { }
    }
}

void RCC_config_CSI_clock_IC18(void)
{
    /* Configure IC18: source = PLL1, divider = 60 -> 1200/60 = 20 MHz */
    RCC->IC18CFGR = (RCC->IC18CFGR & ~RCC_IC18CFGR_IC18SEL_Msk)
                   | ((60UL - 1UL) << RCC_IC18CFGR_IC18INT_Pos);
    (void)RCC->IC18CFGR;

    RCC->DIVENR |= RCC_DIVENR_IC18EN;
    (void)RCC->DIVENR;
}

void RCC_reset_CSI(void){
    RCC->APB5RSTSR |= RCC_APB5RSTSR_CSIRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_CSIRSTC;
    (void)RCC->APB5RSTCR;
}

void RCC_enable_PWR(void){
    RCC->AHB4ENR |= RCC_AHB4ENR_PWREN;
    (void)RCC->AHB4ENR;
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

/**
 * @brief Enable VDDIO2 supply supervision.
 *
 * VDDIO2 is required for GPIOs/peripherals powered by the second I/O domain.
 */
void RCC_enable_VDDIO2(void){
    RCC->AHB4ENSR |= RCC_AHB4ENSR_PWRENS;
    (void)RCC->AHB4ENSR;
    PWR->SVMCR3 |= PWR_SVMCR3_VDDIO2SV;
    (void)PWR->SVMCR3;
}

/**
 * @brief Configure VDDIO2 voltage range to 1.8 V.
 *
 * Clears VDDIO2VRSEL to select the 1.8 V range.
 */
void RCC_config_VDDIO2_1V8(void){
    PWR->SVMCR3 &= ~PWR_SVMCR3_VDDIO2VRSEL;
    (void)PWR->SVMCR3;
}

/**
 * @brief Configure LTDC pixel clock to 25 MHz.
 *
 * PLL4 is configured from HSI and routed through IC16 to the LTDC kernel clock.
 *
 * Clock calculation:
 * - PLL4 source = HSI = 64 MHz
 * - M = 8
 * - N = 225
 * - P1 = 6
 * - P2 = 6
 *
 * VCO      = (64 MHz / 8) * 225 = 1800 MHz
 * PLL4 out = 1800 MHz / (6 * 6) = 50 MHz
 * IC16     = PLL4 out / 2 = 25 MHz
 *
 * The resulting 25 MHz clock is used as LTDC pixel clock.
 *
 * @note This configuration assumes HSI is already enabled and stable.
 */
void RCC_config_LTDC_25MHz_clock(void){
    /*
     * Configure PLL4 only if it is not already running.
     * PLL parameters should not be changed while the PLL is enabled.
     */
    if (!(RCC->SR & RCC_SR_PLL4RDY)) {
        RCC->PLL4CFGR1 &= ~RCC_PLL4CFGR1_PLL4SEL;
        RCC->PLL4CFGR1 = (RCC->PLL4CFGR1 & ~(RCC_PLL4CFGR1_PLL4DIVM | RCC_PLL4CFGR1_PLL4DIVN))
                        | (8UL << RCC_PLL4CFGR1_PLL4DIVM_Pos)
                        | (225UL << RCC_PLL4CFGR1_PLL4DIVN_Pos);

        RCC->PLL4CFGR2 &= ~RCC_PLL4CFGR2_PLL4DIVNFRAC;

        RCC->PLL4CFGR3 = (RCC->PLL4CFGR3 & ~(RCC_PLL4CFGR3_PLL4PDIV1 | RCC_PLL4CFGR3_PLL4PDIV2))
                        | (6UL << RCC_PLL4CFGR3_PLL4PDIV1_Pos)
                        | (6UL << RCC_PLL4CFGR3_PLL4PDIV2_Pos);

        RCC->PLL4CFGR3 |= RCC_PLL4CFGR3_PLL4PDIVEN;
        (void)RCC->PLL4CFGR3;

        RCC->CR |= RCC_CR_PLL4ON;
        while (!(RCC->SR & RCC_SR_PLL4RDY)) {
            /* wait */
        }
    }

    /*
     * Configure IC16 as an intermediate divider for the LTDC clock.
     * IC16 source is selected from PLL4 and divided by 2.
     */
    uint32_t ic16sel = RCC_IC16CFGR_IC16SEL_0 | RCC_IC16CFGR_IC16SEL_1;
    uint32_t ic16int = (2UL - 1UL) << RCC_IC16CFGR_IC16INT_Pos;

    RCC->IC16CFGR = (RCC->IC16CFGR & ~(RCC_IC16CFGR_IC16SEL | RCC_IC16CFGR_IC16INT))
                   | ic16sel | ic16int;
    (void)RCC->IC16CFGR;

    RCC->DIVENR |= RCC_DIVENR_IC16EN;
    (void)RCC->DIVENR;

    /*
     * Select IC16 output as LTDC kernel clock source.
     */
    RCC->CCIPR4 = (RCC->CCIPR4 & ~RCC_CCIPR4_LTDCSEL) | RCC_CCIPR4_LTDCSEL_1;
    (void)RCC->CCIPR4;
}
