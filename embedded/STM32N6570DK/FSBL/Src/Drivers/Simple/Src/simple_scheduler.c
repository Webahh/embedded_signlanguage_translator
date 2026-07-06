/**
 * @file    simple_scheduler.c
 * @author  Gross
 * @date    05.06.2026
 * @brief   Priority preemptive scheduler driver source
 */

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

#include "simple_scheduler.h"
#include "simple_timer.h"
#include "simple_debug_log.h"
#include "config.h"

// -------------------------------------------------------------------------
// Private defines / datatypes
// -------------------------------------------------------------------------

typedef struct {
	SCHEDULER_Task_Function_TypeDef	function;			//  0
	uint32_t						period_ms;			//  4
	uint32_t						last_run_ms;		//  8
	uint8_t							priority;			// 12
	uint8_t							state;				// 13 (_Task_State_TypeDef)
	uint32_t						saved_sp;			// 16 (4-byte aligned)
	uint32_t						saved_exc_return;	// 20
	const char*						pcName;				// 24
} _Task_Handle_TypeDef;							// 28 bytes

typedef enum {
	TaskDeleted   = 0,  // Slot free / task removed
	TaskBlocked   = 1,  // Waiting for period to expire / task returned
	TaskReady     = 2,  // Period has expired, ready to be scheduled
	TaskSuspended = 3   // Explicitly suspended
} _Task_State_TypeDef;

// TCB = Task Control Block
#define _TCB_SIZE           28    // sizeof(_Task_Handle_TypeDef)
#define _TCB_SAVED_SP       16
#define _TCB_EXC_RETURN     20

// Stack frame in words: S16-S31(16) + R4-R11(8) + exception frame(8)
// Must remain 32 regardless of _TCB_SIZE — ARM hardware exception frame is 8 words.
#define _STACK_FRAME_WORDS  32

// Stringify helper for inline assembly
#define _STR_HELPER(x) #x
#define _STR(x)        _STR_HELPER(x)

_Static_assert(offsetof(_Task_Handle_TypeDef, saved_sp) == _TCB_SAVED_SP, "TCB saved_sp offset mismatch with assembly");
_Static_assert(offsetof(_Task_Handle_TypeDef, saved_exc_return) == _TCB_EXC_RETURN, "TCB saved_exc_return offset mismatch with assembly");

_Static_assert(sizeof(_Task_Handle_TypeDef) == _TCB_SIZE, "TCB size mismatch with assembly");

// -------------------------------------------------------------------------
// Private data
// -------------------------------------------------------------------------

static _Task_Handle_TypeDef	_tasks[SCHEDULER_MAX_TASKS];
static volatile uint32_t	_sys_tick_ms = 0;
static int					_current_task = 0;

static volatile int			_scheduler_running = 0;

typedef struct {
	uint32_t					total_cycles;
	uint32_t					total_preempts;
	uint32_t					total_invocations;
	uint32_t					min_cycles;
	uint32_t					max_cycles;
	uint32_t					last_start_cycle;
} _Task_Stats_TypeDef;

static _Task_Stats_TypeDef	_task_stats[SCHEDULER_MAX_TASKS];
static uint32_t				_last_stats_print_cycle;
static volatile int			_stats_pending;

// ISR cycle tracking (written by ISR_enter/exit, consumed by PendSV and PrintStats)
static volatile uint32_t			_isr_total_cycles;
static volatile int					_isr_nest;
static volatile uint32_t			_isr_entry_cycle;
static volatile int					_isr_owner_task;
static volatile uint32_t			_task_isr_cycles[SCHEDULER_MAX_TASKS];

// Diagnostic: counts how many times WFI returns per stats window
static uint32_t						_idle_wakeups;

static uint32_t _task_stacks[SCHEDULER_MAX_TASKS][SCHEDULER_DEFAULT_STACK_SIZE] __attribute__((aligned(8)));

_Static_assert(sizeof(_task_stacks[0]) == SCHEDULER_STACK_SIZE_BYTES,"SCHEDULER_STACK_SIZE_BYTES mismatch");

volatile Scheduler_Fault_Dump_TypeDef g_sched_fault;

// Forward declarations (called from assembly)
void SCHEDULER_Task_exit(void);
static int SCHEDULER_SelectNextTask(void);
void SCHEDULER_FaultHandler_C(uint32_t exc_return, uint32_t *frame, uint32_t reason);
void SCHEDULER_StartTick(void);
static void SCHEDULER_PrintStats(void);

void UsageFault_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void NMI_Handler(void);

// -------------------------------------------------------------------------
// Stack initialisation
// -------------------------------------------------------------------------

/**
 * @brief Initializes Task by filling with default values
 *
 * @param i | task index
 */
static void SCHEDULER_InitTaskStack(int i){
	uint32_t *stack_base = (uint32_t *)((uint32_t)_task_stacks[i] & ~7U);

	for (uint32_t j = 0; j < SCHEDULER_DEFAULT_STACK_SIZE; j++) {
		stack_base[j] = 0xA5A5A5A5;
	}

	uint32_t *stack_end = (uint32_t *)(((uint32_t)_task_stacks[i] + sizeof(_task_stacks[i])) & ~7U);
	uint32_t *sp = stack_end - _STACK_FRAME_WORDS;

	// sp[0..15]  S16-S31  (FPU callee-saved)
	// sp[16..23] R4-R11   (core callee-saved)
	// sp[24..31] exception frame: R0-R3, R12, LR, PC, xPSR

	sp[_STACK_FRAME_WORDS - 3] = (uint32_t)SCHEDULER_Task_exit; // LR on task return
	sp[_STACK_FRAME_WORDS - 2] = (uint32_t)_tasks[i].function;  // PC
	sp[_STACK_FRAME_WORDS - 1] = 0x01000000UL;                  // xPSR (thumb bit)

	_tasks[i].saved_sp         = (uint32_t)&sp[16];
	_tasks[i].saved_exc_return = 0xFFFFFFFDUL;
}

// -------------------------------------------------------------------------
// Idle task
// -------------------------------------------------------------------------

/**
 * @brief Idle task - waits for the next interrupt
 */
__attribute__((noreturn)) static void SCHEDULER_IdleTask(void){
	while (1) {
		if (_stats_pending) {
			_stats_pending = 0;
			SCHEDULER_PrintStats();
			_idle_wakeups = 0;
		}
		_idle_wakeups++;
		__WFI();
	}
}

// -------------------------------------------------------------------------
// Task exit handler
// -------------------------------------------------------------------------

/**
 * @brief Clean up and park the current task after it returns
 */
void SCHEDULER_Task_exit(void){
	__disable_irq();
	_tasks[_current_task].state = TaskBlocked;

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	while (1) {
		__WFI();
	}
}

// -------------------------------------------------------------------------
// Scheduling policy
// -------------------------------------------------------------------------

/**
 * @brief Selects next task with round robin and priority scheduling
 *
 * Chooses the highest-priority task that is both active and ready.
 * Tasks are scanned starting from the task immediately after the
 * currently running task and wrap around the task table, providing
 * round-robin fairness among tasks with the same priority.
 *
 * @return index of next Task to run
 */
static int SCHEDULER_SelectNextTask(void){
	int best = -1;
	uint8_t best_prio = 0xFF;

	int start = _current_task + 1;
	if (start >= SCHEDULER_IDLE_TASK_INDEX) {
		start = 0;
	}

	for (int n = 0; n < SCHEDULER_IDLE_TASK_INDEX; n++) {
		int i = start + n;
		if (i >= SCHEDULER_IDLE_TASK_INDEX) {
			i -= SCHEDULER_IDLE_TASK_INDEX;
		}

		if (_tasks[i].state == TaskReady) {
			if (_tasks[i].priority < best_prio) {
				best_prio = _tasks[i].priority;
				best = i;
			}
		}
	}

	if (best < 0) {
		best = SCHEDULER_IDLE_TASK_INDEX;
	}

	if (_scheduler_running) {

		// Record stat data
		uint32_t now = DWT->CYCCNT;
		int prev = _current_task;
		_Task_Stats_TypeDef *previous_stats = &_task_stats[prev];

		uint32_t raw = now - previous_stats->last_start_cycle;
		uint32_t isr_part = _task_isr_cycles[prev];
		_task_isr_cycles[prev] = 0;
		uint32_t active = (raw > isr_part) ? (raw - isr_part) : 0;

		previous_stats->total_cycles += active;

		if (best != prev) {
			// Min, Max
			if (active < previous_stats->min_cycles) previous_stats->min_cycles = active;
			if (active > previous_stats->max_cycles) previous_stats->max_cycles = active;

			if (_tasks[prev].state == TaskReady && prev != SCHEDULER_IDLE_TASK_INDEX) {
				previous_stats->total_preempts++;
			}

			_Task_Stats_TypeDef *next_stats = &_task_stats[best];
			next_stats->last_start_cycle = now;
			next_stats->total_invocations++;
		} else {
			previous_stats->last_start_cycle = now;
		}
	}

	return best;
}

// -------------------------------------------------------------------------
// Context switch - SVC (first-task bootstrap)
// -------------------------------------------------------------------------

/**
 * @brief SVC handler - bootstrap the first task and start the tick timer
 */
__attribute__((naked)) void SVC_Handler(void){
	__asm volatile (
		"ldr    r0, =_current_task                  \n"
		"ldr    r3, [r0]                            \n"

		"ldr    r2, =_tasks                         \n"
		"mov    r1, #" _STR(_TCB_SIZE) "             \n"
		"mul    r0, r3, r1                          \n"
		"add    r2, r2, r0                          \n"

		"ldr    r1, =_task_stacks                   \n"
		"ldr    r0, =" _STR(SCHEDULER_STACK_SIZE_BYTES) "\n"
		"mul    r0, r3, r0                          \n"
		"add    r0, r1, r0                          \n"
		"add    r0, r0, #" _STR(SCHEDULER_STACK_GUARD_BYTES) "\n"
		"msr    psplim, r0                          \n"
		"isb                                        \n"

		"ldr    r0, [r2, #" _STR(_TCB_SAVED_SP) "]    \n"
		"ldmia  r0!, {r4-r11}                       \n"
		"msr    psp, r0                             \n"
		"isb                                        \n"

		"movs   r0, #2                              \n"
		"msr    control, r0                         \n"
		"isb                                        \n"

		// Start TIM7 now that PSP and CONTROL are valid
		"push {r0, lr}                              \n"
		"bl SCHEDULER_StartTick                     \n"
		"pop {r0, lr}                               \n"

		"ldr    lr, =0xFFFFFFFD                     \n"
		"bx     lr                                  \n"
	);
}

// -------------------------------------------------------------------------
// Context switch - PendSV (preemptive)
// -------------------------------------------------------------------------

/**
 * @brief PendSV handler - preemptive context switch
 */
__attribute__((naked)) void PendSV_Handler(void){
	__asm volatile (
		"cpsid	i\n"

		"mrs	r0, psp\n"
		"stmdb	r0!, {r4-r11}\n"

		"tst	lr, #0x10\n"
		"it		eq\n"
		"vstmdbeq r0!, {s16-s31}\n"

		"ldr	r2, =_current_task\n"
		"ldr	r3, [r2]\n"
		"ldr	r4, =_tasks\n"

		"mov	r5, #" _STR(_TCB_SIZE) "\n"
		"mul	r3, r3, r5\n"
		"add	r4, r4, r3\n"
		"str	r0, [r4, #" _STR(_TCB_SAVED_SP) "]\n"
		"str	lr, [r4, #" _STR(_TCB_EXC_RETURN) "]\n"

		"push	{r1, lr}\n"
		"bl		SCHEDULER_SelectNextTask\n"
		"pop	{r1, lr}\n"

		"ldr	r2, =_current_task\n"
		"str	r0, [r2]\n"

		"ldr	r4, =_tasks\n"
		"mov	r5, #" _STR(_TCB_SIZE) "\n"
		"mul	r0, r0, r5\n"
		"add	r4, r4, r0\n"

		"1:\n"
		"ldr	r0, =_task_stacks\n"
		"ldr	r2, =_current_task\n"
		"ldr	r3, [r2]\n"
		"ldr	r2, =" _STR(SCHEDULER_STACK_SIZE_BYTES) "\n"
		"mul	r3, r3, r2\n"
		"add	r0, r0, r3\n"
		"add	r0, r0, #" _STR(SCHEDULER_STACK_GUARD_BYTES) "\n"
		"msr	psplim, r0\n"
		"isb\n"

		"ldr	r0, [r4, #" _STR(_TCB_SAVED_SP) "]\n"
		"ldr	lr, [r4, #" _STR(_TCB_EXC_RETURN) "]\n"

		"tst	lr, #0x10\n"
		"it		eq\n"
		"vldmiaeq r0!, {s16-s31}\n"

		"ldmia	r0!, {r4-r11}\n"
		"msr	psp, r0\n"

		"cpsie	i\n"
		"bx		lr\n"
	);
}

// -------------------------------------------------------------------------
// Tick source
// -------------------------------------------------------------------------

/**
 * @brief Start the TIM7 tick timer and enable its interrupt
 */
void SCHEDULER_StartTick(void){
	NVIC_ClearPendingIRQ(TIM7_IRQn);
	NVIC_EnableIRQ(TIM7_IRQn);
	TIMER_Start(TIM7);
	_scheduler_running = 1;
}

// -------------------------------------------------------------------------
// Scheduler statistics
// -------------------------------------------------------------------------

static uint32_t _last_print_tick = 0;
static uint32_t _last_stats_tick = 0;

/**
 * @brief Mark ISR entry for cycle tracking.
 *
 * Call at the very top of any peripheral ISR to measure its CPU cycle
 * contribution.  Nested ISRs are handled correctly — only the outermost
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
void SCHEDULER_ISR_enter(void){
	if (_isr_nest == 0) {
		_isr_entry_cycle = DWT->CYCCNT;
		_isr_owner_task = _current_task;
	}
	_isr_nest++;
}

/**
 * @brief Mark ISR exit for cycle tracking (see SCHEDULER_ISR_enter).
 */
void SCHEDULER_ISR_exit(void){
	_isr_nest--;
	if (_isr_nest == 0) {
		uint32_t isr_cycles = DWT->CYCCNT - _isr_entry_cycle;
		_isr_total_cycles += isr_cycles;
		if (_isr_owner_task >= 0 && _isr_owner_task < SCHEDULER_MAX_TASKS) {
			_task_isr_cycles[_isr_owner_task] += isr_cycles;
		}
	}
}

static void SCHEDULER_PrintStats(void){
	__disable_irq();

	uint32_t total = 0;

	// Snapshot struct
	struct {
		uint32_t cycles;
		uint32_t min_cycles;
		uint32_t max_cycles;
		uint32_t preempts;
		uint32_t invocs;
		const char* name;
	} snap[SCHEDULER_MAX_TASKS];
	int snap_n = 0;

	uint32_t now = DWT->CYCCNT;
	uint32_t elapsed = now - _last_stats_print_cycle;
	_last_stats_print_cycle = now;

	uint32_t now_tick = _sys_tick_ms;
	uint32_t elapsed_ms = now_tick - _last_stats_tick;
	_last_stats_tick = now_tick;

	// Fill stat snapshots with data
	for (int i = 0; i < SCHEDULER_MAX_TASKS; i++) {
		if (i == SCHEDULER_IDLE_TASK_INDEX) continue;
		if (_tasks[i].state != TaskDeleted && _tasks[i].function) {
			_Task_Stats_TypeDef *s = &_task_stats[i];
			snap[snap_n].cycles = s->total_cycles;
			snap[snap_n].min_cycles = (s->min_cycles == 0xFFFFFFFF) ? 0 : s->min_cycles;
			snap[snap_n].max_cycles = s->max_cycles;
			snap[snap_n].preempts = s->total_preempts;
			snap[snap_n].invocs = s->total_invocations;
			snap[snap_n].name = _tasks[i].pcName ? _tasks[i].pcName : "?";
			total += s->total_cycles;
			s->total_cycles = 0;
			s->total_preempts = 0;
			s->total_invocations = 0;
			s->min_cycles = 0xFFFFFFFF;
			s->max_cycles = 0;
			s->last_start_cycle = now;
			snap_n++;
		}
	}

	uint32_t isr_snapshot = _isr_total_cycles;
	_isr_total_cycles = 0;
	uint32_t idle = (elapsed > (total + isr_snapshot)) ? (elapsed - total - isr_snapshot) : 0;
	__enable_irq();

	if (elapsed == 0) {
		return;
	}

	DEBUG_PRINTF("\r\n");
	DEBUG_PRINTF("%-14.14s %-10s %-10s %-10s %-10s %-7s %-9s %-8s\r\n",
		"Name", "cycle", "min", "max", "avg", "%ACT", "Prempt", "Invoc");
	DEBUG_PRINTF("---------------------------------------------------------------------------------------\r\n");

	for (int i = 0; i < snap_n; i++) {
		uint32_t avg = snap[i].invocs ? (snap[i].cycles / snap[i].invocs) : 0;
		DEBUG_PRINTF("%-14.14s %10lu %10lu %10lu %10lu %6.2f%% %9lu %8lu\r\n",
			snap[i].name,
			snap[i].cycles,
			snap[i].min_cycles,
			snap[i].max_cycles,
			avg,
			(float)snap[i].cycles * 100.0f / (float)elapsed,
			snap[i].preempts,
			snap[i].invocs);
	}

	DEBUG_PRINTF("%-14.14s %10s %10s %10s %10s %6.2f%% %9s %8s\r\n",
		"Idle", "0", "0", "0", "0",
		(float)idle * 100.0f / (float)elapsed, "0", "0");

	DEBUG_PRINTF("%-14.14s %10lu %10s %10s %10s %6.2f%% %9s %8s\r\n",
		"ISR", isr_snapshot, "-", "-", "-",
		(float)isr_snapshot * 100.0f / (float)elapsed, "-", "-");

	float cpu_util = (float)(total + isr_snapshot) * 100.0f / (float)elapsed;
	float wake_rate = (float)_idle_wakeups / (float)elapsed_ms;
	DEBUG_PRINTF("\r\nCPU Util: %6.2f%% (wall %lu ms, active %lu cyc, %lu WFI/s)\r\n",
		cpu_util, elapsed_ms, elapsed, (uint32_t)(wake_rate * 1000.0f));
	DEBUG_PRINTF("=======================================================================================\r\n");
}

/**
 * @brief TIM7 interrupt handler - scheduler tick
 *
 * Increments the system tick counter and marks tasks ready when
 * their period has elapsed, then pends PendSV for context switch.
 */
void TIM7_IRQHandler(void){
	SCHEDULER_ISR_enter();

	uint32_t tim_flag;
	TIMER_GetFlag(TIM7, TIM_SR_UIF, &tim_flag);
	if (tim_flag) {
		TIMER_ClearFlag(TIM7, TIM_SR_UIF);

		_sys_tick_ms++;

		// Check which Tasks cycle time has expired
		for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
			if (_tasks[i].state != TaskDeleted && _tasks[i].state != TaskSuspended && _tasks[i].function != 0) {
				if ((_sys_tick_ms - _tasks[i].last_run_ms)
					>= _tasks[i].period_ms) {
					_tasks[i].last_run_ms = _sys_tick_ms;

					if (_tasks[i].state == TaskBlocked) {
						SCHEDULER_InitTaskStack(i);
					}

					_tasks[i].state = TaskReady;
				}
			}
		}

		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

		// Look for stats to be printed
		if (_scheduler_running
			&& (_sys_tick_ms - _last_print_tick >= 1000)) {
			_last_print_tick = _sys_tick_ms;
			_stats_pending = 1;
		}
	}
	SCHEDULER_ISR_exit();
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

SCHEDULER_Status_TypeDef SCHEDULER_Task_add(
	SCHEDULER_Task_Function_TypeDef pvTaskCode,
	const char* pcName,
	uint32_t period_ms,
	uint8_t priority,
	uint8_t* taskIndex){

	if (taskIndex == NULL)  { return SCHEDULER_ERR_NOT_FOUND;  }
	if (pvTaskCode == NULL) { return SCHEDULER_ERR_TASK_INVALID; }

	int slot = -1;
	for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
		if (_tasks[i].state == TaskDeleted) {
			slot = i;
			break;
		}
	}

	if (slot < 0) { return SCHEDULER_ERR_FULL; }

	_tasks[slot].function	    = pvTaskCode;
	_tasks[slot].period_ms	    = period_ms;
	_tasks[slot].last_run_ms    = _sys_tick_ms;
	_tasks[slot].priority      = priority;
	_tasks[slot].pcName        = pcName;

	SCHEDULER_InitTaskStack(slot);

	_tasks[slot].state		    = TaskReady;

	*taskIndex = (uint8_t)slot;

	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_Task_remove(int taskIndex){
	if (taskIndex < 0 || taskIndex >= SCHEDULER_IDLE_TASK_INDEX) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	_tasks[taskIndex].state      = TaskDeleted;
	_tasks[taskIndex].function   = 0;
	_tasks[taskIndex].pcName     = 0;

	return SCHEDULER_OK;
}

void SCHEDULER_System_init(void){
	TIMER_Config(TIM7, 400, 999, 0);
	TIMER_EnableIT(TIM7);

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	uint32_t cnt = DWT->CYCCNT;
	_last_stats_print_cycle = cnt;
	_last_stats_tick = 0;
	for (int i = 0; i < SCHEDULER_MAX_TASKS; i++) {
		_task_stats[i].last_start_cycle = cnt;
		_task_stats[i].min_cycles = 0xFFFFFFFF;
		_task_stats[i].max_cycles = 0;
	}
}

SCHEDULER_Status_TypeDef SCHEDULER_Tick_get(uint32_t* tick){
	if (tick == 0) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	*tick = _sys_tick_ms;
	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_Task_suspend(uint8_t taskIndex){
	if (taskIndex >= SCHEDULER_IDLE_TASK_INDEX) {
		return SCHEDULER_ERR_NOT_FOUND;
	}
	if (_tasks[taskIndex].state == TaskDeleted) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	__disable_irq();
	_tasks[taskIndex].state = TaskSuspended;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_Task_resume(uint8_t taskIndex){
	if (taskIndex >= SCHEDULER_IDLE_TASK_INDEX) {
		return SCHEDULER_ERR_NOT_FOUND;
	}
	if (_tasks[taskIndex].state != TaskSuspended) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	__disable_irq();
	_tasks[taskIndex].state = TaskReady;
	_tasks[taskIndex].last_run_ms = _sys_tick_ms;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	return SCHEDULER_OK;
}

void SCHEDULER_Tasks_run(void){
	__disable_irq();

	_tasks[SCHEDULER_IDLE_TASK_INDEX].function       = SCHEDULER_IdleTask;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].period_ms      = 0;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].last_run_ms    = 0;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].priority       = 0xFF;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].state          = TaskReady;

	SCHEDULER_InitTaskStack(SCHEDULER_IDLE_TASK_INDEX);
	_tasks[SCHEDULER_IDLE_TASK_INDEX].pcName = "Idle";

	NVIC_SetPriority(PendSV_IRQn, 0xFF);
	NVIC_SetPriority(SVCall_IRQn, 0x00);
	NVIC_SetPriority(TIM7_IRQn, 0x80);

	FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;

	_current_task = SCHEDULER_SelectNextTask();

	// Enable ALL fault handlers so no crash goes undiagnosed
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk
				| SCB_SHCSR_BUSFAULTENA_Msk
				| SCB_SHCSR_MEMFAULTENA_Msk;

	// TIM7 is NOT started here - it starts in SCHEDULER_StartTick()
	// called from SVC_Handler after PSP and CONTROL are valid.
	// This prevents PendSV from ever firing before PSP is initialized.

	__enable_irq();

	__asm volatile ("SVC #0" : : : "memory");

	while (1) {
		__WFI();
	}
}

// -------------------------------------------------------------------------
// Debug API
// -------------------------------------------------------------------------

SCHEDULER_Status_TypeDef SCHEDULER_GetCurrentTask(int* taskIndex){
	if (taskIndex == 0) { return SCHEDULER_ERR_NOT_FOUND; }
	*taskIndex = _current_task;
	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_GetLastFault(
	volatile const Scheduler_Fault_Dump_TypeDef** dump){

	if (dump == 0) { return SCHEDULER_ERR_NOT_FOUND; }
	*dump = &g_sched_fault;
	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_GetTaskName(uint8_t task,
	const char** name){

	if (task >= SCHEDULER_MAX_TASKS) { return SCHEDULER_ERR_NOT_FOUND; }
	if (name == 0)                   { return SCHEDULER_ERR_NOT_FOUND; }
	*name = _tasks[task].pcName;
	return SCHEDULER_OK;
}

// -------------------------------------------------------------------------
// Fault handler - C core (captures state, then halts)
// -------------------------------------------------------------------------

/**
 * @brief Central fault handler - capture diagnostic state and halt
 *
 * @param [in] exc_return EXC_RETURN value from the fault context
 * @param [in] frame      Stack frame pointer (may be NULL)
 * @param [in] reason     Fault reason identifier
 */

void SCHEDULER_FaultHandler_C(uint32_t exc_return, uint32_t *frame,
	uint32_t reason){

	__disable_irq();

	g_sched_fault.magic   = SCHED_MAGIC;
	g_sched_fault.reason  = reason;
	g_sched_fault.task    = (uint32_t)_current_task;
	g_sched_fault.tick    = _sys_tick_ms;

	g_sched_fault.cfsr    = SCB->CFSR;
	g_sched_fault.hfsr    = SCB->HFSR;
	g_sched_fault.dfsr    = SCB->DFSR;
	g_sched_fault.afsr    = SCB->AFSR;
	g_sched_fault.mmfar   = SCB->MMFAR;
	g_sched_fault.bfar    = SCB->BFAR;
	g_sched_fault.icsr    = SCB->ICSR;
	g_sched_fault.shcsr   = SCB->SHCSR;

	g_sched_fault.msp     = __get_MSP();
	g_sched_fault.psp     = __get_PSP();
	g_sched_fault.psplim  = __get_PSPLIM();
	g_sched_fault.control = __get_CONTROL();
	g_sched_fault.exc_return = exc_return;

	if (frame) {
		g_sched_fault.r0  = frame[0];
		g_sched_fault.r1  = frame[1];
		g_sched_fault.r2  = frame[2];
		g_sched_fault.r3  = frame[3];
		g_sched_fault.r12 = frame[4];
		g_sched_fault.lr  = frame[5];
		g_sched_fault.pc  = frame[6];
		g_sched_fault.xpsr = frame[7];
	}

	char* reason_str;
	switch (reason) {
	case 1:
		reason_str = "Hard Fault (1)";
		break;
	case 2:
		reason_str = "MemManage Fault (2)";
		break;
	case 3:
		reason_str = "Bus Fault (3)";
		break;
	case 4:
		reason_str = "Usage Fault (4)";
		break;
	case 5:
		reason_str = "NMI Fault (5)";
		break;
	default:
		reason_str = "Unknown (?)";
		break;
	}

	DEBUG_PRINTF("\r\n===== SCHEDULER FAULT =====\r\n");
	DEBUG_PRINTF("Reason:%s Task:\"%s\"(%lu) Tick:%lu\r\n",
			reason_str,
			_tasks[_current_task].pcName ? _tasks[_current_task].pcName : "?",
			(uint32_t)_current_task,
			_sys_tick_ms);
	DEBUG_PRINTF("CFSR:0x%lx HFSR:0x%lx\r\n", g_sched_fault.cfsr, g_sched_fault.hfsr);
	DEBUG_PRINTF("DFSR:0x%lx AFSR:0x%lx\r\n", g_sched_fault.dfsr, g_sched_fault.afsr);
	DEBUG_PRINTF("MMFAR:0x%lx BFAR:0x%lx\r\n", g_sched_fault.mmfar, g_sched_fault.bfar);
	DEBUG_PRINTF("ICSR:0x%lx SHCSR:0x%lx\r\n", g_sched_fault.icsr, g_sched_fault.shcsr);
	DEBUG_PRINTF("MSP:0x%lx PSP:0x%lx PSPLIM:0x%lx\r\n", g_sched_fault.msp, g_sched_fault.psp, g_sched_fault.psplim);
	DEBUG_PRINTF("CONTROL:0x%lx EXC_RETURN:0x%lx\r\n", g_sched_fault.control, g_sched_fault.exc_return);

	if (frame) {
		DEBUG_PRINTF("R0:0x%lx R1:0x%lx R2:0x%lx R3:0x%lx\r\n",
			g_sched_fault.r0, g_sched_fault.r1,
			g_sched_fault.r2, g_sched_fault.r3);
		DEBUG_PRINTF("R12:0x%lx LR:0x%lx PC:0x%lx xPSR:0x%lx\r\n",
			g_sched_fault.r12, g_sched_fault.lr,
			g_sched_fault.pc, g_sched_fault.xpsr);
	}

	__BKPT(0);

	while (1) { __NOP(); }
}

// -------------------------------------------------------------------------
// Fault handlers - assembly stubs (tail-call C core)
// -------------------------------------------------------------------------

/**
 * @brief NMI handler - external / RCC clock loss / power failure
 */
__attribute__((naked)) void NMI_Handler(void){
	__asm volatile (
		"mov r0, lr\n"
		"tst r0, #4\n"
		"ite eq\n"
		"mrseq r1, msp\n"
		"mrsne r1, psp\n"
		"mov r2, #5\n"
		"b SCHEDULER_FaultHandler_C\n"
	);
}

/**
 * @brief HardFault handler - escalation from BusFault/MemManage or
 *        synchronous BusFault on unprivileged instruction fetch
 */
__attribute__((naked)) void HardFault_Handler(void){
	__asm volatile (
		"mov r0, lr\n"
		"tst r0, #4\n"
		"ite eq\n"
		"mrseq r1, msp\n"
		"mrsne r1, psp\n"
		"mov r2, #1\n"
		"b SCHEDULER_FaultHandler_C\n"
	);
}

/**
 * @brief MemManage handler - MPU violation
 */
__attribute__((naked)) void MemManage_Handler(void){
	__asm volatile (
		"mov r0, lr\n"
		"tst r0, #4\n"
		"ite eq\n"
		"mrseq r1, msp\n"
		"mrsne r1, psp\n"
		"mov r2, #2\n"
		"b SCHEDULER_FaultHandler_C\n"
	);
}

/**
 * @brief BusFault handler - memory transaction error (precise / imprecise
 *        data or instruction fetch), including stack-push to invalid address
 */
__attribute__((naked)) void BusFault_Handler(void){
	__asm volatile (
		"mov r0, lr\n"
		"tst r0, #4\n"
		"ite eq\n"
		"mrseq r1, msp\n"
		"mrsne r1, psp\n"
		"mov r2, #3\n"
		"b SCHEDULER_FaultHandler_C\n"
	);
}

/**
 * @brief UsageFault handler - stack overflow, undefined instruction,
 *        unaligned access, divide-by-zero
 */
__attribute__((naked)) void UsageFault_Handler(void){
	__asm volatile (
		"mov r0, lr\n"
		"tst r0, #4\n"
		"ite eq\n"
		"mrseq r1, msp\n"
		"mrsne r1, psp\n"
		"mov r2, #4\n"
		"b SCHEDULER_FaultHandler_C\n"
	);
}
