/*
 * simple_timer.c
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#include "simple_timer.h"
#include "simple_rcc.h"


static volatile uint32_t sys_tick_ms = 0; // global millisecond counter

/*
 * Vector table symbol provided by the startup file.
 * Used to explicitly set SCB->VTOR to the application vector table.
 */
extern uint32_t g_pfnVectors[];

/* ------------- Simple_Timer Helper ------------- */


/**
 * @brief Enable timer peripheral clock and optionally its NVIC interrupt.
 *
 * The peripheral clock must be enabled before accessing timer registers.
 * If interrupts are requested, the matching IRQ line is enabled in the NVIC.
 */
static void TIM_Enable_Clock_and_NVIC(TIM_TypeDef* TIMX, int use_irq){
    RCC_enable_TIM(TIMX);

    if (!use_irq) {
        return;
    }

    if (TIMX == TIM1) {
        NVIC_EnableIRQ(TIM1_UP_IRQn);
    }
    else if (TIMX == TIM2) {
        NVIC_EnableIRQ(TIM2_IRQn);
    }
    else if (TIMX == TIM3) {
        NVIC_EnableIRQ(TIM3_IRQn);
    }
    else if (TIMX == TIM4) {
        NVIC_EnableIRQ(TIM4_IRQn);
    }
    else if (TIMX == TIM5) {
        NVIC_EnableIRQ(TIM5_IRQn);
    }
    else if (TIMX == TIM6) {
        NVIC_EnableIRQ(TIM6_IRQn);
    }
    else if (TIMX == TIM7) {
        NVIC_EnableIRQ(TIM7_IRQn);
    }
    else if (TIMX == TIM8) {
        NVIC_EnableIRQ(TIM8_UP_IRQn);
    }
}

/**
 * @brief Configure a timer to generate a 1 kHz update event.
 *
 * The timer input clock is reduced to 1 MHz using the prescaler.
 * With ARR = 999, the timer overflows every 1000 ticks:
 *
 * 1 MHz / 1000 = 1 kHz
 *
 * Therefore, one update event occurs every 1 ms.
 */
static void TIM_Config_1kHz(TIM_TypeDef* TIMX, int use_irq){
    uint32_t tim_clk = RCC_GetTIMClock(TIMX);

    if (tim_clk == 0U) {
        return;
    }

    uint32_t prescaler = tim_clk / 1000000UL;

    if (prescaler == 0U) {
        prescaler = 1U;
    }

    TIM_Config(TIMX, (int)prescaler, 999, use_irq);
}

/* ------------- Simple_Timer Functions ------------- */

/**
 * @brief Configure basic timer settings.
 *
 * Configures prescaler, auto-reload value and optionally enables
 * the update interrupt for the selected timer.
 *
 * @param TIMX        Timer instance from TIM1 to TIM8.
 * @param preScaleVal Prescaler value. Hardware PSC is set to preScaleVal - 1.
 * @param limit       Auto-reload value.
 * @param use_irq     Enable update interrupt if non-zero.
 *
 * @note Currently assumes default bus clock settings.
 * @note For TIM1 and TIM8 only the update interrupt is enabled.
 */
void TIM_Config(TIM_TypeDef* TIMX, int preScaleVal, int limit, int use_irq){
	if(preScaleVal <= 0) {
		return;
	}

	TIM_Enable_Clock_and_NVIC(TIMX, use_irq);
	TIMX->CR1 = 0;
	TIMX->PSC = preScaleVal - 1;
	TIMX->ARR = limit;
	TIMX->CNT = 0;

	if(use_irq){
		TIMX->DIER |= TIM_DIER_UIE;
	} else {
		TIMX->DIER &= ~TIM_DIER_UIE;
	}

	TIMX->EGR = TIM_EGR_UG;
	TIMX->SR &= ~TIM_SR_UIF;
}

void TIM_Start(TIM_TypeDef* TIMX){
	TIMX->CR1 |= TIM_CR1_CEN;
}

void TIM_Stop(TIM_TypeDef* TIMX){
	TIMX->CR1 &= ~TIM_CR1_CEN;
}

void TIM_ResetCounter(TIM_TypeDef* TIMX){
	TIMX->CNT = 0;
}

int TIM_GetCounter(TIM_TypeDef* TIMX){
	return TIMX->CNT;
}

/**
 * @brief Initialize TIM6 as millisecond delay timer.
 *
 * Configures TIM6 so that one update event occurs every 1 ms
 * with the current assumed timer clock configuration.
 */
void delay_init(void){
    TIM_Config_1kHz(TIM6, 0);
}

/**
 * @brief Blocking millisecond delay using TIM6.
 *
 * Uses TIM6 update events to wait for the requested number of milliseconds.
 * This function blocks until the delay has elapsed.
 *
 * @param ms Delay time in milliseconds.
 *
 * @note 	TIM6 must be initialized with delay_init() before using this function.
 * @note 	Do not use TIM6 for other timing tasks while this delay function is active.
 */
void delay_ms(int ms){
    if(ms <= 0){
        return;
    }

    TIM6->SR &= ~TIM_SR_UIF;
    TIM_ResetCounter(TIM6);
    TIM_Start(TIM6);

    for(int i = 0; i < ms; i++){
        while((TIM6->SR & TIM_SR_UIF) == 0U){
        }
        TIM6->SR &= ~TIM_SR_UIF;
    }

    TIM_Stop(TIM6);
}

/**
 * @brief Initialize the global millisecond tick.
 *
 * TIM7 is configured to generate an interrupt every 1 ms.
 * The TIM7 interrupt handler increments sys_tick_ms.
 */
void tick_init(void){
    /*
     * Make sure the vector table base address points to the application
     * vector table that contains TIM7_IRQHandler.
     */
    SCB->VTOR = (uint32_t)g_pfnVectors;

    TIM_Config_1kHz(TIM7, 1);

    TIM_Start(TIM7);
}


/**
 * @brief TIM7 interrupt handler for the global millisecond tick.
 *
 * Called every 1 ms after tick_init().
 */
void TIM7_IRQHandler(void){
    if(TIM7->SR & TIM_SR_UIF){
        TIM7->SR &= ~TIM_SR_UIF;
        sys_tick_ms++;
    }
}

uint32_t get_tick_ms(void){
    return sys_tick_ms;
}

