/**
 * @file    ui.c
 * @author  Groß
 * @date    23.06.2026
 * @brief   Simple UI: button objects + drawer with controls
*/

#include "ui.h"
#include "simple_text.h"
#include "config.h"

// -------------------------------------------------------------------------
// Private helpers - simple buttons
// -------------------------------------------------------------------------

static UI_Object_TypeDef _objects[UI_MAX_OBJECTS];
static int _object_count = 0;

// -------------------------------------------------------------------------
// API - simple buttons
// -------------------------------------------------------------------------

void UI_Init(void) {
	_object_count = 0;
}

UI_Status_TypeDef UI_AddButton(uint16_t x, uint16_t y, uint16_t w,
	uint16_t h, uint32_t color, UI_Callback_TypeDef cb,
	int *out_idx) {
	if (_object_count >= UI_MAX_OBJECTS) return UI_ERR_FULL;
	UI_Object_TypeDef *o = &_objects[_object_count];
	o->x = x;
	o->y = y;
	o->w = w;
	o->h = h;
	o->state = UI_STATE_IDLE;
	o->color = color;
	o->color_pressed = UI_COLOR_YELLOW_DARK;
	o->callback = cb;
	if (out_idx) *out_idx = _object_count;
	_object_count++;
	return UI_OK;
}

UI_Status_TypeDef UI_HandleTouch(uint16_t tx, uint16_t ty,
	uint8_t pressed, int *out_idx) {
	static int active_idx = -1;

	if (pressed) {
		for (int i = 0; i < _object_count; i++) {
			UI_Object_TypeDef *o = &_objects[i];
			if (tx >= o->x && tx < o->x + o->w &&
				ty >= o->y && ty < o->y + o->h) {
				o->state = UI_STATE_PRESSED;
				active_idx = i;
				if (out_idx) *out_idx = i;
				return UI_OK;
			}
		}
		active_idx = -1;
	} else {
		if (active_idx >= 0) {
			UI_Object_TypeDef *o = &_objects[active_idx];
			o->state = UI_STATE_IDLE;
			if (o->callback) o->callback();
			int idx = active_idx;
			active_idx = -1;
			if (out_idx) *out_idx = idx;
			return UI_OK;
		}
	}
	if (out_idx) *out_idx = -1;
	return UI_ERR_NOT_FOUND;
}

void UI_DrawAll(const LTDC_LayerConfig_TypeDef *cfg) {
	for (int i = 0; i < _object_count; i++) {
		UI_Object_TypeDef *o = &_objects[i];
		uint16_t lx = o->x - cfg->x;
		uint16_t ly = o->y - cfg->y;
		uint32_t fill = (o->state == UI_STATE_PRESSED)
			? o->color_pressed : o->color;
		LTDC_LayerDrawRect(cfg, lx, ly, o->w, o->h, fill);
		LTDC_LayerDrawRectBorder(cfg, lx, ly, o->w, o->h, 0x000000U);
	}
}

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

static uint16_t _item_y(const UI_Drawer_TypeDef *d, uint8_t idx) {
	return d->y + UI_DRAWER_BTN_SIZE + _BTN_PAD_TOP
		+ idx * (UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP);
}

static uint8_t _label_w(const char *s) {
	uint8_t w = 0;
	for (uint8_t i = 0; s[i]; i++) w += _FONT_W;
	return w;
}

static uint8_t _slider_from_tx(int16_t tx, uint16_t sx, uint16_t sw) {
	int16_t rel;
	if      (tx < sx)       rel = 0;
	else if (tx >= sx + sw) rel = sw;
	else                    rel = tx - sx;
	uint8_t v = (uint8_t)((uint32_t)rel * 100U / sw);
	return (v > 100) ? 100 : v;
}

// -------------------------------------------------------------------------
// Private - per-type draw
// -------------------------------------------------------------------------

static void _draw_label(const LTDC_LayerConfig_TypeDef *cfg,
	uint16_t x, uint16_t y, const char *s, uint32_t color) {
	TEXT_String_draw(cfg, s, x, y, color);
}

static void _draw_toggle(UI_Drawer_TypeDef *d, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t iy = _item_y(d, idx);
	uint16_t lx = d->x + _PAD_L;
	uint16_t ly = iy + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2;

	TEXT_String_draw(cfg, it->label, lx, ly, d->color_text);

	uint16_t tx = d->x + UI_DRAWER_WIDTH - _PAD_R - _TOGGLE_W;
	uint16_t ty = iy + (UI_DRAWER_ITEM_HEIGHT - _TOGGLE_H) / 2;
	uint32_t tc = it->value ? d->color_accent : _CLR_TRACK;
	LTDC_LayerDrawRect(cfg, tx, ty, _TOGGLE_W, _TOGGLE_H, tc);
	LTDC_LayerDrawRectBorder(cfg, tx, ty, _TOGGLE_W, _TOGGLE_H, _CLR_BORDER);

	uint16_t thumb_cx = it->value
		? tx + _TOGGLE_W - _THUMB_R - 3
		: tx + _THUMB_R + 3;
	uint16_t thumb_cy = ty + _TOGGLE_H / 2;
	LTDC_LayerDrawCricle(cfg, thumb_cx, thumb_cy, _THUMB_R, d->color_text);
}

static void _draw_slider(UI_Drawer_TypeDef *d, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t iy = _item_y(d, idx);
	uint16_t lx = d->x + _PAD_L;
	uint16_t ly = iy + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2;

	TEXT_String_draw(cfg, it->label, lx, ly, d->color_text);

	uint16_t sx = lx + _label_w(it->label) + _SLIDER_GAP;
	uint16_t sy = iy + (UI_DRAWER_ITEM_HEIGHT - _SLIDER_H) / 2;
	uint16_t sw = UI_DRAWER_WIDTH - _PAD_L - _PAD_R - _label_w(it->label) - _SLIDER_GAP;
	if ((int16_t)sw <= 0) return;

	LTDC_LayerDrawRect(cfg, sx, sy, sw, _SLIDER_H, _CLR_TRACK);
	uint16_t fw = (uint16_t)((uint32_t)sw * it->value / 100);
	if (fw > 0)
		LTDC_LayerDrawRect(cfg, sx, sy, fw, _SLIDER_H, d->color_accent);
	LTDC_LayerDrawCricle(cfg, sx + fw, sy + _SLIDER_H / 2,
		_THUMB_R, d->color_text);
}

static void _draw_selector(UI_Drawer_TypeDef *d, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t iy = _item_y(d, idx);
	uint16_t sx = d->x + _PAD_L;
	uint16_t aw = UI_DRAWER_WIDTH - _PAD_L - _PAD_R;
	uint16_t sw = aw / 3;
	uint16_t sy = iy + (UI_DRAWER_ITEM_HEIGHT - _SEG_H) / 2;

	const char *labels[3] = {"Palm", "Hand", "Sign"};
	for (uint8_t s = 0; s < 3; s++) {
		uint16_t seg_x = sx + s * sw;
		uint32_t bg = (s == it->value) ? d->color_accent : _CLR_BTN;
		LTDC_LayerDrawRect(cfg, seg_x, sy, sw - _SEG_GAP, _SEG_H, bg);
		LTDC_LayerDrawRectBorder(cfg, seg_x, sy,
			sw - _SEG_GAP, _SEG_H, _CLR_BORDER);

		uint16_t tx = seg_x + (sw - _SEG_GAP - _label_w(labels[s])) / 2;
		uint16_t ty = sy + (_SEG_H - _FONT_H) / 2;
		TEXT_String_draw(cfg, labels[s], tx, ty, d->color_text);
	}
}

static void _draw_composite(UI_Drawer_TypeDef *d, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t iy = _item_y(d, idx);
	uint16_t cx = d->x + _PAD_L;
	uint16_t aw = UI_DRAWER_WIDTH - _PAD_L - _PAD_R;
	uint16_t mid_y = iy + UI_DRAWER_ITEM_HEIGHT / 2;

	TEXT_String_draw(cfg, it->label, cx, mid_y - _FONT_H / 2,
		d->color_text);

	uint16_t eye_x = cx + _label_w(it->label) + _EYE_GAP;
	uint16_t eye_y = mid_y - _EYE_R;
	if (it->comp.visible) {
		LTDC_LayerDrawCricle(cfg, eye_x + _EYE_R, eye_y + _EYE_R,
			_EYE_R, d->color_text);
		LTDC_LayerDrawCricle(cfg, eye_x + _EYE_R, eye_y + _EYE_R,
			_EYE_PUPIL_R, d->color_text);
	} else {
		LTDC_LayerDrawRect(cfg, eye_x + 2, eye_y + _EYE_R - 1,
			_EYE_R * 2 - 4, 2, d->color_text);
	}

	uint16_t slider_x = eye_x + _EYE_R * 2 + 10;
	uint16_t slider_w = cx + aw - slider_x;
	if ((int16_t)slider_w > _SLIDER_MIN_W) {
		uint16_t slider_y = mid_y - _SLIDER_H / 2;
		LTDC_LayerDrawRect(cfg, slider_x, slider_y,
			slider_w, _SLIDER_H, _CLR_TRACK);
		uint16_t fw = (uint16_t)((uint32_t)slider_w
			* it->comp.slider / 100);
		if (fw > 0)
			LTDC_LayerDrawRect(cfg, slider_x, slider_y,
				fw, _SLIDER_H, d->color_accent);
		LTDC_LayerDrawCricle(cfg, slider_x + fw,
			slider_y + _SLIDER_H / 2,
			_SLIDER_THUMB_R, d->color_text);
	}
}

// -------------------------------------------------------------------------
// Private - draw dispatch
// -------------------------------------------------------------------------

static void _draw_item(UI_Drawer_TypeDef *d, uint8_t idx,
	const LTDC_LayerConfig_TypeDef *cfg) {
	if (_item_y(d, idx) + UI_DRAWER_ITEM_HEIGHT > cfg->height) return;
	switch (d->items[idx].type) {
	case UI_DRAWER_ITEM_LABEL:
		_draw_label(cfg,
			d->x + _PAD_L,
			_item_y(d, idx) + (UI_DRAWER_ITEM_HEIGHT - _FONT_H) / 2,
			d->items[idx].label, d->color_text);
		break;
	case UI_DRAWER_ITEM_TOGGLE:   _draw_toggle(d, idx, cfg);    break;
	case UI_DRAWER_ITEM_SLIDER:   _draw_slider(d, idx, cfg);    break;
	case UI_DRAWER_ITEM_SELECTOR: _draw_selector(d, idx, cfg);  break;
	case UI_DRAWER_ITEM_COMPOSITE:_draw_composite(d, idx, cfg); break;
	}
}

// -------------------------------------------------------------------------
// Private - hamburger icon
// -------------------------------------------------------------------------

static void _draw_hamburger(const LTDC_LayerConfig_TypeDef *cfg,
	int16_t bx, int16_t by, uint32_t color) {
	uint16_t cx = (uint16_t)bx + (UI_DRAWER_BTN_SIZE / 2);
	uint16_t bar_x = cx - _BAR_W / 2;
	LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + _BAR_GAP + 4,
		_BAR_W, _BAR_H, color);
	LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + _BAR_GAP + 12,
		_BAR_W, _BAR_H, color);
	LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + _BAR_GAP + 20,
		_BAR_W, _BAR_H, color);
}

// -------------------------------------------------------------------------
// Private - per-type touch handlers
// -------------------------------------------------------------------------

static int _touch_toggle(UI_Drawer_TypeDef *d, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)tx; (void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	it->value = !it->value;
	if (it->callback) it->callback(idx, it->value, it->context);
	return 1;
}

static int _touch_slider(UI_Drawer_TypeDef *d, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t sx = d->x + _PAD_L + _label_w(it->label) + _SLIDER_GAP;
	uint16_t sw = UI_DRAWER_WIDTH - _PAD_L - _PAD_R
		- _label_w(it->label) - _SLIDER_GAP;
	it->value = _slider_from_tx((int16_t)tx, sx, sw);
	if (it->callback) it->callback(idx, it->value, it->context);
	return 1;
}

static int _touch_selector(UI_Drawer_TypeDef *d, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t seg_x = d->x + _PAD_L;
	uint16_t seg_w = (UI_DRAWER_WIDTH - _PAD_L - _PAD_R) / 3;
	uint8_t s = (tx < seg_x) ? 0 : (tx - seg_x) / seg_w;
	if (s > 2) s = 2;
	it->value = s;
	if (it->callback) it->callback(idx, it->value, it->context);
	return 1;
}

static int _touch_composite(UI_Drawer_TypeDef *d, uint8_t idx,
	uint16_t tx, uint16_t ty, const LTDC_LayerConfig_TypeDef *cfg) {
	(void)ty; (void)cfg;
	UI_DrawerItem_TypeDef *it = &d->items[idx];
	uint16_t eye_x = d->x + _PAD_L + _label_w(it->label) + _EYE_GAP;

	if (tx >= eye_x && tx < eye_x + _EYE_W) {
		it->comp.visible = !it->comp.visible;
		if (it->callback) it->callback(idx, it->comp.visible, it->context);
		return 1;
	}

	uint16_t slider_x = eye_x + _EYE_R * 2 + 10;
	uint16_t slider_w = UI_DRAWER_WIDTH - _PAD_L - _PAD_R - (slider_x - d->x);
	if ((int16_t)slider_w > _SLIDER_MIN_W && tx >= slider_x) {
		it->comp.slider = _slider_from_tx((int16_t)tx, slider_x, slider_w);
		if (it->callback) it->callback(idx, it->comp.slider, it->context);
		return 1;
	}
	return 0;
}

// -------------------------------------------------------------------------
// API - drawer lifecycle
// -------------------------------------------------------------------------

void UI_Drawer_Init(UI_Drawer_TypeDef *d,
	uint8_t *buf_open, uint8_t *buf_closed) {
	d->x = 0;
	d->y = 0;
	d->is_open = 0;
	d->color_bg     = 0x802B2B2BU;
	d->color_border = _CLR_BORDER;
	d->color_text   = 0xFFFFFFFFU;
	d->color_accent = 0xFF00DD55U;
	d->color_surface = 0x80383838U;
	d->item_count = 0;
	d->active_item = -1;
	d->buf_open = buf_open;
	d->buf_closed = buf_closed;
}

void UI_Drawer_Prepare(UI_Drawer_TypeDef *d,
	const LTDC_LayerConfig_TypeDef *cfg) {
	LTDC_LayerConfig_TypeDef tmp = *cfg;

	tmp.fb = d->buf_closed;
	LTDC_LayerFill(&tmp, 0x00000000U);
	d->is_open = 0;
	UI_Drawer_Draw(d, &tmp);

	tmp.fb = d->buf_open;
	LTDC_LayerFill(&tmp, 0x00000000U);
	d->is_open = 1;
	UI_Drawer_Draw(d, &tmp);

	d->is_open = 0;
	LTDC_Layer2Config.fb = d->buf_closed;
	LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

UI_Status_TypeDef UI_Drawer_AddItem(UI_Drawer_TypeDef *d,
	UI_DrawerItemType_TypeDef type, const char *label,
	UI_DrawerItemCallback_TypeDef cb, int *out_idx) {
	if (d->item_count >= UI_DRAWER_MAX_ITEMS) return UI_ERR_FULL;
	UI_DrawerItem_TypeDef *it = &d->items[d->item_count];
	it->type = type;
	it->value = (type == UI_DRAWER_ITEM_TOGGLE
		|| type == UI_DRAWER_ITEM_SELECTOR) ? 0 : 50;
	it->comp.visible = 1;
	it->comp.slider  = 50;
	it->callback = cb;
	it->context = NULL;

	uint8_t i;
	for (i = 0; i < sizeof(it->label) - 1 && label[i]; i++)
		it->label[i] = label[i];
	it->label[i] = '\0';

	if (out_idx) *out_idx = (int)d->item_count;
	d->item_count++;
	return UI_OK;
}

void UI_Drawer_Toggle(UI_Drawer_TypeDef *d) {
	d->is_open = !d->is_open;
	LTDC_Layer2Config.fb = d->is_open ? d->buf_open : d->buf_closed;
	LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

void UI_Drawer_Open(UI_Drawer_TypeDef *d) {
	d->is_open = 1;
	LTDC_Layer2Config.fb = d->buf_open;
	LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

void UI_Drawer_Close(UI_Drawer_TypeDef *d) {
	d->is_open = 0;
	LTDC_Layer2Config.fb = d->buf_closed;
	LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

UI_Status_TypeDef UI_Drawer_GetItemValue(
	const UI_Drawer_TypeDef *d, uint8_t idx, uint8_t *out_val) {
	if (idx >= d->item_count) return UI_ERR_RANGE;
	if (out_val) *out_val = d->items[idx].value;
	return UI_OK;
}

// -------------------------------------------------------------------------
// API - drawer touch dispatch
// -------------------------------------------------------------------------

UI_Status_TypeDef UI_Drawer_HandleTouch(UI_Drawer_TypeDef *d,
	uint16_t tx, uint16_t ty, uint8_t pressed,
	const LTDC_LayerConfig_TypeDef *cfg, int *out_idx) {
	if (pressed) {
		if (tx >= d->x && tx < d->x + UI_DRAWER_BTN_SIZE &&
			ty >= d->y && ty < d->y + UI_DRAWER_BTN_SIZE) {
			d->active_item = -2;
			if (out_idx) *out_idx = -2;
			return UI_OK;
		}

		if (d->is_open) {
			for (uint8_t i = 0; i < d->item_count; i++) {
				uint16_t iy = _item_y(d, i);
				if (tx >= d->x + _PAD_L &&
					tx < d->x + UI_DRAWER_WIDTH - _PAD_R &&
					ty >= iy && ty < iy + UI_DRAWER_ITEM_HEIGHT) {
					d->active_item = (int8_t)i;
					if (out_idx) *out_idx = (int)i;
					return UI_OK;
				}
			}
		}
		d->active_item = -1;
		if (out_idx) *out_idx = -1;
		return UI_ERR_NOT_FOUND;
	}

	if (d->active_item == -2) {
		d->active_item = -1;
		UI_Drawer_Toggle(d);
		if (out_idx) *out_idx = -2;
		return UI_OK;
	}

	if (d->active_item >= 0 && (uint8_t)d->active_item < d->item_count) {
		uint8_t idx = (uint8_t)d->active_item;
		int handled = 0;
		switch (d->items[idx].type) {
		case UI_DRAWER_ITEM_TOGGLE:
			handled = _touch_toggle(d, idx, tx, ty, cfg);    break;
		case UI_DRAWER_ITEM_SLIDER:
			handled = _touch_slider(d, idx, tx, ty, cfg);    break;
		case UI_DRAWER_ITEM_SELECTOR:
			handled = _touch_selector(d, idx, tx, ty, cfg);  break;
		case UI_DRAWER_ITEM_COMPOSITE:
			handled = _touch_composite(d, idx, tx, ty, cfg); break;
		default: break;
		}
		d->active_item = -1;
		if (handled) {
			LTDC_LayerConfig_TypeDef tmp = *cfg;
			tmp.fb = d->buf_open;
			UI_Drawer_DrawItem(d, idx, &tmp);
			if (out_idx) *out_idx = (int)idx;
			return UI_OK;
		}
	}
	d->active_item = -1;
	if (out_idx) *out_idx = -1;
	return UI_ERR_NOT_FOUND;
}

// -------------------------------------------------------------------------
// API - drawer render
// -------------------------------------------------------------------------

void UI_Drawer_Draw(UI_Drawer_TypeDef *d,
	const LTDC_LayerConfig_TypeDef *cfg) {
	LTDC_LayerDrawRect(cfg, d->x, d->y,
		UI_DRAWER_WIDTH, cfg->height - d->y, 0x00000000U);

	if (d->is_open) {
		uint16_t panel_h = cfg->height - d->y;
		LTDC_LayerDrawRect(cfg, d->x, d->y,
			UI_DRAWER_WIDTH, panel_h, d->color_bg);
		LTDC_LayerDrawRectBorder(cfg, d->x, d->y,
			UI_DRAWER_WIDTH, panel_h, d->color_border);

		for (uint8_t i = 0; i < d->item_count; i++) {
			uint16_t iy = _item_y(d, i);
			if (iy + UI_DRAWER_ITEM_HEIGHT > cfg->height) break;
			LTDC_LayerDrawRect(cfg, d->x + _SURFACE_MARGIN, iy,
				UI_DRAWER_WIDTH - _SURFACE_MARGIN * 2,
				UI_DRAWER_ITEM_HEIGHT, d->color_surface);
		}

		for (uint8_t i = 0; i < d->item_count; i++)
			_draw_item(d, i, cfg);
	}

	LTDC_LayerDrawRect(cfg, d->x, d->y,
		UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, _CLR_BTN);
	LTDC_LayerDrawRectBorder(cfg, d->x, d->y,
		UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, d->color_border);
	_draw_hamburger(cfg, (int16_t)d->x, (int16_t)d->y, d->color_text);
}

void UI_Drawer_DrawItem(UI_Drawer_TypeDef *d,
	uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg) {
	if (idx >= d->item_count || !d->is_open) return;
	uint16_t iy = _item_y(d, idx);
	if (iy + UI_DRAWER_ITEM_HEIGHT > cfg->height) return;
	LTDC_LayerDrawRect(cfg, d->x + _SURFACE_MARGIN, iy,
		UI_DRAWER_WIDTH - _SURFACE_MARGIN * 2,
		UI_DRAWER_ITEM_HEIGHT, d->color_surface);
	_draw_item(d, idx, cfg);
}
