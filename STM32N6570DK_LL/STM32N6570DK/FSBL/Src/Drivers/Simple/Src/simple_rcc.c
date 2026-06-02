/*
 * simple_rcc.c
 *
 *  Created on: 22.05.2026
 *      Author: Weber
 */
#include "simple_rcc.h"
#include "simple_timer.h"

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
    (void)RCC->APB5ENSR;
}

void RCC_reset_LTDC(void){
	RCC->APB5RSTSR |= RCC_APB5RSTSR_LTDCRSTS;
	(void)RCC->APB5RSTSR;

	delay_ms(10);

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
    RCC->APB5ENSR |= RCC_APB5ENSR_DCMIPPENS;
    (void)RCC->APB5ENSR;
}

void RCC_reset_DCMIPP(void){
    RCC->APB5RSTSR |= RCC_APB5RSTSR_DCMIPPRSTS;
    (void)RCC->APB5RSTSR;
    RCC->APB5RSTCR |= RCC_APB5RSTCR_DCMIPPRSTC;
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
    delay_ms(10);
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
    delay_ms(10);
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
    delay_ms(10);
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

