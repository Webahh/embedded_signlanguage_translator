/**
 * @file    tasks.h
 * @author  Weber
 * @date    05.06.2026
 * @brief   Application task declarations
 *
 * Usage
 * -----
 * Task functions registered with SCHEDULER_Task_add()
 */

#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

#define LED2_PIN 10
#define BG_NUM_COLORS 3

void vRecursionTestTask(void);
void vSystemTimeTask(void);
void vLEDTask(void);
void vBackgroundTask(void);
void vAETask(void);
void vTouchTask(void);
void vAIPipelineTask(void);
void vSystemInfoTask(void);

extern volatile uint8_t isr_systime_vis;
extern volatile uint8_t isr_sysinfo_vis;

#endif /* TASKS_H */
