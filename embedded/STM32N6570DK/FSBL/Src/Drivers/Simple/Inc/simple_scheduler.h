/*
 * simple_scheduler.h
 *
 * Priority preemptive scheduler.  The tick source is TIM7 (1 ms period).
 * The scheduler preempts tasks via PendSV every 1 ms and dispatches the
 * highest-priority ready task.  Lower priority values = higher priority.
 *
 * Task functions must have an infinite loop.  The scheduler never calls
 * them repeatedly; it merely marks them ready every period_ms and the
 * PendSV context-switch preempts whichever task is currently running.
 * A task that omits the while(1) will fall through to SCHEDULER_Task_exit
 * and be re-initialised on its next period.
 *
 * Example task – infinite loop, preempted at 1 ms granularity:
 *
 * @code{.c}
 * void vLEDTask(void){
 *     while (1) {
 *         GPIO_Pin_set(LED2, 1);
 *         // ... return here after 500 ms when the scheduler
 *         // marks us ready again; the loop body runs once
 *         // per period and does NOT block
 *     }
 * }
 *
 * void app_init(void){
 *     SCHEDULER_System_init();
 *     uint8_t idx;
 *     SCHEDULER_Task_add(vLEDTask, "LED", 500, 2, &idx);
 *     // ...
 * }
 *
 * void app_run(void){
 *     SCHEDULER_Tasks_run();  // never returns
 * }
 * @endcode
 *
 * @author  Groß
 * @date    May 24, 2026
 */

#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#include "stm32n657xx.h"

#define SCHEDULER_MAX_TASKS			10
#define SCHEDULER_DEFAULT_STACK_SIZE	256	// words per task stack

// Idle task occupies the last slot
#define SCHEDULER_IDLE_TASK_INDEX	(SCHEDULER_MAX_TASKS - 1)

typedef void (*SCHEDULER_TaskFunction_TypeDef)(void);

typedef enum {
	SCHEDULER_OK				=  0,
	SCHEDULER_ERR_FULL			= -1,
	SCHEDULER_ERR_NOT_FOUND		= -2,
	SCHEDULER_ERR_TASK_INVALID	= -3
} SCHEDULER_Status_TypeDef;

/**
 * @brief  Register a periodic task with the scheduler
 *
 * @param [in]  pvTaskCode | Pointer to the task function
 * @param [in]  pcName     | Human-readable task name (currently unused)
 * @param [in]  period_ms  | Task period in milliseconds
 * @param [in]  priority   | Scheduling priority (0 = highest, 255 = lowest)
 * @param [out] taskIndex  | Assigned task slot index
 *
 * @retval SCHEDULER_OK       on success
 * @retval SCHEDULER_ERR_FULL if no slot available
 */
SCHEDULER_Status_TypeDef SCHEDULER_Task_add(SCHEDULER_TaskFunction_TypeDef pvTaskCode, const char* pcName, uint32_t period_ms, uint8_t priority, uint8_t* taskIndex);

/**
 * @brief  Remove a task from the scheduler
 *
 * @param [in] taskIndex | Slot index returned by SCHEDULER_Task_add()
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND on invalid index
 */
SCHEDULER_Status_TypeDef SCHEDULER_Task_remove(int taskIndex);

/**
 * @brief  Initialise the scheduler hardware (TIM7, NVIC)
 *
 * @note   Must be called once before SCHEDULER_Tasks_run()
 */
void SCHEDULER_System_init(void);

/**
 * @brief  Read the current system tick counter
 *
 * @param [out] tick | Tick value (ms since scheduler start)
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND if tick is NULL
 */
SCHEDULER_Status_TypeDef SCHEDULER_Tick_get(uint32_t* tick);

/**
 * @brief  Start the preemptive scheduler
 *
 * This function never returns.
 */
void SCHEDULER_Tasks_run(void);

#endif /* SIMPLE_SCHEDULER_H */
