/**
 * @file    simple_scheduler.h
 * @author  Gross
 * @date    05.06.2026
 * @brief   Priority preemptive scheduler driver header
 *
 *          Tick source: TIM7 (1 ms period)
 *          Tasks preempt via PendSV
 *          Lower priority values = higher priority
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
 *   - min / max / avg  -- min, max, and average cycles per invocation
 *   - %ACT             -- share of active (non-WFI) CPU time
 *   - Prempt           -- number of times preempted
 *   - Invoc            -- invocation count
 *
 * ISR cycles are tracked via SCHEDULER_ISR_enter()/exit() and subtracted
 * from the interrupted task's totals, so %ACT reflects pure application-
 * level time.  An "ISR" row shows total interrupt overhead separately.
 *
 * Total cycles for the window = avg * invoc (printed in "cycle" column).
 * Idle %ACT is time spent in the idle loop with no ISR active.
 */

#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#include "stm32n657xx.h"

// ---- Configuration ---------------------------------------------------------

#define SCHEDULER_MAX_TASKS				9

// Idle task occupies the last slot
#define SCHEDULER_IDLE_TASK_INDEX		(SCHEDULER_MAX_TASKS - 1)

#define SCHEDULER_STACK_SIZE_WORDS      2048U
// Exception frame reserved: hardware auto-push on exception entry.
// With ASPEN+LSPEN enabled (see SCHEDULER_Tasks_run()), non-FPU ISRs
// push only the basic 8-word frame (32 B) onto the task's PSP.
// The ISR body itself runs on MSP, not the task stack.
#define SCHEDULER_EXCEPTION_FRAME_BYTES (8 * 4)

#define SCHEDULER_STACK_POOL_SIZE_WORDS 8192

// For debugging on Fault
#define SCHED_MAGIC						0x53434448u

// ---- Typedefs --------------------------------------------------------------

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

// ---- Extern variables ------------------------------------------------------

extern volatile Scheduler_Fault_Dump_TypeDef	g_sched_fault;

// ---- ISR cycle tracking ----------------------------------------------------

/**
 * @brief Mark ISR entry for cycle tracking.
 *
 * Call at the very top of any peripheral ISR to measure its CPU cycle
 * contribution.  Nested ISRs are handled correctly -- only the outermost
 * entry/exit pair records the full ISR burst.
 *
 * Every cycle spent in ISR context is subtracted from the interrupted
 * task's cycle total and accumulated in a separate ISR counter,
 * ensuring per-task %ACT reflects pure application time.
 *
 * Usage (place at top and bottom of every peripheral ISR):
 * @code{.c}
 * void XXX_IRQHandler(void){
 *     SCHEDULER_ISR_enter();
 *     // ... handler body ...
 *     SCHEDULER_ISR_exit();
 * }
 * @endcode
 */
void SCHEDULER_ISR_enter(void);

/**
 * @brief  Mark ISR exit for cycle tracking
 *
 * Call at the bottom of every peripheral ISR (paired with ISR_enter).
 */
void SCHEDULER_ISR_exit(void);

// ---- Task lifecycle --------------------------------------------------------

/**
 * @brief Register a new task with the scheduler
 *
 * If *taskIndex is 0xFF, a free slot is auto-selected.  Otherwise
 * the value is treated as a slot hint: if that slot is free
 * (state == TaskDeleted) it is reused; if busy a free slot is
 * auto-searched.
 *
 * The stack is allocated from a fixed-size pool.  If the requested
 * size exceeds the per-task ceiling it is clamped. if below the
 * minimum it is raised.  On re-add with a larger stack the old
 * allocation is abandoned (leaving a hole) and a fresh block is
 * taken from the pool.
 *
 * @param [in]  pvTaskCode       Task entry function
 * @param [in]  pcName           Human-readable name (may be NULL)
 * @param [in]  period_ms        Reschedule period in ms (0 = one-shot)
 * @param [in]  priority         0 = highest, 0xFF = lowest
 * @param [in]  stack_size_words Stack size in 32-bit words
 * @param [out] taskIndex        Assigned slot index, or 0xFF on error
 *
 * @retval SCHEDULER_OK          Task registered successfully
 * @retval SCHEDULER_ERR_FULL    No free slot or insufficient pool space
 * @retval SCHEDULER_ERR_TASK_INVALID  pvTaskCode is NULL
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

// ---- System lifecycle ------------------------------------------------------

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

// ---- Stack measurement -----------------------------------------------------

/**
 * @brief  Get the allocated stack size for a task
 *
 * @param [in] taskIndex | Task slot index
 *
 * @return Stack size in 32-bit words (0 if invalid index)
 */
uint32_t SCHEDULER_GetTaskStackSize(uint8_t taskIndex);

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

// ---- Debug API -------------------------------------------------------------

/**
 * @brief Get the slot index of the currently running task
 *
 * @param [out] taskIndex  Current task slot
 *
 * @retval SCHEDULER_OK            Success
 * @retval SCHEDULER_ERR_NOT_FOUND taskIndex is NULL
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetCurrentTask(int* taskIndex);

/**
 * @brief Get the name of a registered task
 *
 * @param [in]  task  Task slot index
 * @param [out] name  Task name pointer
 *
 * @retval SCHEDULER_OK            Success
 * @retval SCHEDULER_ERR_NOT_FOUND Invalid index or NULL pointer
 */
SCHEDULER_Status_TypeDef SCHEDULER_GetTaskName(uint8_t task, const char** name);

#endif /* SIMPLE_SCHEDULER_H */
