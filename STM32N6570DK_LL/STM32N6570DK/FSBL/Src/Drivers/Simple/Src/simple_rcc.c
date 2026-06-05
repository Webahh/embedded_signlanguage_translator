/*
 * simple_rcc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */
#include "simple_rcc.h"


static void RCC_WaitHSIReady(void){
    while (!(RCC->SR & RCC_SR_HSIRDY)) {
        /* wait */
    }
}

static uint32_t RCC_GetAHBPrescalerDiv(uint32_t hpre_bits){
    /*
     * Standard STM32 AHB prescaler encoding:
     *
     * 0xxx: /1
     * 1000: /2
     * 1001: /4
     * 1010: /8
     * 1011: /16
     * 1100: /64
     * 1101: /128
     * 1110: /256
     * 1111: /512
     */

    static const uint16_t ahb_div_table[16] = {
        1, 1, 1, 1,
        1, 1, 1, 1,
        2, 4, 8, 16,
        64, 128, 256, 512
    };

    return ahb_div_table[hpre_bits & 0xFU];
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

void RCC_SystemClock_Config(void){
    /*
     * Basic system clock configuration:
     *
     * HSI    = 64 MHz
     * SYSCLK = HSI
     * HCLK   = SYSCLK / 1
     * PCLK1  = HCLK / 1
     * PCLK2  = HCLK / 1
     */

    /* Enable HSI */
    RCC->CR |= RCC_CR_HSION;
    RCC_WaitHSIReady();

    /* Set prescalers to /1 */
    RCC->CFGR2 &= ~(RCC_CFGR2_HPRE |
                   RCC_CFGR2_PPRE1 |
                   RCC_CFGR2_PPRE2);

    /* Set HSI as SYSCLK and CPUCLK */

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
             * Später ergänzen:
             * case 0x1U: ...
             * case 0x2U: ...
             * case 0x3U: ...
             *
             * Für jetzt bewusst 0, damit falsche auffallen!
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
             * Später ergänzen, sobald PLL/HSE/MSI/CSI genutzt werden.
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

void RCC_enable_GPIO(GPIO_TypeDef* GPIOX){
	if (GPIOX == GPIOA){
		RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
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
    source &= 0x7U;   // 3 bits width

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

    RCC->CCIPR1 = (RCC->CCIPR1 & ~RCC_CCIPR1_DCMIPPSEL_Msk)
                | (0x2U << RCC_CCIPR1_DCMIPPSEL_Pos);
    (void)RCC->CCIPR1;

    RCC->DIVENR |= RCC_DIVENR_IC17EN;
    (void)RCC->DIVENR;

    RCC->APB5ENSR = RCC_APB5ENSR_DCMIPPENS;
    (void)RCC->APB5ENSR;
}

void RCC_reset_DCMIPP(void){
    RCC->APB5RSTSR = RCC_APB5RSTSR_DCMIPPRSTS;
    (void)RCC->APB5RSTSR;

    RCC->APB5RSTCR = RCC_APB5RSTCR_DCMIPPRSTC;
    (void)RCC->APB5RSTCR;
}

void RCC_enable_CSI(void){
    RCC->APB5ENSR |= RCC_APB5ENSR_CSIENS;
    (void)RCC->APB5ENSR;
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

void RCC_enable_VDDIO2(void){
    RCC->AHB4ENSR |= RCC_AHB4ENSR_PWRENS;
    (void)RCC->AHB4ENSR;
    PWR->SVMCR3 |= PWR_SVMCR3_VDDIO2SV;
    (void)PWR->SVMCR3;
}

void RCC_config_VDDIO2_1V8(void){
    PWR->SVMCR3 &= ~PWR_SVMCR3_VDDIO2VRSEL;
    (void)PWR->SVMCR3;
}

void RCC_config_LTDC_clock(void){
    /* PLL4 configuration for 25 MHz pixel clock
     * PLL4 source = HSI (64 MHz), M=8, N=225, P1=6, P2=6
     * VCO = (64/8) * 225 = 1800 MHz
     * PLL4_out = 1800 / (6*6) = 50 MHz
     * IC16 divider = 2  ->  LTDC clock = 25 MHz
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
        while (!(RCC->SR & RCC_SR_PLL4RDY));
    }

    uint32_t ic16sel = RCC_IC16CFGR_IC16SEL_0 | RCC_IC16CFGR_IC16SEL_1;
    uint32_t ic16int = (2UL - 1UL) << RCC_IC16CFGR_IC16INT_Pos;

    RCC->IC16CFGR = (RCC->IC16CFGR & ~(RCC_IC16CFGR_IC16SEL | RCC_IC16CFGR_IC16INT))
                   | ic16sel | ic16int;
    (void)RCC->IC16CFGR;

    RCC->DIVENR |= RCC_DIVENR_IC16EN;
    (void)RCC->DIVENR;

    RCC->CCIPR4 = (RCC->CCIPR4 & ~RCC_CCIPR4_LTDCSEL) | RCC_CCIPR4_LTDCSEL_1;
    (void)RCC->CCIPR4;
}

void DCMIPP_IC17_Clock_Config(void){
    /*
     * PLL4 already configured to 200 MHz.
     * IC17 source = PLL4
     * IC17 divider = 1
     * DCMIPPSEL = 0b10 = ic17_ck
     */

    RCC->IC17CFGR =
        (RCC->IC17CFGR & ~(RCC_IC17CFGR_IC17SEL |
                           RCC_IC17CFGR_IC17INT))
        | (RCC_IC17CFGR_IC17SEL_0 | RCC_IC17CFGR_IC17SEL_1)
        | ((1UL - 1UL) << RCC_IC17CFGR_IC17INT_Pos);

    (void)RCC->IC17CFGR;

    RCC->DIVENR |= RCC_DIVENR_IC17EN;
    (void)RCC->DIVENR;

    RCC->CCIPR1 =
        (RCC->CCIPR1 & ~RCC_CCIPR1_DCMIPPSEL)
        | (0x2UL << RCC_CCIPR1_DCMIPPSEL_Pos);

    (void)RCC->CCIPR1;
}

void SystemClock_Config_DCMIPP_IC17(void){
    /*
     * Configure PLL4 only:
     *
     * HSI = 64 MHz
     * PLL4 = 64 / 8 * 200 / (4 * 2)
     *      = 200 MHz
     *
     * ic17_ck = PLL4 / 1 = 200 MHz
     */

    if (RCC->SR & RCC_SR_PLL4RDY) {
        RCC->CR &= ~RCC_CR_PLL4ON;
        while (RCC->SR & RCC_SR_PLL4RDY) {
            /* wait until PLL4 disabled */
        }
    }

    /* PLL4 source = HSI */
    RCC->PLL4CFGR1 &= ~RCC_PLL4CFGR1_PLL4SEL;

    RCC->PLL4CFGR1 =
        (RCC->PLL4CFGR1 & ~(RCC_PLL4CFGR1_PLL4DIVM |
                            RCC_PLL4CFGR1_PLL4DIVN))
        | (8UL   << RCC_PLL4CFGR1_PLL4DIVM_Pos)
        | (200UL << RCC_PLL4CFGR1_PLL4DIVN_Pos);

    RCC->PLL4CFGR2 &= ~RCC_PLL4CFGR2_PLL4DIVNFRAC;

    RCC->PLL4CFGR3 =
        (RCC->PLL4CFGR3 & ~(RCC_PLL4CFGR3_PLL4PDIV1 |
                            RCC_PLL4CFGR3_PLL4PDIV2))
        | (4UL << RCC_PLL4CFGR3_PLL4PDIV1_Pos)
        | (2UL << RCC_PLL4CFGR3_PLL4PDIV2_Pos);

    RCC->PLL4CFGR3 |= RCC_PLL4CFGR3_PLL4PDIVEN;
    (void)RCC->PLL4CFGR3;

    RCC->CR |= RCC_CR_PLL4ON;
    RCC_WaitHSIReady();

    DCMIPP_IC17_Clock_Config();
}
