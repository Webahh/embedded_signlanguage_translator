/*
 * simple_gpio.c
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#include "simple_gpio.h"
#include "simple_rcc.h"

/* ------------- Simple_GPIO Functions ------------- */

static void GPIO_setAF(GPIO_TypeDef* GPIOX, int pinNr, int af){
    GPIOX->OSPEEDR &=
        ~(3U << (2U * pinNr));

    GPIOX->OSPEEDR |=
        (3U << (2U * pinNr));

    if (pinNr < 8) {
        GPIOX->AFR[0] &= ~(0xFU << (4U * pinNr));
        GPIOX->AFR[0] |=  ((uint32_t)af << (4U * pinNr));
    }
    else {
        GPIOX->AFR[1] &= ~(0xFU << (4U * (pinNr - 8)));
        GPIOX->AFR[1] |=  ((uint32_t)af << (4U * (pinNr - 8)));
    }
}

/**
 * @brief Configure a GPIO pin.
 *
 * Enables the GPIO port clock and configures mode, output type
 * and pull-up/pull-down setting for one pin.
 *
 * @param GPIOX GPIO port instance, e.g. GPIOA, GPIOB, ...
 * @param pinNr Pin number from 0 to 15.
 * @param mode  GPIO mode value for MODER register.
 * @param otyp  Output type value for OTYPER register.
 * @param pupdr Pull-up/pull-down value for PUPDR register.
 * @param af	Alternate Function identifier (I2C, SPI, ...)
 */
void GPIO_Config(GPIO_TypeDef* GPIOX, int pinNr, int mode, int otyp, int pupdr, int af){
	RCC_enable_GPIO(GPIOX);
	GPIOX->MODER  = (GPIOX->MODER  & ~(3U << (2 * pinNr))) | (mode  << (2 * pinNr));
	GPIOX->OTYPER = (GPIOX->OTYPER & ~(1U << (pinNr)))     | (otyp  << (pinNr));
	GPIOX->PUPDR  = (GPIOX->PUPDR  & ~(3U << (2 * pinNr))) | (pupdr << (2 * pinNr));

	if(mode == GPIO_MODE_AF){
		GPIO_setAF(GPIOX, pinNr, af);
	}
}

int GPIO_get(GPIO_TypeDef* GPIOX, int pinNr){
	return (int)((GPIOX->IDR >> pinNr) & 1U);
}

void GPIO_BSRR_set(GPIO_TypeDef* GPIOX, int pinNr){
	GPIOX->BSRR = (1U<<pinNr);
}

void GPIO_BSRR_reset(GPIO_TypeDef* GPIOX, int pinNr){
	GPIOX->BSRR = (1U<<(pinNr+16));
}
