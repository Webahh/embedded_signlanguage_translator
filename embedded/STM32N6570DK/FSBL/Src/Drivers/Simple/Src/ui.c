/**
 * @file    ui.c
 * @author  Groß
 * @date    23.06.2026
 * @brief   Simple UI: slide-out drawer with toggle / slider / selector / composite items
 */

#include "ui.h"
#include "simple_text.h"
#include "config.h"

// -------------------------------------------------------------------------
// Private constants - drawer
// -------------------------------------------------------------------------

#define _PAD_L          10
#define _PAD_R          10
#define _SURFACE_MARGIN   4

#define _FONT_W           8
#define _FONT_H          16

#define _BTN_PAD_TOP      8

// colours
#define _CLR_BTN        0x80444444U
#define _CLR_TRACK      0x80555555U
#define _CLR_BORDER     0x80777777U

// toggle
#define _TOGGLE_W       44
#define _TOGGLE_H       24
#define _THUMB_R         7

// slider
#define _SLIDER_H        8
#define _SLIDER_GAP     16
#define _SLIDER_MIN_W   20
#define _SLIDER_THUMB_R  6

// selector
#define _SEG_H          30
#define _SEG_GAP         2

// eye icon
#define _EYE_R           6
#define _EYE_PUPIL_R     2
#define _EYE_W          12
#define _EYE_GAP        12

// hamburger bars
#define _BAR_W          20
#define _BAR_H           3
#define _BAR_GAP         8

// -------------------------------------------------------------------------
// Private helpers - geometry
// -------------------------------------------------------------------------

// Y-offset of item idx below the hamburger button
static uint16_t _item_y(const UI_Drawer_TypeDef *drawer, uint8_t idx) {
	return drawer->y_pos + UI_DRAWER_BTN_SIZE + _BTN_PAD_TOP
		+ idx * (UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP);
}

// Pixel width of a text string at fixed font width
static uint8_t _label_w(const char *text) {
	uint8_t width = 0;
	for (uint8_t i = 0; text[i]; i++) {
		width += _FONT_W;
	}
	return width;
}

// Touch x -> 0-100 slider value, clamped
static uint8_t _value_from_slider_touch(int16_t touch_x, uint16_t slider_start_x, uint16_t slider_width) {
	int16_t relative_pos;
	if (touch_x < slider_start_x) {
		relative_pos = 0;
	}
	else if (touch_x >= slider_start_x + slider_width) {
		relative_pos = slider_width;
	}
	else {
		relative_pos = touch_x - slider_start_x;
	}

	uint8_t value = (uint8_t)((uint32_t)relative_pos * 100U / slider_width);
	return (value > 100) ? 100 : value;
}

// -------------------------------------------------------------------------
// Private - per-type draw
// -------------------------------------------------------------------------

// Single-line label (no interactivity)
static void _draw_label(const LTDC_LayerConfig_TypeDef *cfg,
	uint16_t x, uint16_t y, const char *s, uint32_t color) {
	TEXT_String_draw(cfg, s, x, y, color);
}

// On/off switch: label + coloured track + circular thumb
static void _draw_toggle(UI_Drawer_TypeDef *drawer, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t item_y = _item_y(drawer, idx);
	uint16_t label_x = drawer->x_pos + _PAD_L;
	uint16_t label_y = item_y + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2;

	TEXT_String_draw(cfg, item->label, label_x, label_y, drawer->color_text);

	uint16_t toggle_x = drawer->x_pos + UI_DRAWER_WIDTH - _PAD_R - _TOGGLE_W;
	uint16_t toggle_y = item_y + (UI_DRAWER_ITEM_HEIGHT - _TOGGLE_H) / 2;
	uint32_t toggle_color = item->value ? drawer->color_accent : _CLR_TRACK;
	LTDC_LayerDrawRect(cfg, toggle_x, toggle_y, _TOGGLE_W, _TOGGLE_H, toggle_color);
	LTDC_LayerDrawRectBorder(cfg, toggle_x, toggle_y, _TOGGLE_W, _TOGGLE_H, _CLR_BORDER);

	// Thumb slides to the right when ON
	uint16_t thumb_center_x = item->value
		? toggle_x + _TOGGLE_W - _THUMB_R - 3
		: toggle_x + _THUMB_R + 3;
	uint16_t thumb_center_y = toggle_y + _TOGGLE_H / 2;
	LTDC_LayerDrawCricle(cfg, thumb_center_x, thumb_center_y, _THUMB_R, drawer->color_text);
}

// Value bar: label + track + accent fill + thumb at current position
static void _draw_slider(UI_Drawer_TypeDef *drawer, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t item_y = _item_y(drawer, idx);
	uint16_t label_x = drawer->x_pos + _PAD_L;
	uint16_t label_y = item_y + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2;

	TEXT_String_draw(cfg, item->label, label_x, label_y, drawer->color_text);

	uint16_t slider_x = label_x + _label_w(item->label) + _SLIDER_GAP;
	uint16_t slider_y = item_y + (UI_DRAWER_ITEM_HEIGHT - _SLIDER_H) / 2;
	uint16_t slider_width = UI_DRAWER_WIDTH - _PAD_L - _PAD_R - _label_w(item->label) - _SLIDER_GAP;

	if ((int16_t)slider_width <= 0) {
		return;
	}

	// Background track, then filled portion, then thumb circle
	LTDC_LayerDrawRect(cfg, slider_x, slider_y, slider_width, _SLIDER_H, _CLR_TRACK);
	uint16_t fill_width = (uint16_t)((uint32_t)slider_width * item->value / 100);
	if (fill_width > 0) {
		LTDC_LayerDrawRect(cfg, slider_x, slider_y, fill_width, _SLIDER_H, drawer->color_accent);
	}
	LTDC_LayerDrawCricle(cfg, slider_x + fill_width, slider_y + _SLIDER_H / 2,
		_THUMB_R, drawer->color_text);
}

// Segmented control: highlights the active segment
static void _draw_selector(UI_Drawer_TypeDef *drawer, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t item_y = _item_y(drawer, idx);
	uint16_t start_x = drawer->x_pos + _PAD_L;
	uint16_t area_width = UI_DRAWER_WIDTH - _PAD_L - _PAD_R;
	uint8_t seg_count = (item->seg_labels && item->seg_count) ? item->seg_count : 3;
	uint16_t seg_width = area_width / seg_count;
	uint16_t seg_y = item_y + (UI_DRAWER_ITEM_HEIGHT - _SEG_H) / 2;

	// Draw each segment: accent background for the selected one
	for (uint8_t seg_idx = 0; seg_idx < seg_count; seg_idx++) {
		const char *label = item->seg_labels ? item->seg_labels[seg_idx] : "";
		uint16_t seg_x = start_x + seg_idx * seg_width;
		uint32_t bg = (seg_idx == item->value) ? drawer->color_accent : _CLR_BTN;
		LTDC_LayerDrawRect(cfg, seg_x, seg_y, seg_width - _SEG_GAP, _SEG_H, bg);
		LTDC_LayerDrawRectBorder(cfg, seg_x, seg_y, seg_width - _SEG_GAP, _SEG_H, _CLR_BORDER);

		uint16_t text_x = seg_x + (seg_width - _SEG_GAP - _label_w(label)) / 2;
		uint16_t text_y = seg_y + (_SEG_H - _FONT_H) / 2;
		TEXT_String_draw(cfg, label, text_x, text_y, drawer->color_text);
	}
}

// Visibility eye + opacity slider in one row
static void _draw_composite(UI_Drawer_TypeDef *drawer, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t item_y = _item_y(drawer, idx);
	uint16_t content_x = drawer->x_pos + _PAD_L;
	uint16_t area_width = UI_DRAWER_WIDTH - _PAD_L - _PAD_R;
	uint16_t midpoint_y = item_y + UI_DRAWER_ITEM_HEIGHT / 2;

	TEXT_String_draw(cfg, item->label, content_x, midpoint_y - _FONT_H / 2,
		drawer->color_text);

	// Draw eye icon: filled circle when visible, line when hidden
	uint16_t eye_x = content_x + _label_w(item->label) + _EYE_GAP;
	uint16_t eye_y = midpoint_y - _EYE_R;
	if (item->composite.visible) {
		LTDC_LayerDrawCricle(cfg, eye_x + _EYE_R, eye_y + _EYE_R,
			_EYE_R, drawer->color_accent);
		LTDC_LayerDrawCricle(cfg, eye_x + _EYE_R, eye_y + _EYE_R,
			_EYE_PUPIL_R, drawer->color_accent);
	} else {
		LTDC_LayerDrawRect(cfg, eye_x + 2, eye_y + _EYE_R - 1,
			_EYE_R * 2 - 4, 2, drawer->color_text);
	}

	// Slider to the right of the eye
	uint16_t slider_x = eye_x + _EYE_R * 2 + 10;
	uint16_t slider_width = content_x + area_width - slider_x;
	if ((int16_t)slider_width > _SLIDER_MIN_W) {
		uint16_t slider_y = midpoint_y - _SLIDER_H / 2;
		LTDC_LayerDrawRect(cfg, slider_x, slider_y,
			slider_width, _SLIDER_H, _CLR_TRACK);
		uint16_t fill_width = (uint16_t)((uint32_t)slider_width
			* item->composite.slider_value / 100);
		if (fill_width > 0) {
			LTDC_LayerDrawRect(cfg, slider_x, slider_y,
				fill_width, _SLIDER_H, drawer->color_accent);
		}
		LTDC_LayerDrawCricle(cfg, slider_x + fill_width,
			slider_y + _SLIDER_H / 2,
			_SLIDER_THUMB_R, drawer->color_text);
	}
}

// -------------------------------------------------------------------------
// Private - draw dispatch
// -------------------------------------------------------------------------

// Dispatch to the correct drawer function for this item type
static void _draw_item(UI_Drawer_TypeDef *drawer, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {

	if (_item_y(drawer, idx) + UI_DRAWER_ITEM_HEIGHT > cfg->height) {
		return;
	}

	switch (drawer->items[idx].type) {
	case UI_DRAWER_ITEM_LABEL:
		_draw_label(cfg,
			drawer->x_pos + _PAD_L,
			_item_y(drawer, idx) + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2,
			drawer->items[idx].label, drawer->color_text);
		break;
	case UI_DRAWER_ITEM_TOGGLE:   _draw_toggle(drawer, idx, cfg);    break;
	case UI_DRAWER_ITEM_SLIDER:   _draw_slider(drawer, idx, cfg);    break;
	case UI_DRAWER_ITEM_SELECTOR: _draw_selector(drawer, idx, cfg);  break;
	case UI_DRAWER_ITEM_COMPOSITE:_draw_composite(drawer, idx, cfg); break;
	}
}

// -------------------------------------------------------------------------
// Private - hamburger icon
// -------------------------------------------------------------------------

// Three horizontal bars stacked vertically
static void _draw_hamburger(const LTDC_LayerConfig_TypeDef *cfg,
	int16_t button_x, int16_t button_y, uint32_t color) {
	uint16_t center_x = (uint16_t)button_x + (UI_DRAWER_BTN_SIZE / 2);
	uint16_t bar_start_x = center_x - _BAR_W / 2;
	LTDC_LayerDrawRect(cfg, bar_start_x, (uint16_t)button_y + _BAR_GAP + 4,
		_BAR_W, _BAR_H, color);
	LTDC_LayerDrawRect(cfg, bar_start_x, (uint16_t)button_y + _BAR_GAP + 12,
		_BAR_W, _BAR_H, color);
	LTDC_LayerDrawRect(cfg, bar_start_x, (uint16_t)button_y + _BAR_GAP + 20,
		_BAR_W, _BAR_H, color);
}

// -------------------------------------------------------------------------
// Private - per-type touch handlers
// -------------------------------------------------------------------------

// Flip toggle value and fire callback
static int _touch_toggle(UI_Drawer_TypeDef *drawer, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)tx; (void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	item->value = !item->value;

	if (item->callback) {
		item->callback(idx, item->value, item->context);
	}

	return 1;
}

// Map touch x-coordinate onto a 0-100 value and fire callback
static int _touch_slider(UI_Drawer_TypeDef *drawer, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t slider_start_x = drawer->x_pos + _PAD_L + _label_w(item->label) + _SLIDER_GAP;
	uint16_t slider_width = UI_DRAWER_WIDTH - _PAD_L - _PAD_R
		- _label_w(item->label) - _SLIDER_GAP;
	item->value = _value_from_slider_touch((int16_t)tx, slider_start_x, slider_width);
	if (item->callback) {
		item->callback(idx, item->value, item->context);
	}
	return 1;
}

// Map touch x-coordinate onto a segment index and fire callback
static int _touch_selector(UI_Drawer_TypeDef *drawer, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t seg_start_x = drawer->x_pos + _PAD_L;
	uint8_t seg_count = (item->seg_labels && item->seg_count) ? item->seg_count : 3;
	uint16_t seg_width = (UI_DRAWER_WIDTH - _PAD_L - _PAD_R) / seg_count;
	uint8_t seg_idx = (tx < seg_start_x) ? 0 : (tx - seg_start_x) / seg_width;

	if (seg_idx > seg_count - 1) {
		seg_idx = seg_count - 1;
	}

	item->value = seg_idx;

	if (item->callback) {
		item->callback(idx, item->value, item->context);
	}

	return 1;
}

// Check eye region first (toggle visibility), else handle slider
static int _touch_composite(UI_Drawer_TypeDef *drawer, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *item = &drawer->items[idx];
	uint16_t eye_x = drawer->x_pos + _PAD_L + _label_w(item->label) + _EYE_GAP;

	// Tap on eye -> toggle visibility
	if (tx >= eye_x && tx < eye_x + _EYE_W) {
		item->composite.visible = !item->composite.visible;

		if (item->callback) {
			item->callback(idx, item->composite.visible, item->context);
		}

		return 1;
	}

	// Touch on slider area -> update slider value
	uint16_t slider_x = eye_x + _EYE_R * 2 + 10;
	uint16_t slider_width = UI_DRAWER_WIDTH - _PAD_L - _PAD_R - (slider_x - drawer->x_pos);

	if ((int16_t)slider_width > _SLIDER_MIN_W && tx >= slider_x) {
		item->composite.slider_value = _value_from_slider_touch((int16_t)tx, slider_x, slider_width);
		if (item->callback) {
			item->callback(idx, item->composite.slider_value, item->context);
		}
		return 1;
	}
	return 0;
}

// -------------------------------------------------------------------------
// API - drawer lifecycle
// -------------------------------------------------------------------------

// Set defaults and assign the two framebuffers
void UI_Drawer_Init(UI_Drawer_TypeDef *drawer,
	uint8_t *buf_open, uint8_t *buf_closed) {
	drawer->x_pos = 0;
	drawer->y_pos = 0;
	drawer->is_open = 0;
	drawer->color_bg     = 0x802B2B2BU;
	drawer->color_border = _CLR_BORDER;
	drawer->color_text   = 0xFFFFFFFFU;
	drawer->color_accent = 0xFF00DD55U;
	drawer->color_surface = 0x80383838U;
	drawer->item_count = 0;
	drawer->active_item = -1;
	drawer->buf_open = buf_open;
	drawer->buf_closed = buf_closed;
}

// Pre-render both open/closed states into their respective buffers
void UI_Drawer_Prepare(UI_Drawer_TypeDef *drawer,
	LTDC_LayerConfig_TypeDef *cfg) {
	LTDC_LayerConfig_TypeDef temp_cfg = *cfg;

	// Render closed state
	temp_cfg.fb = drawer->buf_closed;
	LTDC_LayerFill(&temp_cfg, 0x00000000U);
	drawer->is_open = 0;
	UI_Drawer_Draw(drawer, &temp_cfg);

	// Render open state
	temp_cfg.fb = drawer->buf_open;
	LTDC_LayerFill(&temp_cfg, 0x00000000U);
	drawer->is_open = 1;
	UI_Drawer_Draw(drawer, &temp_cfg);

	// Start closed
	drawer->is_open = 0;
	cfg->fb = drawer->buf_closed;
	LTDC_UpdateLayerAddress(cfg);
}

// Add a new item at the end of the items array
UI_Status_TypeDef UI_Drawer_AddItem(UI_Drawer_TypeDef *drawer,
	UI_DrawerItemType_TypeDef type, const char *label,
	UI_DrawerItemCallback_TypeDef cb, void *context, int *out_idx) {

	if (drawer->item_count >= UI_DRAWER_MAX_ITEMS) {
		return UI_ERR_FULL;
	}

	UI_DrawerItem_TypeDef *item = &drawer->items[drawer->item_count];
	item->type = type;
	item->value = (type == UI_DRAWER_ITEM_TOGGLE
		|| type == UI_DRAWER_ITEM_SELECTOR) ? 0 : 50;
	item->composite.visible = 1;
	item->composite.slider_value  = 50;
	item->seg_labels = NULL;
	item->seg_count = 0;
	item->callback = cb;
	item->context = context;

	uint8_t i;
	for (i = 0; i < sizeof(item->label) - 1 && label[i]; i++) {
		item->label[i] = label[i];
	}
	item->label[i] = '\0';

	if (out_idx) {
		*out_idx = (int)drawer->item_count;
	}
	drawer->item_count++;
	return UI_OK;
}

// Swap framebuffer pointers to show/hide the drawer
void UI_Drawer_Toggle(UI_Drawer_TypeDef *drawer,
	LTDC_LayerConfig_TypeDef *cfg) {
	drawer->is_open = !drawer->is_open;
	cfg->fb = drawer->is_open ? drawer->buf_open : drawer->buf_closed;
	LTDC_UpdateLayerAddress(cfg);
}

// Read item.value (toggle 0/1, slider 0-100, selector segment index)
UI_Status_TypeDef UI_Drawer_GetItemValue(
	const UI_Drawer_TypeDef *drawer, uint8_t idx, uint8_t *out_val) {

	if (idx >= drawer->item_count) {
		return UI_ERR_RANGE;
	}

	if (out_val) {
		*out_val = drawer->items[idx].value;
	}

	return UI_OK;
}

// -------------------------------------------------------------------------
// API - drawer touch dispatch
// -------------------------------------------------------------------------

// Two-stage touch: press marks the active item, release acts on it
UI_Status_TypeDef UI_Drawer_HandleTouch(UI_Drawer_TypeDef *drawer,
	uint16_t touch_x, uint16_t touch_y, uint8_t pressed,
	LTDC_LayerConfig_TypeDef *cfg, int *out_idx) {
	// --- Press phase: remember which item was hit ---
	if (pressed) {
		// Hamburger button area -> toggle open/close on release
		if (touch_x >= drawer->x_pos && touch_x < drawer->x_pos + UI_DRAWER_BTN_SIZE &&
			touch_y >= drawer->y_pos && touch_y < drawer->y_pos + UI_DRAWER_BTN_SIZE) {
			drawer->active_item = -2;
			if (out_idx) {
				*out_idx = -2;
			}
			return UI_OK;
		}

		// Check each drawer item for a hit
		if (drawer->is_open) {
			for (uint8_t i = 0; i < drawer->item_count; i++) {
				uint16_t item_y = _item_y(drawer, i);
				if (touch_x >= drawer->x_pos + _PAD_L &&
					touch_x < drawer->x_pos + UI_DRAWER_WIDTH - _PAD_R &&
					touch_y >= item_y && touch_y < item_y + UI_DRAWER_ITEM_HEIGHT) {
					drawer->active_item = (int8_t)i;
					if (out_idx) {
						*out_idx = (int)i;
					}
					return UI_OK;
				}
			}
		}
		drawer->active_item = -1;
		if (out_idx) {
			*out_idx = -1;
		}
		return UI_ERR_NOT_FOUND;
	}

	// --- Release phase: act on the previously pressed item ---

	// Hamburger was pressed -> toggle
	if (drawer->active_item == -2) {
		drawer->active_item = -1;
		UI_Drawer_Toggle(drawer, cfg);
		if (out_idx) {
			*out_idx = -2;
		}
		return UI_OK;
	}

	// A valid item was pressed -> route to its touch handler
	if (drawer->active_item >= 0 && (uint8_t)drawer->active_item < drawer->item_count) {
		uint8_t idx = (uint8_t)drawer->active_item;
		int handled = 0;
		switch (drawer->items[idx].type) {
			case UI_DRAWER_ITEM_TOGGLE:
				handled = _touch_toggle(drawer, idx, touch_x, touch_y, cfg);    break;
			case UI_DRAWER_ITEM_SLIDER:
				handled = _touch_slider(drawer, idx, touch_x, touch_y, cfg);    break;
			case UI_DRAWER_ITEM_SELECTOR:
				handled = _touch_selector(drawer, idx, touch_x, touch_y, cfg);  break;
			case UI_DRAWER_ITEM_COMPOSITE:
				handled = _touch_composite(drawer, idx, touch_x, touch_y, cfg); break;
			default: break;
		}

		drawer->active_item = -1;
		if (handled) {
			// Re-render just this item on the open framebuffer
			LTDC_LayerConfig_TypeDef temp_cfg = *cfg;
			temp_cfg.fb = drawer->buf_open;
			UI_Drawer_DrawItem(drawer, idx, &temp_cfg);
			if (out_idx) {
				*out_idx = (int)idx;
			}
			return UI_OK;
		}
	}
	drawer->active_item = -1;
	if (out_idx) {
		*out_idx = -1;
	}
	return UI_ERR_NOT_FOUND;
}

// -------------------------------------------------------------------------
// API - drawer render
// -------------------------------------------------------------------------

// Full redraw: clear, then draw panel background + all items + hamburger
void UI_Drawer_Draw(UI_Drawer_TypeDef *drawer,
	const LTDC_LayerConfig_TypeDef *cfg) {
	// Clear the entire drawer area
	LTDC_LayerDrawRect(cfg, drawer->x_pos, drawer->y_pos,
		UI_DRAWER_WIDTH, cfg->height - drawer->y_pos, 0x00000000U);

	if (drawer->is_open) {
		// Panel background and border
		uint16_t panel_height = cfg->height - drawer->y_pos;
		LTDC_LayerDrawRect(cfg, drawer->x_pos, drawer->y_pos,
			UI_DRAWER_WIDTH, panel_height, drawer->color_bg);
		LTDC_LayerDrawRectBorder(cfg, drawer->x_pos, drawer->y_pos,
			UI_DRAWER_WIDTH, panel_height, drawer->color_border);

		// Item surface cards
		for (uint8_t i = 0; i < drawer->item_count; i++) {
			uint16_t item_y = _item_y(drawer, i);

			if (item_y + UI_DRAWER_ITEM_HEIGHT > cfg->height) {
				break;
			}

			LTDC_LayerDrawRect(cfg, drawer->x_pos + _SURFACE_MARGIN, item_y,
				UI_DRAWER_WIDTH - _SURFACE_MARGIN * 2,
				UI_DRAWER_ITEM_HEIGHT, drawer->color_surface);
		}

		// Draw each item's controls
		for (uint8_t i = 0; i < drawer->item_count; i++) {
			_draw_item(drawer, i, cfg);
		}
	}

	// Hamburger button (always visible)
	LTDC_LayerDrawRect(cfg, drawer->x_pos, drawer->y_pos,
		UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, _CLR_BTN);
	LTDC_LayerDrawRectBorder(cfg, drawer->x_pos, drawer->y_pos,
		UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, drawer->color_border);
	_draw_hamburger(cfg, (int16_t)drawer->x_pos, (int16_t)drawer->y_pos, drawer->color_text);
}

// Redraw the surface card + controls for a single item
void UI_Drawer_DrawItem(UI_Drawer_TypeDef *drawer,
	uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg) {

	if (idx >= drawer->item_count || !drawer->is_open) {
		return;
	}
	uint16_t item_y = _item_y(drawer, idx);
	if (item_y + UI_DRAWER_ITEM_HEIGHT > cfg->height){
		return;
	}

	LTDC_LayerDrawRect(cfg, drawer->x_pos + _SURFACE_MARGIN, item_y,
		UI_DRAWER_WIDTH - _SURFACE_MARGIN * 2,
		UI_DRAWER_ITEM_HEIGHT, drawer->color_surface);

	_draw_item(drawer, idx, cfg);
}
