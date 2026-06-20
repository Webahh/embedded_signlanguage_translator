/*
 * simple_scheduler.h
 *
 * Priority preemptive scheduler.  The tick source is TIM7 (1 ms period).
 * The scheduler preempts tasks via PendSV every 1 ms and dispatches the
 * highest-priority ready task.  Lower priority values = higher priority.
 *
 * Each task runs to completion once per period.  When a task function
 * returns, the scheduler parks it and re-initialises its stack frame.
 * On the next period tick the task is marked ready and PendSV selects it
 * again, starting from the function entry.  Static or global variables
 * preserve state across invocations.
 *
 * Example – blink an LED every 500 ms:
 *
 * @code{.c}
 * void vLEDTask(void){
 *     GPIO_BSRR_toggle(GPIOG, LED2_PIN);
 * }
 *
 * void app_init(void){
 *     SCHEDULER_System_init();
 *     uint8_t idx;
 *     SCHEDULER_Task_add(vLEDTask, "LED", 500, 2, &idx);
 * }
 *
 * void app_run(void){
 *     SCHEDULER_Tasks_run();  // never returns
 * }
 * @endcode
 *
 * @note  A task that returns (run-to-completion) has its ready flag
 *        cleared by SCHEDULER_Task_exit and is re-marked ready by the
 *        TIM7 ISR only when its period elapses.  A while(1) task never
 *        returns, so its ready flag stays set; PendSV may still preempt
 *        it when a higher-priority task becomes ready, resuming it on
 *        the next tick
 *
 * @author  Groß
 * @date    May 24, 2026
 */

#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#include "stm32n657xx.h"

#define SCHEDULER_MAX_TASKS			10
#define SCHEDULER_DEFAULT_STACK_SIZE	SCHEDULER_STACK_SIZE_WORDS	// words per task stack

// Idle task occupies the last slot
#define SCHEDULER_IDLE_TASK_INDEX	(SCHEDULER_MAX_TASKS - 1)

// Stack limit – PSPLIM is set to the bottom of each task's stack on every
// context switch.  The hardware raises a Stack Usage Fault instantly when
// SP < PSPLIM (ARMv8.1-M).  Each stack is 256 words = 1024 bytes, with a
// guard zone below PSPLIM so the CPU has room to push the exception frame
// during fault entry without tripping over itself.
#define SCHEDULER_STACK_SIZE_WORDS		256
#define SCHEDULER_STACK_SIZE_BYTES		1024
#define SCHEDULER_STACK_GUARD_BYTES		128


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
