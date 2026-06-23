/**
 * @file    simple_timer.h
 * @author  Weber
 * @date    21.05.2026
 * @brief   TIM7 system timer driver header
 *
 * Usage
 * -----
 * 1. TIMER_Delay_init()   – configure TIM7 (1 ms period)
 * 2. TIMER_Delay_ms()     – blocking delay
 * 3. TIMER_GetTick()      – read system tick
 */

#ifndef SIMPLE_TIMER_H
#define SIMPLE_TIMER_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"

#define TIMER_IRQ_ON   1
#define TIMER_IRQ_OFF  0

#define TIMER_TIMEOUT_10_MS     10
#define TIMER_TIMEOUT_100_MS    100
#define TIMER_TIMEOUT_200_MS    200
#define TIMER_TIMEOUT_1000_MS   1000
#define TIMER_TIMEOUT_5000_MS   5000
#define TIMER_TIMEOUT_10000_MS  10000

typedef enum {
    TIMER_OK    = 0,
    TIMER_ERROR = 1
} TIMER_Status_TypeDef;

/**
 * @brief Configure basic timer settings
 *
 * @param [in] TIMX        Timer instance
 * @param [in] preScaleVal Prescaler value
 * @param [in] limit       Auto-reload value
 * @param [in] use_irq     Enable update interrupt if non-zero
 */
void TIMER_Config(TIM_TypeDef* TIMX, int preScaleVal, int limit, int use_irq);

/**
 * @brief Start the timer counter
 *
 * @param [in] TIMX Timer instance
 */
void TIMER_Start(TIM_TypeDef* TIMX);

/**
 * @brief Stop the timer counter
 *
 * @param [in] TIMX Timer instance
 */
void TIMER_Stop(TIM_TypeDef* TIMX);

/**
 * @brief Reset counter to zero
 *
 * @param [in] TIMX Timer instance
 */
void TIMER_ResetCounter(TIM_TypeDef* TIMX);

/**
 * @brief Get the current counter value
 *
 * @param [in]  TIMX    Timer instance
 * @param [out] counter Current counter value
 *
 * @retval TIMER_OK    Success
 * @retval TIMER_ERROR Null pointer
 */
TIMER_Status_TypeDef TIMER_GetCounter(TIM_TypeDef* TIMX, int* counter);

/**
 * @brief Read a status flag
 *
 * @param [in]  TIMX   Timer instance
 * @param [in]  flag   Flag mask to check
 * @param [out] result Flag state
 *
 * @retval TIMER_OK    Success
 * @retval TIMER_ERROR Null pointer
 */
TIMER_Status_TypeDef TIMER_GetFlag(TIM_TypeDef* TIMX, uint32_t flag, uint32_t* result);

/**
 * @brief Clear a status flag
 *
 * @param [in] TIMX Timer instance
 * @param [in] flag Flag mask to clear
 */
void TIMER_ClearFlag(TIM_TypeDef* TIMX, uint32_t flag);

/**
 * @brief Enable update interrupt
 *
 * @param [in] TIMX Timer instance
 */
void TIMER_EnableIT(TIM_TypeDef* TIMX);

/**
 * @brief Disable update interrupt
 *
 * @param [in] TIMX Timer instance
 */
void TIMER_DisableIT(TIM_TypeDef* TIMX);

/**
 * @brief Initialise TIM6 as millisecond delay timer
 */
void TIMER_Delay_init(void);

/**
 * @brief Blocking millisecond delay using TIM6
 *
 * @param [in] ms Delay time in milliseconds
 */
void TIMER_Delay_ms(int ms);

#endif /* SIMPLE_TIMER_H */
