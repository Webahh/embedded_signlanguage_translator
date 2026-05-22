/*
 * simple_gpio.c
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#include "simple_gpio.h"

/* ------------- Config Helper ------------- */

static void GPIO_enableClock(GPIO_TypeDef* GPIOX){
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
}

/* ------------- Simple_GPIO Functions ------------- */

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
 */
void GPIO_Config(GPIO_TypeDef* GPIOX, int pinNr, int mode, int otyp, int pupdr){
	GPIO_enableClock(GPIOX);
	GPIOX->MODER  = (GPIOX->MODER  & ~(3U << (2 * pinNr))) | (mode  << (2 * pinNr));
	GPIOX->OTYPER = (GPIOX->OTYPER & ~(1U << (pinNr)))     | (otyp  << (pinNr));
	GPIOX->PUPDR  = (GPIOX->PUPDR  & ~(3U << (2 * pinNr))) | (pupdr << (2 * pinNr));
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
