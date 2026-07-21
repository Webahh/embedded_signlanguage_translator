/*
 * ai_hal_adapter.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#include "stm32n6xx_hal.h"
#include "simple_scheduler.h"

/*
 * HAL compatibility bridge.
 *
 * The CACHEAXI HAL driver uses HAL_GetTick() for timeout handling.
 * The project already maintains its own millisecond tick.
 */
uint32_t HAL_GetTick(void)
{
	uint32_t tick = 0;

    return SCHEDULER_Tick_get(&tick);
}


