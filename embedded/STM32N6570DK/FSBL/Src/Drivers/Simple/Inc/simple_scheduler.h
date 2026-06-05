/**
 * @file    simple_scheduler.h
 *
 * @note    The tick source is TIM7 (1 ms period).  VTOR **must** be configured
 *          before the first interrupt fires — this is handled inside
 *          SCHEDULER_Init().
 *
 * @author  Groß
 * @date    May 24, 2026
 */

/** Example Use
static void vBackgroundTask(void) {
	switch (bg_color_state) {
		case 0: LCD_SetBackgroundColor(255, 0, 0); break;
		case 1: LCD_SetBackgroundColor(0, 255, 0); break;
		case 2: LCD_SetBackgroundColor(0, 0, 255); break;
	}
	bg_color_state = (bg_color_state + 1) % 3;
}

int main(void){
	delay_init();

	LCD_Init();

	SCHEDULER_Init();

	SCHEDULER_AddTask(vBackgroundTask, "BgColor", 500);

	while (1) {
		SCHEDULER_Run();
	}
}

 */

#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_MAX_TASKS 10

/**
 * @brief Task entry-point type.
 * @note  A task is a void-void function that returns when it has finished
 */
typedef void (*SCHEDULER_TaskFunction)(void);

/**
 * @brief Runtime descriptor for one scheduled task.
 */
typedef struct {
    SCHEDULER_TaskFunction function;    /**< Pointer to the task function             */
    uint32_t               period_ms;   /**< Nominal period in milliseconds           */
    uint32_t               last_run_ms; /**< Tick stamp of the most recent execution  */
    uint8_t                active;      /**< 1 = enabled, 0 = disabled / deleted      */
} SCHEDULER_TaskHandle;

/**
 * @brief Return codes for SCHEDULER_AddTask / SCHEDULER_RemoveTask.
 */
typedef enum {
    SCHEDULER_OK         =  0, 		/**< Operation succeeded                                       */
    SCHEDULER_ERR_FULL   = -1, 		/**< Task table is full (SCHEDULER_MAX_TASKS reached)           */
    SCHEDULER_ERR_NOT_FOUND = -2 	/**< The supplied task index does not exist or is invalid      */
} SCHEDULER_Status_t;

/**
 * @brief  Register a new periodic task
 * @param  pvTaskCode  Pointer to the task function
 * @param  pcName      Human-readable label (unused)
 * @param  period_ms   Interval between executions in ms
 * @return On success the assigned task index (0 ... SCHEDULER_MAX_TASKS-1);
 *         SCHEDULER_ERR_FULL if the table is full.
 */
int SCHEDULER_AddTask(SCHEDULER_TaskFunction pvTaskCode, const char *pcName, uint32_t period_ms);

/**
 * @brief  Remove a task from the schedule
 * @param  taskIndex  Index returned by SCHEDULER_AddTask()
 * @return SCHEDULER_OK on success, SCHEDULER_ERR_NOT_FOUND otherwise
 * @note   Does NOT reclaim the slot — a linear scan is avoided to keep the
 *         scheduler O(n) in the run loop.  Deleted tasks are skipped at
 *         run-time via the active flag
 */
int SCHEDULER_RemoveTask(int taskIndex);

/**
 * @brief  Initialise the scheduler tick (TIM7, 1 ms interrupt)
 * @note   Must be called once before any SCHEDULER_AddTask() or SCHEDULER_Run()
 *         Also configures VTOR if it has not been set yet
 */
void SCHEDULER_Init(void);

/**
 * @brief  Read the current system tick count.
 * @return Monotonically increasing millisecond counter.
 */
uint32_t SCHEDULER_GetTick(void);

/**
 * @brief  Execute pending tasks (call inside main loop)
 */
void SCHEDULER_Run(void);

#endif /* SIMPLE_SCHEDULER_H */
