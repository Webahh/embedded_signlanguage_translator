/*
 * ui_callback.h
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#ifndef UI_CALLBACK_H
#define UI_CALLBACK_H

void _dr_cb_SystemMode(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_composite(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_toggle_SystemInfo(uint8_t idx, uint8_t val, void *ctx);

#endif /* UI_CALLBACK_H */
