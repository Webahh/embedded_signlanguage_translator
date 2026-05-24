#ifndef SIMPLE_SCHEDULER_H
#define SIMPLE_SCHEDULER_H

#include <stdint.h>

#define MAX_TASKS 10

typedef void (*TaskFunction_t)(void);

typedef struct {
    TaskFunction_t function;
    uint32_t period_ms;
    uint32_t last_run_ms;
    uint8_t active;
} TaskHandle_t;

typedef enum {
    SCHEDULER_OK = 0,
    SCHEDULER_ERR_FULL = -1,
    SCHEDULER_ERR_NOT_FOUND = -2
} SchedulerError_t;

int xTaskCreate(TaskFunction_t pvTaskCode, const char *pcName, uint32_t period_ms);
int xTaskDelete(int taskIndex);
void vTaskStartScheduler(void);
uint32_t xTaskGetTickCount(void);
void xTaskSchedulerRun(void);

#endif
