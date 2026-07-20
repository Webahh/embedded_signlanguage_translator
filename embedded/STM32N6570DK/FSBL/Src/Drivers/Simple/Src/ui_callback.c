/*
 * ui_callback.c
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#include "ui.h"
#include "config.h"
#include "ui_callback.h"
#include "simple_debug_log.h"
#include "tasks.h"
#include "simple_scheduler.h"
#include "simple_ltdc_layer_draw.h"

volatile uint8_t UI_Callback_Info_Active = 0;

void _dr_cb_SystemMode(uint8_t idx, uint8_t val, void *ctx)
{
	(void)idx;
	(void)ctx;
	DEBUG_PRINTF("[UI] Mode: %u\r\n", val);
}

void _dr_cb_Palm(uint8_t idx, uint8_t val, void *ctx)
{
	(void)idx;
	(void)val;
	UI_Drawer_TypeDef *drawer = (UI_Drawer_TypeDef *)ctx;
	DEBUG_PRINTF("[UI] Palm (ROI draw): visible=%u slider=%u\r\n",
		drawer->items[idx].composite.visible,
		drawer->items[idx].composite.slider_value);
}

void _dr_cb_Hand(uint8_t idx, uint8_t val, void *ctx)
{
	(void)idx;
	(void)val;
	UI_Drawer_TypeDef *drawer = (UI_Drawer_TypeDef *)ctx;
	DEBUG_PRINTF("[UI] Hand (Landmark draw): visible=%u slider=%u\r\n",
		drawer->items[idx].composite.visible,
		drawer->items[idx].composite.slider_value);
}

void _dr_cb_Sign(uint8_t idx, uint8_t val, void *ctx)
{
	(void)idx;
	(void)val;
	UI_Drawer_TypeDef *drawer = (UI_Drawer_TypeDef *)ctx;
	DEBUG_PRINTF("[UI] Sign (result print): visible=%u slider=%u\r\n",
		drawer->items[idx].composite.visible,
		drawer->items[idx].composite.slider_value);
}

void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx)
{
	static uint8_t systemtime_id = 0xFF;
	(void)idx;
	(void)ctx;
	if (val) {
		SCHEDULER_Task_add(vSystemTimeTask, "Sys Systemtime", 100, 1, 256, &systemtime_id);
		isr_systime_vis = 1U;
	} else {
		SCHEDULER_Task_remove(systemtime_id);
		isr_systime_vis = 0U;
	}
}

void _dr_cb_toggle_SystemInfo(uint8_t idx, uint8_t val, void *ctx)
{
	static uint8_t systeminfo_id = 0xFF;
	(void)idx;
	(void)ctx;
	if (val) {
		SCHEDULER_Task_add(vSystemInfoTask, "Sys Systeminfo", 100, 1, 256, &systeminfo_id);
		isr_sysinfo_vis = 1U;
		UI_Callback_Info_Active = 1;
	} else {
		SCHEDULER_Task_remove(systeminfo_id);
		isr_sysinfo_vis = 0U;
		UI_Callback_Info_Active = 0;
	}
}


