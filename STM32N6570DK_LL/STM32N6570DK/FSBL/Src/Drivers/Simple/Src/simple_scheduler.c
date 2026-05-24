#include "simple_scheduler.h"
#include "simple_timer.h"

static TaskHandle_t tasks[MAX_TASKS];
static uint8_t next_task_index = 0;

int xTaskCreate(TaskFunction_t pvTaskCode, const char *pcName, uint32_t period_ms) {
    if (next_task_index >= MAX_TASKS) {
        return SCHEDULER_ERR_FULL;
    }

    tasks[next_task_index].function = pvTaskCode;
    tasks[next_task_index].period_ms = period_ms;
    tasks[next_task_index].last_run_ms = get_tick_ms();
    tasks[next_task_index].active = 1;

    (void)pcName;

    return next_task_index++;
}

int xTaskDelete(int taskIndex) {
    if (taskIndex < 0 || taskIndex >= MAX_TASKS) {
        return SCHEDULER_ERR_NOT_FOUND;
    }

    tasks[taskIndex].active = 0;
    tasks[taskIndex].function = 0;

    return SCHEDULER_OK;
}

void vTaskStartScheduler(void) {
    tick_init();
}

uint32_t xTaskGetTickCount(void) {
    return get_tick_ms();
}

void xTaskSchedulerRun(void) {
    uint32_t now = get_tick_ms();

    for (int i = 0; i < next_task_index; i++) {
        if (tasks[i].active && tasks[i].function != 0) {
            if (now - tasks[i].last_run_ms >= tasks[i].period_ms) {
                tasks[i].last_run_ms = now;
                tasks[i].function();
            }
        }
    }
}
