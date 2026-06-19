/*
 * simple_scheduler.h
 *
 * Priority preemptive scheduler.  The tick source is TIM7 (1 ms period).
 * The scheduler preempts tasks via PendSV every 1 ms and dispatches the
 * highest-priority ready task.  Lower priority values = higher priority.
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

typedef struct {
	SCHEDULER_TaskFunction_TypeDef	function;			//  0
	uint32_t						period_ms;			//  4
	uint32_t						last_run_ms;		//  8
	uint8_t							priority;			// 12
	uint8_t							ready;				// 13
	uint8_t							active;				// 14
	uint8_t							needs_init;			// 15
	uint32_t						saved_sp;			// 16
	uint32_t						saved_exc_return;	// 20
} SCHEDULER_TaskHandle_TypeDef;							// 24 bytes

// TCB = Task Control Block
#define TCB_SIZE		24
#define TCB_SAVED_SP	16
#define TCB_EXC_RETURN	20
#define TCB_NEEDS_INIT	15

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
