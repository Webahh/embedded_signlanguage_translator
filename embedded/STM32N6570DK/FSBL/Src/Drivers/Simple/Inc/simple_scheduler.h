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
 *     SCHEDULER_Task_add(vLEDTask, "LED", 500, 2, 128, &idx);
 * }
 *
 * void app_run(void){
 *     SCHEDULER_Tasks_run();  // never returns
 * }
 * @endcode
 *
 * @note  A task that returns (run-to-completion) transitions to
 *        TaskBlocked via SCHEDULER_Task_exit and is moved back to
 *        TaskReady by the TIM7 ISR when its period elapses.  A while(1)
 *        task never returns, so it stays TaskReady; PendSV may still
 *        preempt it when a higher-priority task becomes ready.
 *
 * Task Statistics
 * ---------------
 * The scheduler tracks per-task CPU usage using DWT CYCCNT (CPU cycle
 * counter at core clock frequency).  Every 1 second the idle task prints
 * a table with per-window counters, reset each cycle:
 *
 *   - min / max / avg  – min, max, and average cycles per invocation
 *   - %ACT             – share of active (non-WFI) CPU time
 *   - Prempt           – number of times preempted
 *   - Invoc            – invocation count
 *
 * ISR cycles are tracked via SCHEDULER_ISR_enter()/exit() and subtracted
 * from the interrupted task's totals, so %ACT reflects pure application-
 * level time.  An "ISR" row shows total interrupt overhead separately.
 *
 * Total cycles for the window = avg * invoc (printed in "cycle" column).
 * Idle %ACT is time spent in the idle loop with no ISR active.
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

#define SCHEDULER_MAX_TASKS				5
#define SCHEDULER_DEFAULT_STACK_SIZE	SCHEDULER_STACK_SIZE_WORDS

// Idle task occupies the last slot
#define SCHEDULER_IDLE_TASK_INDEX	(SCHEDULER_MAX_TASKS - 1)

#define SCHEDULER_STACK_SIZE_WORDS      4096U
#define SCHEDULER_STACK_SIZE_BYTES      (SCHEDULER_STACK_SIZE_WORDS * 4)
// Guard band: ensures headroom for PendSV save (R4-R11 + S16-S31 if FP active,
// up to 24 words = 96 bytes) plus generous reserve.
#define SCHEDULER_STACK_GUARD_BYTES     512U

// Exception frame reserved: hardware auto-push on exception entry.
// With ASPEN+LSPEN enabled (see SCHEDULER_Tasks_run()), non-FPU ISRs
// push only the basic 8-word frame (32 B) onto the task's PSP.
// The ISR body itself runs on MSP, not the task stack.
#define SCHEDULER_EXCEPTION_FRAME_WORDS 8
#define SCHEDULER_EXCEPTION_FRAME_BYTES (SCHEDULER_EXCEPTION_FRAME_WORDS * 4)

#define SCHEDULER_DEFAULT_STACK_SIZE    SCHEDULER_STACK_SIZE_WORDS
#define SCHEDULER_STACK_POOL_SIZE_WORDS (SCHEDULER_MAX_TASKS * SCHEDULER_STACK_SIZE_WORDS)

// For debugging on Fault
#define SCHED_MAGIC						0x53434448u

typedef void (*SCHEDULER_Task_Function_TypeDef)(void);
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
 * @param [in]  pvTaskCode      | Pointer to the task function
 * @param [in]  pcName          | Human-readable task name
 * @param [in]  period_ms       | Task period in milliseconds
 * @param [in]  priority        | Scheduling priority (0 = highest, 255 = lowest)
 * @param [in]  stack_size_words| Stack size in 32-bit words
 * @param [out] taskIndex       | Assigned task slot index
 *
 * @retval SCHEDULER_OK       on success
 * @retval SCHEDULER_ERR_FULL if no slot available
 */
SCHEDULER_Status_TypeDef SCHEDULER_Task_add(SCHEDULER_Task_Function_TypeDef pvTaskCode, const char* pcName, uint32_t period_ms, uint8_t priority, uint32_t stack_size_words, uint8_t* taskIndex);

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

/**
 * @brief  Suspend a task (prevents it from being scheduled)
 *
 * @param [in] taskIndex | Task slot to suspend
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND if task index invalid or slot free
 */
SCHEDULER_Status_TypeDef SCHEDULER_Task_suspend(uint8_t taskIndex);

/**
 * @brief  Resume a suspended task (moves it to the ready state)
 *
 * @param [in] taskIndex | Task slot to resume
 *
 * @retval SCHEDULER_OK          on success
 * @retval SCHEDULER_ERR_NOT_FOUND if task index invalid or not suspended
 */
SCHEDULER_Status_TypeDef SCHEDULER_Task_resume(uint8_t taskIndex);

/**
 * @brief  Suspend the currently running task.
 *
 * Caller must hold IRQs disabled (__disable_irq()) for a race-free
 * check-then-suspend pattern.  This function sets the current task's
 * state to TaskSuspended and pends PendSV so the scheduler can
 * switch to another ready task.
 *
 * @note   The caller should follow with __enable_irq() and a WFI
 *         loop that waits for the condition that will trigger a
 *         SCHEDULER_Task_resume() call (e.g. from a peripheral ISR).
 */
void SCHEDULER_Task_suspend_self(void);

/**
 * @brief  Get the stack usage (high-water mark) for a task
 *
 * Scans from the stack base upward counting words that have been
 * overwritten (no longer equal to the initial 0xA5A5A5A5 pattern).
 *
 * @param [in] taskIndex | Task slot index
 *
 * @return Number of 32-bit words used (0 if invalid index)
 */
uint32_t SCHEDULER_GetTaskStackUsed(uint8_t taskIndex);

/**
 * @brief  Get the allocated stack size for a task
 *
 * @param [in] taskIndex | Task slot index
 *
 * @return Stack size in 32-bit words (0 if invalid index)
 */
uint32_t SCHEDULER_GetTaskStackSize(uint8_t taskIndex);

/**
 * @brief  Mark ISR entry for cycle tracking
 *
 * Call at the top of every peripheral ISR.  See simple_scheduler.c for
 * full documentation.
 */
void SCHEDULER_ISR_enter(void);

/**
 * @brief  Mark ISR exit for cycle tracking
 *
 * Call at the bottom of every peripheral ISR (paired with ISR_enter).
 */
void SCHEDULER_ISR_exit(void);

#endif /* SIMPLE_SCHEDULER_H */
