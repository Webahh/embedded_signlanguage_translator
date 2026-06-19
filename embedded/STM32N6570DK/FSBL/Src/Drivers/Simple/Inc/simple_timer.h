/*
 * simple_timer.h
 *
 *  Created on: May 21, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_TIMER_H
#define SIMPLE_TIMER_H

#include "stm32n657xx.h"

#define TIM_IRQ_ON 	1
#define TIM_IRQ_OFF	0

#define TIMEOUT_10_MS	 			10
#define TIMEOUT_100_MS				100
#define TIMEOUT_200_MS				200
#define TIMEOUT_1000_MS				1000
#define TIMEOUT_5000_MS				5000
#define TIMEOUT_10000_MS			10000

void TIM_Config(TIM_TypeDef* TIMX, int preScaleVal, int limit, int use_irq);
void TIM_Start(TIM_TypeDef* TIMX);
void TIM_Stop(TIM_TypeDef* TIMX);
void TIM_ResetCounter(TIM_TypeDef* TIMX);
int TIM_GetCounter(TIM_TypeDef* TIMX);
void delay_init();
void delay_ms(int ms);

#endif /* SIMPLE_TIMER_H */
