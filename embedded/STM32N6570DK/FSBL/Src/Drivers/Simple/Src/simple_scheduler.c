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

#include "simple_scheduler.h"
#include "simple_rcc.h"
#include "stm32n657xx.h"

/* Stringify helper for inline assembly */
#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

/* --------------------------------------------------------------------------
 * Private data
 * -------------------------------------------------------------------------- */

static SCHEDULER_TaskHandle_TypeDef	_tasks[SCHEDULER_MAX_TASKS];
static volatile uint32_t			_sys_tick_ms = 0;
static int							_current_task = 0;

static uint32_t _task_stacks[SCHEDULER_MAX_TASKS][SCHEDULER_DEFAULT_STACK_SIZE]
							__attribute__((aligned(8)));

/* Forward declarations (called from assembly) */
void SCHEDULER_Task_exit(void);
static int SCHEDULER_SelectNextTask(void);
void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr);

/* --------------------------------------------------------------------------
 * Stack initialisation
 * -------------------------------------------------------------------------- */

static void SCHEDULER_InitTaskStack(int i)
{
    uint32_t *stack_end = (uint32_t *)(((uint32_t)_task_stacks[i]
        + sizeof(_task_stacks[i])) & ~7U);

    uint32_t *sp = stack_end - 32;

    for (int j = 0; j < 32; j++) {
        sp[j] = 0;
    }

    /* sp[0..15]  reserved FP area */
    /* sp[16..23] r4-r11 */
    /* sp[24..31] hardware exception frame */

    sp[29] = (uint32_t)SCHEDULER_Task_exit; /* LR */
    sp[30] = (uint32_t)_tasks[i].function;  /* PC */
    sp[31] = 0x01000000UL;                  /* xPSR */

    _tasks[i].saved_sp         = (uint32_t)&sp[16];
    _tasks[i].saved_exc_return = 0xFFFFFFFDUL;
    _tasks[i].needs_init       = 0;
}

/* --------------------------------------------------------------------------
 * Idle task
 * -------------------------------------------------------------------------- */

__attribute__((noreturn)) static void SCHEDULER_IdleTask(void){
	while (1) {
		__WFI();
	}
}

/* --------------------------------------------------------------------------
 * Task exit handler
 * -------------------------------------------------------------------------- */

void SCHEDULER_Task_exit(void){
	__disable_irq();
	_tasks[_current_task].ready      = 0;
	_tasks[_current_task].needs_init = 1;

	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__enable_irq();

	while (1) {
		__WFI();
	}
}

/* --------------------------------------------------------------------------
 * Schedule - pick the highest-priority ready task
 * -------------------------------------------------------------------------- */

static int SCHEDULER_SelectNextTask(void){
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

	if (best < 0) {
		best = SCHEDULER_IDLE_TASK_INDEX;
	}
	return best;
}

/* --------------------------------------------------------------------------
 * Helper - called from PendSV assembly to re-init a finished task
 * -------------------------------------------------------------------------- */

void SCHEDULER_ReinitTask(int task_idx, uint32_t tcb_addr){
	SCHEDULER_InitTaskStack(task_idx);
	((SCHEDULER_TaskHandle_TypeDef*)tcb_addr)->ready      = 1;
	((SCHEDULER_TaskHandle_TypeDef*)tcb_addr)->needs_init = 0;
}

/* --------------------------------------------------------------------------
 * PendSV handler - preemptive context switch
 * -------------------------------------------------------------------------- */

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

		"mov	r5, #" STR(TCB_SIZE) "\n"
		"mul	r3, r3, r5\n"
		"add	r4, r4, r3\n"
		"str	r0, [r4, #" STR(TCB_SAVED_SP) "]\n"
		"str	lr, [r4, #" STR(TCB_EXC_RETURN) "]\n"

		"push	{lr}\n"
		"bl		SCHEDULER_SelectNextTask\n"
		"pop	{lr}\n"

		"ldr	r2, =_current_task\n"
		"str	r0, [r2]\n"

		"ldr	r4, =_tasks\n"
		"mov	r5, #" STR(TCB_SIZE) "\n"
		"mul	r0, r0, r5\n"
		"add	r4, r4, r0\n"

		"ldrb	r1, [r4, #" STR(TCB_NEEDS_INIT) "]\n"
		"cmp	r1, #0\n"
		"beq	1f\n"

		"push	{lr}\n"
		"mov	r0, r4\n"
		"ldr	r1, =_tasks\n"
		"sub	r0, r0, r1\n"
		"mov	r1, #" STR(TCB_SIZE) "\n"
		"udiv	r0, r0, r1\n"
		"mov	r1, r4\n"
		"bl		SCHEDULER_ReinitTask\n"
		"pop	{lr}\n"

		"ldr	r4, =_tasks\n"
		"ldr	r2, =_current_task\n"
		"ldr	r3, [r2]\n"
		"mov	r5, #" STR(TCB_SIZE) "\n"
		"mul	r3, r3, r5\n"
		"add	r4, r4, r3\n"

		"1:\n"
		"ldr	r0, [r4, #" STR(TCB_SAVED_SP) "]\n"
		"ldr	lr, [r4, #" STR(TCB_EXC_RETURN) "]\n"

		"tst	lr, #0x10\n"
		"it		eq\n"
		"vldmiaeq r0!, {s16-s31}\n"

		"ldmia	r0!, {r4-r11}\n"
		"msr	psp, r0\n"

		"cpsie	i\n"
		"bx		lr\n"
	);
}

/* --------------------------------------------------------------------------
 * SVC handler - start first task
 * -------------------------------------------------------------------------- */

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile (
        "ldr    r0, =_current_task                  \n"
        "ldr    r1, [r0]                            \n"

        "ldr    r2, =_tasks                         \n"
        "mov    r3, #" STR(TCB_SIZE) "             \n"
        "mul    r1, r1, r3                          \n"
        "add    r2, r2, r1                          \n"

        "ldr    r0, [r2, #" STR(TCB_SAVED_SP) "]    \n"
        "ldmia  r0!, {r4-r11}                       \n"
        "msr    psp, r0                             \n"
        "isb                                        \n"

        "movs   r0, #2                              \n"
        "msr    control, r0                         \n"
        "isb                                        \n"

        "ldr    lr, =0xFFFFFFFD                     \n"
        "bx     lr                                  \n"
    );
}

/* --------------------------------------------------------------------------
 * TIM7 ISR - tick source
 * -------------------------------------------------------------------------- */

void TIM7_IRQHandler(void){
	if (TIM7->SR & TIM_SR_UIF) {
		TIM7->SR &= ~TIM_SR_UIF;

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

/* --------------------------------------------------------------------------
 * SCHEDULER - API
 * -------------------------------------------------------------------------- */

SCHEDULER_Status_TypeDef SCHEDULER_Task_add(
	SCHEDULER_TaskFunction_TypeDef pvTaskCode,
	const char* pcName,
	uint32_t period_ms,
	uint8_t priority,
	uint8_t* taskIndex){

	if (taskIndex == 0) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	int slot = -1;
	for (int i = 0; i < SCHEDULER_IDLE_TASK_INDEX; i++) {
		if (!_tasks[i].active) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		return SCHEDULER_ERR_FULL;
	}

	_tasks[slot].function	  = pvTaskCode;
	_tasks[slot].period_ms	  = period_ms;
	_tasks[slot].last_run_ms  = _sys_tick_ms;
	_tasks[slot].priority	  = priority;
	_tasks[slot].ready		  = 1;
	_tasks[slot].active		  = 1;
	_tasks[slot].needs_init   = 0;

	SCHEDULER_InitTaskStack(slot);

	(void)pcName;

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

	return SCHEDULER_OK;
}

void SCHEDULER_System_init(void){
	RCC_enable_TIM(TIM7);

	TIM7->CR1   = 0;
	TIM7->PSC   = 200 - 1;
	TIM7->ARR   = 999;
	TIM7->CNT   = 0;
	TIM7->DIER |= TIM_DIER_UIE;
	TIM7->EGR   = TIM_EGR_UG;
	TIM7->SR   &= ~TIM_SR_UIF;
}

SCHEDULER_Status_TypeDef SCHEDULER_Tick_get(uint32_t* tick){
	if (tick == 0) {
		return SCHEDULER_ERR_NOT_FOUND;
	}

	*tick = _sys_tick_ms;
	return SCHEDULER_OK;
}

/* --------------------------------------------------------------------------
 * SCHEDULER_Tasks_run - start the preemptive scheduler
 * -------------------------------------------------------------------------- */

void SCHEDULER_Tasks_run(void)
{
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

    NVIC_ClearPendingIRQ(TIM7_IRQn);
    NVIC_EnableIRQ(TIM7_IRQn);
    TIM7->CR1 |= TIM_CR1_CEN;

    __enable_irq();

    __asm volatile ("SVC #0" : : : "memory");

    while (1) {
        __WFI();
    }
}
