/**
 * @file    simple_scheduler.h
 * @author  Gross
 * @date    05.06.2026
 * @brief   Priority preemptive scheduler driver source
 */

#include <stdint.h>
#include <stddef.h>

#include "simple_scheduler.h"

#include "stm32n657xx.h"
#include "simple_timer.h"

// -------------------------------------------------------------------------
// Private defines / datatypes
// -------------------------------------------------------------------------

typedef struct {
	SCHEDULER_TaskFunction_TypeDef	function;			//  0
	uint32_t						period_ms;			//  4
	uint32_t						last_run_ms;		//  8
	uint8_t							priority;			// 12
	uint8_t							ready;				// 13
	uint8_t							active;				// 14
	uint8_t							needs_init;			// 15
	uint8_t							stack_overflow;		// 16
	uint32_t						saved_sp;			// 20 (aligned)
	uint32_t						saved_exc_return;	// 24
	const char*						pcName;				// 28
} _TaskHandle_TypeDef;							// 32 bytes

// TCB = Task Control Block
#define _TCB_SIZE		32
#define _TCB_SAVED_SP	20
#define _TCB_EXC_RETURN	24
#define _TCB_NEEDS_INIT	15

// Stringify helper for inline assembly
#define _STR_HELPER(x) #x
#define _STR(x)        _STR_HELPER(x)

_Static_assert(offsetof(_TaskHandle_TypeDef, saved_sp) == _TCB_SAVED_SP, "TCB saved_sp offset mismatch with assembly");
_Static_assert(offsetof(_TaskHandle_TypeDef, saved_exc_return) == _TCB_EXC_RETURN, "TCB saved_exc_return offset mismatch with assembly");
_Static_assert(offsetof(_TaskHandle_TypeDef, needs_init) == _TCB_NEEDS_INIT, "TCB needs_init offset mismatch with assembly");
_Static_assert(sizeof(_TaskHandle_TypeDef) == _TCB_SIZE, "TCB size mismatch with assembly");

// -------------------------------------------------------------------------
// Private data
// -------------------------------------------------------------------------

static _TaskHandle_TypeDef	_tasks[SCHEDULER_MAX_TASKS];
static volatile uint32_t	_sys_tick_ms = 0;
static int					_current_task = 0;

static uint32_t _task_stacks[SCHEDULER_MAX_TASKS][SCHEDULER_DEFAULT_STACK_SIZE] __attribute__((aligned(8)));

_Static_assert(sizeof(_task_stacks[0]) == SCHEDULER_STACK_SIZE_BYTES,"SCHEDULER_STACK_SIZE_BYTES mismatch");

volatile Scheduler_Fault_Dump_TypeDef g_sched_fault;

// Forward declarations (called from assembly)
void SCHEDULER_Task_exit(void);
static int SCHEDULER_SelectNextTask(void);
void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr);
void SCHEDULER_FaultHandler_C(uint32_t exc_return, uint32_t *frame, uint32_t reason);
void SCHEDULER_StartTick(void);

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
	uint32_t *sp = stack_end - 32;

	// sp[0..15]  S16-S31  (FPU callee-saved)
	// sp[16..23] R4-R11   (core callee-saved)
	// sp[24..31] exception frame: R0-R3, R12, LR, PC, xPSR

	sp[29] = (uint32_t)SCHEDULER_Task_exit; // LR on task return
	sp[30] = (uint32_t)_tasks[i].function;  // PC
	sp[31] = 0x01000000UL;                  // xPSR (thumb bit)

	_tasks[i].saved_sp         = (uint32_t)&sp[16];
	_tasks[i].saved_exc_return = 0xFFFFFFFDUL;
	_tasks[i].needs_init       = 0;
}

// -------------------------------------------------------------------------
// Idle task
// -------------------------------------------------------------------------

/**
 * @brief Idle task - waits for the next interrupt
 */
__attribute__((noreturn)) static void SCHEDULER_IdleTask(void){
	while (1) {
		__NOP();
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
	_tasks[_current_task].ready      = 0;
	_tasks[_current_task].needs_init = 1;

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	while (1) {
		__NOP();
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

		if (_tasks[i].active && _tasks[i].ready) {
			if (_tasks[i].priority < best_prio) {
				best_prio = _tasks[i].priority;
				best = i;
			}
		}
	}

	if (best < 0) {
		best = SCHEDULER_IDLE_TASK_INDEX;
	}

	return best;
}

/**
 * @brief Reinizializes Task
 *
 * @param [in] task_idx | Task index
 * @param [in] tcb_addr | TaskCodeBlock adress
 */
void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr){
	SCHEDULER_InitTaskStack(task_idx);
	((_TaskHandle_TypeDef*)tcb_addr)->needs_init = 0;
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

		"ldrb	r1, [r4, #" _STR(_TCB_NEEDS_INIT) "]\n"
		"cmp	r1, #0\n"
		"beq	1f\n"

		"push	{r1, lr}\n"
		"mov	r0, r4\n"
		"ldr	r1, =_tasks\n"
		"sub	r0, r0, r1\n"
		"mov	r1, #" _STR(_TCB_SIZE) "\n"
		"udiv	r0, r0, r1\n"
		"mov	r1, r4\n"
		"bl		SCHEDULER_ReinitTask\n"
		"pop	{r1, lr}\n"

		"ldr	r4, =_tasks\n"
		"ldr	r2, =_current_task\n"
		"ldr	r3, [r2]\n"
		"mov	r5, #" _STR(_TCB_SIZE) "\n"
		"mul	r3, r3, r5\n"
		"add	r4, r4, r3\n"

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
}

/**
 * @brief TIM7 interrupt handler - scheduler tick
 *
 * Increments the system tick counter and marks tasks ready when
 * their period has elapsed, then pends PendSV for context switch.
 */
void TIM7_IRQHandler(void){
	uint32_t tim_flag;
	TIMER_GetFlag(TIM7, TIM_SR_UIF, &tim_flag);
	if (tim_flag) {
		TIMER_ClearFlag(TIM7, TIM_SR_UIF);

		_sys_tick_ms++;

		for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
			if (_tasks[i].active && _tasks[i].function != 0) {
				if ((_sys_tick_ms - _tasks[i].last_run_ms)
					>= _tasks[i].period_ms) {
					_tasks[i].last_run_ms = _sys_tick_ms;
					_tasks[i].ready       = 1;
				}
			}
		}

		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

SCHEDULER_Status_TypeDef SCHEDULER_Task_add(
	SCHEDULER_TaskFunction_TypeDef pvTaskCode,
	const char* pcName,
	uint32_t period_ms,
	uint8_t priority,
	uint8_t* taskIndex){

	if (taskIndex == NULL)  { return SCHEDULER_ERR_NOT_FOUND;  }
	if (pvTaskCode == NULL) { return SCHEDULER_ERR_TASK_INVALID; }

	int slot = -1;
	for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
		if (!_tasks[i].active) {
			slot = i;
			break;
		}
	}

	if (slot < 0) { return SCHEDULER_ERR_FULL; }

	_tasks[slot].function	    = pvTaskCode;
	_tasks[slot].period_ms	    = period_ms;
	_tasks[slot].last_run_ms    = _sys_tick_ms;
	_tasks[slot].priority	    = priority;
	_tasks[slot].ready		    = 1;
	_tasks[slot].active		    = 1;
	_tasks[slot].needs_init     = 0;
	_tasks[slot].stack_overflow = 0;
	_tasks[slot].pcName         = pcName;

	SCHEDULER_InitTaskStack(slot);

	*taskIndex = (uint8_t)slot;

	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_Task_remove(int taskIndex){
	if (taskIndex < 0 || taskIndex >= SCHEDULER_IDLE_TASK_INDEX) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	_tasks[taskIndex].active     = 0;
	_tasks[taskIndex].function   = 0;
	_tasks[taskIndex].ready      = 0;
	_tasks[taskIndex].needs_init = 0;
	_tasks[taskIndex].pcName     = 0;

	return SCHEDULER_OK;
}

void SCHEDULER_System_init(void){
	TIMER_Config(TIM7, 200, 999, 0);
	TIMER_EnableIT(TIM7);
}

SCHEDULER_Status_TypeDef SCHEDULER_Tick_get(uint32_t* tick){
	if (tick == 0) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	*tick = _sys_tick_ms;
	return SCHEDULER_OK;
}

void SCHEDULER_Tasks_run(void){
	__disable_irq();

	_tasks[SCHEDULER_IDLE_TASK_INDEX].function       = SCHEDULER_IdleTask;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].period_ms      = 0;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].last_run_ms    = 0;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].priority       = 0xFF;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].ready          = 1;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].active         = 1;
	_tasks[SCHEDULER_IDLE_TASK_INDEX].needs_init     = 0;

	SCHEDULER_InitTaskStack(SCHEDULER_IDLE_TASK_INDEX);

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
		__NOP();
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

	if (reason == 4 && (SCB->CFSR & SCB_CFSR_STKOF_Msk)) {
		_tasks[_current_task].stack_overflow = 1;
	}

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
 * @brief UsageFault handler – stack overflow, undefined instruction,
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
