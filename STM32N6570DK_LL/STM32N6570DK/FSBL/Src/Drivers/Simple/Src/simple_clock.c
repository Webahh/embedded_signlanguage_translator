#include "simple_clock.h"

static void wait_pll4_ready(void)
{
    while (!(RCC->SR & RCC_SR_PLL4RDY)) {
        /* wait */
    }
}

void DCMIPP_IC17_Clock_Config(void)
{
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

void SystemClock_Config_DCMIPP_IC17(void)
{
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
    wait_pll4_ready();

    DCMIPP_IC17_Clock_Config();
}
