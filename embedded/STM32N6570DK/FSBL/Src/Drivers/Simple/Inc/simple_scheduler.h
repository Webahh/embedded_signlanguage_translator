/**
 * @file    simple_scheduler.h
 * @author  Gross
 * @date    05.06.2026
 * @brief   Priority preemptive scheduler driver header
 *
 *          Tick source: TIM7 (1 ms period).  Tasks preempt via PendSV.
 *          Lower priority values = higher priority.
 *
 * Usage
 * -----
 * 1. SCHEDULER_System_init()   - initialise scheduler data
 * 2. SCHEDULER_Task_add()      - register tasks
 * 3. SCHEDULER_Tasks_run()     - start scheduling (never returns)
 *
 * Example - blink an LED every 500 ms:
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
 * Task Statistics
 * ---------------
 * The scheduler tracks per-task CPU usage using DWT CYCCNT (CPU cycle
 * counter at core clock frequency).  Every 1 second the idle task prints
 * a table with per-window counters, reset each cycle:
 *
 *   - min / max / avg  – min, max, and average cycles per invocation
 *   - %CPU             – share of total wall-clock time (800 MHz basis)
 *   - Prempt           – number of times preempted
 *   - Invoc            – invocation count
 *
 * Total cycles for the window = avg * invoc (printed in "cycle" column).
 * Idle %CPU is derived as (wall_cycles - sum(task_cycles)) / wall_cycles,
 * so it includes WFI sleep time during which DWT CYCCNT stops.
 *
 * Usage
 * -----
 * 1. SCHEDULER_System_init()  – configures TIM7 (1 ms tick) and DWT
 * 2. SCHEDULER_Task_add()     – register tasks (period, priority)
 * 3. SCHEDULER_Tasks_run()    – start scheduler (never returns)
 *
 */

#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#include "stm32n657xx.h"

#define SCHEDULER_MAX_TASKS				10
#define SCHEDULER_DEFAULT_STACK_SIZE	SCHEDULER_STACK_SIZE_WORDS

// Idle task occupies the last slot
#define SCHEDULER_IDLE_TASK_INDEX	(SCHEDULER_MAX_TASKS - 1)

#define SCHEDULER_STACK_SIZE_WORDS      2048U
#define SCHEDULER_STACK_SIZE_BYTES      8192U
#define SCHEDULER_STACK_GUARD_BYTES     256U
#define SCHEDULER_DEFAULT_STACK_SIZE    SCHEDULER_STACK_SIZE_WORDS

// For debugging on Fault
#define SCHED_MAGIC						0x53434448u


typedef void (*SCHEDULER_TaskFunction_TypeDef)(void);

typedef enum {
	SCHEDULER_OK				=  0,
	SCHEDULER_ERR_FULL			= -1,
	SCHEDULER_ERR_NOT_FOUND		= -2,
	SCHEDULER_ERR_TASK_INVALID	= -3
} SCHEDULER_Status_TypeDef;

typedef struct {
	uint32_t	magic;
	uint32_t	reason;
	uint32_t	task;
	uint32_t	tick;

	uint32_t	cfsr, hfsr, dfsr, afsr;
	uint32_t	mmfar, bfar;
	uint32_t	icsr, shcsr;

	uint32_t	msp, psp, psplim, control, exc_return;
	uint32_t	r0, r1, r2, r3, r12, lr, pc, xpsr;
} Scheduler_Fault_Dump_TypeDef;

// ── Mutable runtime state ──
extern volatile Scheduler_Fault_Dump_TypeDef	g_sched_fault;

/**
 * @brief  Get the index of the currently running task
 *
 * @param [out] taskIndex | Pointer to store the task index
 *
 * @retval SCHEDULER_OK on success
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetCurrentTask(int* taskIndex);

/**
 * @brief  Get the last fault dump
 *
 * @param [out] dump | Pointer to store the fault-dump address
 *
 * @retval SCHEDULER_OK on success
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetLastFault(volatile const Scheduler_Fault_Dump_TypeDef** dump);

/**
 * @brief  Get the free stack space for a task
 *
 * @param [in]  task | Task index
 * @param [out] free | Pointer to store free stack bytes
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND if task index invalid
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetTaskStackFree(uint8_t task, uint32_t* free);

/**
 * @brief  Get the name of a task
 *
 * @param [in]  task | Task index
 * @param [out] name | Pointer to store the task name pointer
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND if task index invalid
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetTaskName(uint8_t task, const char** name);

/**
 * @brief  Register a periodic task with the scheduler
 *
 * @param [in]  pvTaskCode | Pointer to the task function
 * @param [in]  pcName     | Human-readable task name
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
