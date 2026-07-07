/*
 * ui_callback.c
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#include "ui.h"
#include "config.h"
#include "simple_debug_log.h"
#include "tasks.h"
#include "simple_scheduler.h"
#include "simple_ltdc_layer_draw.h"

void _dr_cb_SystemMode(uint8_t idx, uint8_t val, void *ctx)
{
	(void)idx;
	(void)ctx;
	DEBUG_PRINTF("[UI] Mode: %u\r\n", val);
}

void _dr_cb_composite(uint8_t idx, uint8_t val, void *ctx)
{
	(void)val;
	UI_Drawer_TypeDef *drawer = (UI_Drawer_TypeDef *)ctx;
	DEBUG_PRINTF("[UI] %s: visible=%u slider=%u\r\n",
		drawer->items[idx].label, drawer->items[idx].composite.visible,
		drawer->items[idx].composite.slider_value);
}

void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx)
{
	static uint8_t systemtime_id = 0xFF;
	(void)idx;
	(void)ctx;
	if (val) {
		SCHEDULER_Task_add(vSystemTimeTask, "Sys Systemtime", 100, 1, 256, &systemtime_id);
	} else {
		SCHEDULER_Task_remove(systemtime_id);
		LTDC_Layer_Draw_Rect(&LTDC_Layer2Config, 720, 0, 80, 16, 0x00000000);
	}
}

void _dr_cb_toggle_SystemInfo(uint8_t idx, uint8_t val, void *ctx)
{
	static uint8_t systeminfo_id = 0xFF;
	(void)idx;
	(void)ctx;
	if (val) {
		SCHEDULER_Task_add(vSystemInfoTask, "Sys Systeminfo", 100, 1, 256, &systeminfo_id);
	} else {
		SCHEDULER_Task_remove(systeminfo_id);
		LTDC_Layer_Draw_Rect(&LTDC_Layer1Config, 720, 16, 80, 48, 0x00000000U);
	}
}


