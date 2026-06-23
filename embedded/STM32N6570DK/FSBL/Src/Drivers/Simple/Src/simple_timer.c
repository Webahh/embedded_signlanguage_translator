/**
 * @file    simple_timer.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   TIM7 system timer driver source
 */

#include <stdint.h>
#include <stddef.h>

#include "simple_timer.h"
#include "simple_rcc.h"

/*
 * Reference clock setup:
 * HCLK  = 200 MHz
 * PCLK1 = 200 MHz
 * PCLK2 = 200 MHz
 *
 * TIM2..TIM7  -> APB1 -> 200 MHz
 * TIM1/TIM8   -> APB2 -> 200 MHz
 *
 * This avoids using RCC_Clock_TIM_get(), because the current RCC getter
 * does not yet decode the IC-based clock tree.
 */
#define _TIMER_INPUT_CLK_HZ 200000000UL
#define _TIMER_1MHZ         1000000UL

// -------------------------------------------------------------------------
// Private
// -------------------------------------------------------------------------

/**
 * @brief Enable timer peripheral clock and optionally its NVIC interrupt
 *
 * @param [in] TIMX    Timer instance
 * @param [in] use_irq Enable NVIC interrupt if non-zero
 */
static void _TIMER_Enable_Clock_and_NVIC(TIM_TypeDef* TIMX, int use_irq){
	RCC_enable_TIM(TIMX);

	if (!use_irq) {
		return;
	}

	if (TIMX == TIM1) {
		NVIC_EnableIRQ(TIM1_UP_IRQn);
	} else if (TIMX == TIM2) {
		NVIC_EnableIRQ(TIM2_IRQn);
	} else if (TIMX == TIM3) {
		NVIC_EnableIRQ(TIM3_IRQn);
	} else if (TIMX == TIM4) {
		NVIC_EnableIRQ(TIM4_IRQn);
	} else if (TIMX == TIM5) {
		NVIC_EnableIRQ(TIM5_IRQn);
	} else if (TIMX == TIM6) {
		NVIC_EnableIRQ(TIM6_IRQn);
	} else if (TIMX == TIM7) {
		NVIC_EnableIRQ(TIM7_IRQn);
	} else if (TIMX == TIM8) {
		NVIC_EnableIRQ(TIM8_UP_IRQn);
	}
}

/**
 * @brief Configure a timer to generate a 1 kHz update event
 *
 * With reference clock:
 * TIM clock = 200 MHz
 * PSC       = 200 - 1
 * Counter   = 1 MHz
 * ARR       = 999
 *
 * 1 MHz / 1000 = 1 kHz -> 1 ms update event
 *
 * @param [in] TIMX    Timer instance
 * @param [in] use_irq Enable update interrupt if non-zero
 */
static void _TIMER_Config_1kHz(TIM_TypeDef* TIMX, int use_irq){
	uint32_t prescaler = _TIMER_INPUT_CLK_HZ / _TIMER_1MHZ;

	if (prescaler == 0U) {
		prescaler = 1U;
	}

	TIMER_Config(TIMX, (int)prescaler, 999, use_irq);
}

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * @brief Configure basic timer settings
 *
 * @param [in] TIMX        Timer instance
 * @param [in] preScaleVal Prescaler value
 * @param [in] limit       Auto-reload value
 * @param [in] use_irq     Enable update interrupt if non-zero
 */
void TIMER_Config(TIM_TypeDef* TIMX, int preScaleVal, int limit, int use_irq){
	if (preScaleVal <= 0) {
		return;
	}

	_TIMER_Enable_Clock_and_NVIC(TIMX, use_irq);

	TIMX->CR1 = 0;
	TIMX->PSC = preScaleVal - 1;
	TIMX->ARR = limit;
	TIMX->CNT = 0;

	if (use_irq) {
		TIMX->DIER |= TIM_DIER_UIE;
	} else {
		TIMX->DIER &= ~TIM_DIER_UIE;
	}

	TIMX->EGR = TIM_EGR_UG;
	TIMX->SR &= ~TIM_SR_UIF;
}

void TIMER_Start(TIM_TypeDef* TIMX){
	TIMX->CR1 |= TIM_CR1_CEN;
}

void TIMER_Stop(TIM_TypeDef* TIMX){
	TIMX->CR1 &= ~TIM_CR1_CEN;
}

void TIMER_ResetCounter(TIM_TypeDef* TIMX){
	TIMX->CNT = 0;
}

TIMER_Status_TypeDef TIMER_GetCounter(TIM_TypeDef* TIMX, int* counter){
	if (counter == NULL) {
		return TIMER_ERROR;
	}

	*counter = (int)TIMX->CNT;
	return TIMER_OK;
}

TIMER_Status_TypeDef TIMER_GetFlag(TIM_TypeDef* TIMX, uint32_t flag, uint32_t* result){
	if (result == NULL) {
		return TIMER_ERROR;
	}

	*result = TIMX->SR & flag;
	return TIMER_OK;
}

void TIMER_ClearFlag(TIM_TypeDef* TIMX, uint32_t flag){
	TIMX->SR &= ~flag;
}

void TIMER_EnableIT(TIM_TypeDef* TIMX){
	TIMX->DIER |= TIM_DIER_UIE;
}

void TIMER_DisableIT(TIM_TypeDef* TIMX){
	TIMX->DIER &= ~TIM_DIER_UIE;
}

/**
 * @brief Initialise TIM6 as millisecond delay timer
 */
void TIMER_Delay_init(void){
	_TIMER_Config_1kHz(TIM6, 0);
}

/**
 * @brief Blocking millisecond delay using TIM6
 *
 * @param [in] ms Delay time in milliseconds
 */
void TIMER_Delay_ms(int ms){
	uint32_t flag;

	if (ms <= 0) {
		return;
	}

	TIMER_ClearFlag(TIM6, TIM_SR_UIF);
	TIMER_ResetCounter(TIM6);
	TIMER_Start(TIM6);

	for (int i = 0; i < ms; i++) {
		do {
			TIMER_GetFlag(TIM6, TIM_SR_UIF, &flag);
		} while (flag == 0U);
		TIMER_ClearFlag(TIM6, TIM_SR_UIF);
	}

	TIMER_Stop(TIM6);
}
