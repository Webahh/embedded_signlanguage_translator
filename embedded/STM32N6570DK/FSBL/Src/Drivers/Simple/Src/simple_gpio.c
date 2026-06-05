/*
 * simple_gpio.c
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#include "simple_gpio.h"
#include "simple_rcc.h"

/* ------------- Simple_GPIO Helper ------------- */

/**
 * @brief Configure alternate function settings for one GPIO pin.
 *
 * This helper sets the output speed and selkects the alternate function
 * number in the correct AFR register.
 *
 * AFR[0] is used for pins 0..7.
 * AFR[1] is used for pins 8..15.
 */
static void GPIO_setAF(GPIO_TypeDef* GPIOX, uint32_t pinNr, uint32_t af, uint32_t speed){
    GPIOX->OSPEEDR =
        (GPIOX->OSPEEDR & ~(3U << (2U * pinNr))) |
        (((uint32_t)speed & 3U) << (2U * pinNr));

    if (pinNr < 8) {
        GPIOX->AFR[0] &= ~(0xFU << (4U * pinNr));
        GPIOX->AFR[0] |=  ((uint32_t)af << (4U * pinNr));
    }
    else {
        GPIOX->AFR[1] &= ~(0xFU << (4U * (pinNr - 8U)));
        GPIOX->AFR[1] |=  ((uint32_t)af << (4U * (pinNr - 8U)));
    }
}

/* ------------- Simple_GPIO Functions ------------- */

/**
 * @brief Configure a GPIO pin.
 *
 * Enables the GPIO port clock and configures mode, output type
 * and pull-up/pull-down setting for one pin.
 *
 * If the pin is configured as alternate function, the AF number and
 * GPIO speed are configured as well.
 *
 * @param GPIOX GPIO port instance, e.g. GPIOA, GPIOB, ...
 * @param pinNr Pin number from 0 to 15.
 * @param cfg	GPIO cfg for GPIO mode, output type, pull-up/pull-down,
 * 				alternate function identifier and speed value.
 */
void GPIO_Config(GPIO_TypeDef* GPIOX, uint32_t pinNr, GPIO_cfg_TypeDef cfg){
	RCC_enable_GPIO(GPIOX);
	GPIOX->MODER  = (GPIOX->MODER  & ~(3U << (2U * pinNr))) | (cfg.mode  << (2U * pinNr));
	GPIOX->OTYPER = (GPIOX->OTYPER & ~(1U << (pinNr)))     | (cfg.otyp  << (pinNr));
	GPIOX->PUPDR  = (GPIOX->PUPDR  & ~(3U << (2U * pinNr))) | (cfg.pupdr << (2U * pinNr));

	if(cfg.mode == GPIO_MODE_AF){
		GPIO_setAF(GPIOX, pinNr, cfg.af, cfg.speed);
	}
}

uint32_t GPIO_get(GPIO_TypeDef* GPIOX, uint32_t pinNr){
	return (uint32_t)((GPIOX->IDR >> pinNr) & 1U);
}

void GPIO_BSRR_toggle(GPIO_TypeDef* GPIOX, uint32_t pinNr) {
    if (GPIOX->ODR & (1U << pinNr))
        GPIOX->BSRR = (1U << (pinNr + 16U));
    else
        GPIOX->BSRR = (1U << pinNr);
}

void GPIO_BSRR_set(GPIO_TypeDef* GPIOX, uint32_t pinNr){
	GPIOX->BSRR = (1U<<pinNr);
}

void GPIO_BSRR_reset(GPIO_TypeDef* GPIOX, uint32_t pinNr){
	GPIOX->BSRR = (1U<<(pinNr+16U));
}
