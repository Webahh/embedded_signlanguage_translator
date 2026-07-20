/*
 * ui_callback.h
 *
 *  Created on: 02.07.2026
 *      Author: Weber
 */

#ifndef UI_CALLBACK_H
#define UI_CALLBACK_H

extern volatile uint8_t UI_Callback_Info_Active;

void _dr_cb_SystemMode(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_Palm(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_Hand(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_Sign(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_toggle_SystemTime(uint8_t idx, uint8_t val, void *ctx);
void _dr_cb_toggle_SystemInfo(uint8_t idx, uint8_t val, void *ctx);

#endif /* UI_CALLBACK_H */
