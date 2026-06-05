/**
 * @file    simple_scheduler.c
 * @brief   Cooperative round-robin scheduler on top of a TIM7 1 ms tick.
 * @note    Tick-count wrap-around is handled implicitly by unsigned 32-bit
 *          subtraction — periods up to 2^31 ms (~ 24.9 days) are safe
 *
 * @author  Groß
 * @date    May 24, 2026
 */

#include "simple_scheduler.h"
#include "simple_timer.h"

static SCHEDULER_TaskHandle tasks[SCHEDULER_MAX_TASKS];
static uint8_t next_task_index = 0;

int SCHEDULER_AddTask(SCHEDULER_TaskFunction pvTaskCode, const char *pcName, uint32_t period_ms)
{
    if (next_task_index >= SCHEDULER_MAX_TASKS) {
        return SCHEDULER_ERR_FULL;
    }

    tasks[next_task_index].function    = pvTaskCode;
    tasks[next_task_index].period_ms   = period_ms;
    tasks[next_task_index].last_run_ms = SCHEDULER_GetTick();
    tasks[next_task_index].active      = 1;

    (void)pcName;       /* reserved for future debugging / tracing */

    return next_task_index++;
}

int SCHEDULER_RemoveTask(int taskIndex)
{
    if (taskIndex < 0 || taskIndex >= SCHEDULER_MAX_TASKS) {
        return SCHEDULER_ERR_NOT_FOUND;
    }

    tasks[taskIndex].active   = 0;
    tasks[taskIndex].function = 0;

    return SCHEDULER_OK;
}

void SCHEDULER_Init(void)
{
    tick_init();
}

uint32_t SCHEDULER_GetTick(void)
{
    return get_tick_ms();
}

void SCHEDULER_Run(void)
{
    uint32_t now = SCHEDULER_GetTick();

    for (int i = 0; i < next_task_index; i++) {
        if (tasks[i].active && tasks[i].function != 0) {
            if (now - tasks[i].last_run_ms >= tasks[i].period_ms) {
                tasks[i].last_run_ms = now;
                tasks[i].function();
            }
        }
    }
}
