/*
 * simple_scheduler.c
 *
 * Priority preemptive scheduler on TIM7 (1 ms tick).  The TIM7 ISR marks
 * tasks ready and pends PendSV.  The PendSV handler performs a full context
 * switch (auto + callee-saved registers, including FPU if active).  SVC is
 * used to start the first task.  Idle task runs when no user task is ready.
 * Tick-count wrap-around is handled implicitly by unsigned 32-bit subtraction
 * - periods up to 2^31 ms (~ 24.9 days) are safe.
 * Lower priority values = higher priority.
 *
 * @author  Groß
 * @date    May 24, 2026
 */

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

#include "simple_scheduler.h"
#include "simple_timer.h"

// -------------------------------------------------------------------------
// Private defines/datatypes
// -------------------------------------------------------------------------

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
} _TaskHandle_TypeDef;							// 24 bytes

// TCB = Task Control Block
#define _TCB_SIZE		24
#define _TCB_SAVED_SP	16
#define _TCB_EXC_RETURN	20
#define _TCB_NEEDS_INIT	15

// Stringify helper for inline assembly
#define _STR_HELPER(x) #x
#define _STR(x)        _STR_HELPER(x)

// -------------------------------------------------------------------------
// Private data
// -------------------------------------------------------------------------

static _TaskHandle_TypeDef	_tasks[SCHEDULER_MAX_TASKS];
static volatile uint32_t	_sys_tick_ms = 0;
static int					_current_task = 0;

static uint32_t _task_stacks[SCHEDULER_MAX_TASKS][SCHEDULER_DEFAULT_STACK_SIZE]
							__attribute__((aligned(8)));

// Forward declarations (called from assembly)
void SCHEDULER_Task_exit(void);
static int SCHEDULER_SelectNextTask(void);
void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr);

// -------------------------------------------------------------------------
// Stack initialisation
// -------------------------------------------------------------------------

// Return type is the exception because of use in Assembly
static void SCHEDULER_InitTaskStack(int i){
    // Align to 8 bytes, get end address of the stack area
    uint32_t *stack_end = (uint32_t *)(((uint32_t)_task_stacks[i]
        + sizeof(_task_stacks[i])) & ~7U);

    // Reserve 32 words: 16 for FP regs + 8 callee-saved + 8 exception frame
    uint32_t *sp = stack_end - 32;

    for (int j = 0; j < 32; j++) {
        sp[j] = 0;
    }

    // sp[0..15]  S16-S31  (FPU callee-saved, restored when FPU active)
    // sp[16..23] R4-R11   (core callee-saved)
    // sp[24..31] hardware exception frame (R0-R3, R12, LR, PC, xPSR)

    // Exception frame layout (words 24-31, pushed by CPU on exception entry)
    sp[29] = (uint32_t)SCHEDULER_Task_exit; // LR  – return address on task exit
    sp[30] = (uint32_t)_tasks[i].function;  // PC  – first instruction to execute
    sp[31] = 0x01000000UL;                  // xPSR – thumb bit must be set

    // saved_sp points past the callee-saved block (r4-r11),
    // so PendSV restores r4-r11 then PSP points at the exception frame
    _tasks[i].saved_sp         = (uint32_t)&sp[16];
    // EXC_RETURN = 0xFFFFFFFD: return to thread mode, use PSP, FPU not active
    _tasks[i].saved_exc_return = 0xFFFFFFFDUL;
    _tasks[i].needs_init       = 0;
}

// -------------------------------------------------------------------------
// Idle task
// -------------------------------------------------------------------------

__attribute__((noreturn)) static void SCHEDULER_IdleTask(void){
	// Waiting for interrupt
	while (1) {
		__WFI();
	}
}

// -------------------------------------------------------------------------
// Task exit handler
// -------------------------------------------------------------------------

void SCHEDULER_Task_exit(void){
	// Disable interrupts while modifying shared state
	__disable_irq();
	// Mark the task as finished - PendSV will re-init it when rescheduled
	_tasks[_current_task].ready      = 0;
	_tasks[_current_task].needs_init = 1;

	// Trigger a context switch so PendSV picks the next ready task
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	// Park this core - PendSV will re-init our stack frame before
	// Exit to interrupt
	while (1) {
		__WFI();
	}
}

// -------------------------------------------------------------------------
// Schedule - pick the highest-priority ready task
// -------------------------------------------------------------------------

static int SCHEDULER_SelectNextTask(void){
	// Scan all user tasks for the highest-priority ready task (Schedule Algorithm)
	// (lower priority value = higher priority)
	int     best      = -1;
	uint8_t best_prio = 0xFF;

	for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
		if (_tasks[i].active && _tasks[i].ready) {
			if (_tasks[i].priority < best_prio) {
				best_prio = _tasks[i].priority;
				best      = i;
			}
		}
	}

	// If no user task is ready, fall back to the idle task
	if (best < 0) {
		best = SCHEDULER_IDLE_TASK_INDEX;
	}
	return best;
}

// -------------------------------------------------------------------------
// Helper - called from PendSV assembly to re-init a finished task
// -------------------------------------------------------------------------

void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr){
	// Re-initialise the finished task's stack as if it were freshly added
	SCHEDULER_InitTaskStack(task_idx);
	// Mark it ready so it will be scheduled again on its next period
	((_TaskHandle_TypeDef*)tcb_addr)->ready      = 1;
	((_TaskHandle_TypeDef*)tcb_addr)->needs_init = 0;
}

// -------------------------------------------------------------------------
// PendSV handler - preemptive context switch
// -------------------------------------------------------------------------

__attribute__((naked)) void PendSV_Handler(void){
	// Preemptive context switch – saves the current task's context,
	// selects the next ready task, and restores its context
	// Uses PSP as the task stack pointer
	__asm volatile (
		// -- Save current task context --
		"cpsid	i\n"                        // disable IRQs during switch

		"mrs	r0, psp\n"                  // r0 = current task's PSP
		"stmdb	r0!, {r4-r11}\n"            // push callee-saved regs r4-r11

		// If the interrupted code used the FPU, preserve S16-S31 as well
		"tst	lr, #0x10\n"                // check FPCA bit in EXC_RETURN
		"it		eq\n"
		"vstmdbeq r0!, {s16-s31}\n"

		// Locate the current task's TCB
		"ldr	r2, =_current_task\n"       // r2 = &_current_task
		"ldr	r3, [r2]\n"                 // r3 = current index
		"ldr	r4, =_tasks\n"              // r4 = base of _tasks array

		"mov	r5, #" _STR(_TCB_SIZE) "\n"   // r5 = sizeof(TCB)
		"mul	r3, r3, r5\n"               // r3 = index * sizeof(TCB)
		"add	r4, r4, r3\n"               // r4 = &_tasks[current]
		"str	r0, [r4, #" _STR(_TCB_SAVED_SP) "]\n"     // save PSP
		"str	lr, [r4, #" _STR(_TCB_EXC_RETURN) "]\n"  // save EXC_RETURN

		// -- Select next task to run --
		"push	{lr}\n"                     // preserve EXC_RETURN on main stack
		"bl		SCHEDULER_SelectNextTask\n" // returns next index in r0
		"pop	{lr}\n"                     // restore EXC_RETURN

		// Update current task index
		"ldr	r2, =_current_task\n"
		"str	r0, [r2]\n"                 // _current_task = next index

		// Locate the next task's TCB
		"ldr	r4, =_tasks\n"
		"mov	r5, #" _STR(_TCB_SIZE) "\n"
		"mul	r0, r0, r5\n"
		"add	r4, r4, r0\n"               // r4 = &_tasks[next]

		// -- Re-init the task's stack if it previously exited --
		"ldrb	r1, [r4, #" _STR(_TCB_NEEDS_INIT) "]\n"
		"cmp	r1, #0\n"
		"beq	1f\n"                       // skip if not flagged

		// needs_init is set: rebuild the exception frame
		"push	{lr}\n"
		"mov	r0, r4\n"                   // r0 = TCB address
		"ldr	r1, =_tasks\n"
		"sub	r0, r0, r1\n"               // r0 = byte offset into _tasks[]
		"mov	r1, #" _STR(_TCB_SIZE) "\n"
		"udiv	r0, r0, r1\n"               // r0 = task index
		"mov	r1, r4\n"                   // r1 = TCB address
		"bl		SCHEDULER_ReinitTask\n"
		"pop	{lr}\n"

		// Reload next task's TCB base (ReinitTask may have changed saved_sp)
		"ldr	r4, =_tasks\n"
		"ldr	r2, =_current_task\n"
		"ldr	r3, [r2]\n"
		"mov	r5, #" _STR(_TCB_SIZE) "\n"
		"mul	r3, r3, r5\n"
		"add	r4, r4, r3\n"

		// -- Restore next task context --
		"1:\n"
		"ldr	r0, [r4, #" _STR(_TCB_SAVED_SP) "]\n"     // r0 = saved PSP
		"ldr	lr, [r4, #" _STR(_TCB_EXC_RETURN) "]\n"  // lr = saved EXC_RETURN

		// Restore FPU callee-saved regs if the task uses the FPU
		"tst	lr, #0x10\n"
		"it		eq\n"
		"vldmiaeq r0!, {s16-s31}\n"

		"ldmia	r0!, {r4-r11}\n"            // restore r4-r11
		"msr	psp, r0\n"                  // PSP = updated stack pointer

		"cpsie	i\n"                        // re-enable IRQs
		"bx		lr\n"                       // return to the restored task
	);
}

// -------------------------------------------------------------------------
// SVC handler - start first task
// -------------------------------------------------------------------------

__attribute__((naked)) void SVC_Handler(void){
    // First-task startup: invoked by "SVC #0" in SCHEDULER_Tasks_run
    // Loads the initial task's context and switches to thread mode using PSP
    __asm volatile (
        // Locate the first scheduled task's TCB
        "ldr    r0, =_current_task                  \n"
        "ldr    r1, [r0]                            \n"  // r1 = _current_task index

        "ldr    r2, =_tasks                         \n"
        "mov    r3, #" _STR(_TCB_SIZE) "             \n"
        "mul    r1, r1, r3                          \n"
        "add    r2, r2, r1                          \n"  // r2 = &_tasks[current]

        // Restore callee-saved registers and set PSP
        "ldr    r0, [r2, #" _STR(_TCB_SAVED_SP) "]    \n"  // r0 = saved PSP
        "ldmia  r0!, {r4-r11}                       \n"  // pop r4-r11
        "msr    psp, r0                             \n"  // PSP past callee-saved
        "isb                                        \n"

        // Switch to thread mode + PSP (bit 1 = 1) + no FP active (bit 2 = 0)
        "movs   r0, #2                              \n"  // CONTROL = 2
        "msr    control, r0                         \n"
        "isb                                        \n"

        // EXC_RETURN = 0xFFFFFFFD: return to thread mode, use PSP
        "ldr    lr, =0xFFFFFFFD                     \n"
        "bx     lr                                  \n"  // exception return -> first task
    );
}

// -------------------------------------------------------------------------
// TIM7 ISR - tick source
// -------------------------------------------------------------------------

void TIM7_IRQHandler(void){
	// 1 ms periodic tick – check and clear the update flag
	if (TIM_GetFlag(TIM7, TIM_SR_UIF)) {
		TIM_ClearFlag(TIM7, TIM_SR_UIF);

		// Advance the system tick counter
		_sys_tick_ms++;

		// Mark any task whose period has elapsed as ready
		for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
			if (_tasks[i].active && _tasks[i].function != 0) {
				if ((_sys_tick_ms - _tasks[i].last_run_ms)
					>= _tasks[i].period_ms) {
					_tasks[i].last_run_ms = _sys_tick_ms;
					_tasks[i].ready       = 1;
				}
			}
		}

		// Pend PendSV – the actual context switch runs as the lowest-priority
		// exception, so it only fires after we return from this ISR
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}

// -------------------------------------------------------------------------
// SCHEDULER - API
// -------------------------------------------------------------------------

SCHEDULER_Status_TypeDef SCHEDULER_Task_add(
	SCHEDULER_TaskFunction_TypeDef pvTaskCode,
	const char* pcName,
	uint32_t period_ms,
	uint8_t priority,
	uint8_t* taskIndex){

	// Validate
	if (taskIndex == NULL)  { return SCHEDULER_ERR_NOT_FOUND; 	 }	// output pointer
	if (pvTaskCode == NULL) { return SCHEDULER_ERR_TASK_INVALID; }	// TaskFunction

	// Find the first free slot
	int slot = -1;
	for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
		if (!_tasks[i].active) {
			slot = i;
			break;
		}
	}

	// Validate
	if (slot < 0) { return SCHEDULER_ERR_FULL;	}	// slot

	// Populate the TCB
	_tasks[slot].function	  = pvTaskCode;
	_tasks[slot].period_ms	  = period_ms;
	_tasks[slot].last_run_ms  = _sys_tick_ms;
	_tasks[slot].priority	  = priority;
	_tasks[slot].ready		  = 1;
	_tasks[slot].active		  = 1;
	_tasks[slot].needs_init   = 0;

	// Set up the initial stack frame for this task
	SCHEDULER_InitTaskStack(slot);

	// TODO: USART out for debug purposes
	(void)pcName;

	// Set return values
	*taskIndex = (uint8_t)slot;

	return SCHEDULER_OK;
}

SCHEDULER_Status_TypeDef SCHEDULER_Task_remove(int taskIndex){
	// Validate that the slot is within the user-task range (not the idle slot)
	if (taskIndex < 0 || taskIndex >= SCHEDULER_IDLE_TASK_INDEX) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	// Mark the slot as inactive - the scheduler will ignore it
	_tasks[taskIndex].active     = 0;
	_tasks[taskIndex].function   = 0;
	_tasks[taskIndex].ready      = 0;
	_tasks[taskIndex].needs_init = 0;

	return SCHEDULER_OK;
}

void SCHEDULER_System_init(void){
	// Configure TIM7 as a 1 ms tick source:
	//   HCLK = 200 MHz -> PSC = 200 -> 1 MHz counter -> ARR = 999 -> 1 kHz
	// This call does NOT enable the NVIC – that happens later in Tasks_run()
	// after interrupt priorities have been configured.
	TIM_Config(TIM7, 200, 999, 0);
	// Enable the update interrupt (UIE bit in DIER)
	TIM_EnableIT(TIM7);
}

SCHEDULER_Status_TypeDef SCHEDULER_Tick_get(uint32_t* tick){
	if (tick == 0) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	*tick = _sys_tick_ms;
	return SCHEDULER_OK;
}

// -------------------------------------------------------------------------
// SCHEDULER_Tasks_run - start the preemptive scheduler
// -------------------------------------------------------------------------

void SCHEDULER_Tasks_run(void){
    // Disable IRQs during scheduler initialisation
    __disable_irq();

    // Set up the idle task in the last TCB slot
    _tasks[SCHEDULER_IDLE_TASK_INDEX].function       = SCHEDULER_IdleTask;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].period_ms      = 0;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].last_run_ms    = 0;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].priority       = 0xFF;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].ready          = 1;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].active         = 1;
    _tasks[SCHEDULER_IDLE_TASK_INDEX].needs_init     = 0;

    SCHEDULER_InitTaskStack(SCHEDULER_IDLE_TASK_INDEX);

    // Configure exception priorities:
    //   PendSV = lowest (0xFF) – context switch only when nothing else pending
    //   SVC    = highest (0x00) – first-task startup must not be delayed
    //   TIM7   = mid     (0x80) – tick must preempt user tasks but not SVC
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_SetPriority(SVCall_IRQn, 0x00);
    NVIC_SetPriority(TIM7_IRQn, 0x80);

    // Disable lazy FPU stacking – we handle S16-S31 explicitly in PendSV
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;

    // Select the first task to run
    _current_task = SCHEDULER_SelectNextTask();

    // Enable the TIM7 tick interrupt now that priorities are configured
    NVIC_ClearPendingIRQ(TIM7_IRQn);
    NVIC_EnableIRQ(TIM7_IRQn);
    TIM_Start(TIM7);

    __enable_irq();

    // SVC #0 triggers SVC_Handler, which loads the first task's context
    // and performs the exception return into it – we never return here
    __asm volatile ("SVC #0" : : : "memory");

    // SVC never returns – this is a safety catch
    while (1) {
        __WFI();
    }
}
