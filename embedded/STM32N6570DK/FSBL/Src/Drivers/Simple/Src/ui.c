#include "ui.h"
#include "simple_text.h"
#include "config.h"

static UI_Object_TypeDef _objects[UI_MAX_OBJECTS];
static int _object_count = 0;

void UI_Init(void)
{
    _object_count = 0;
}

int UI_AddButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
    uint32_t color, UI_Callback_TypeDef cb)
{
    if (_object_count >= UI_MAX_OBJECTS) return -1;
    UI_Object_TypeDef *o = &_objects[_object_count++];
    o->x = x;
    o->y = y;
    o->w = w;
    o->h = h;
    o->state = UI_STATE_IDLE;
    o->color = color;
    o->color_pressed = UI_COLOR_YELLOW_DARK;
    o->callback = cb;
    return _object_count - 1;
}

int UI_HandleTouch(uint16_t tx, uint16_t ty, uint8_t pressed)
{
    static int active_idx = -1;

    if (pressed)
    {
        for (int i = 0; i < _object_count; i++)
        {
            UI_Object_TypeDef *o = &_objects[i];
            if (tx >= o->x && tx < o->x + o->w &&
                ty >= o->y && ty < o->y + o->h)
            {
                o->state = UI_STATE_PRESSED;
                active_idx = i;
                return i;
            }
        }
        active_idx = -1;
    }
    else
    {
        if (active_idx >= 0)
        {
            UI_Object_TypeDef *o = &_objects[active_idx];
            o->state = UI_STATE_IDLE;
            if (o->callback) {
                o->callback();
            }
            int idx = active_idx;
            active_idx = -1;
            return idx;
        }
    }
    return -1;
}

void UI_DrawAll(const LTDC_LayerConfig_TypeDef *cfg)
{
    for (int i = 0; i < _object_count; i++)
    {
        UI_Object_TypeDef *o = &_objects[i];
        uint16_t lx = o->x - cfg->x;
        uint16_t ly = o->y - cfg->y;
        uint32_t fill = (o->state == UI_STATE_PRESSED)
            ? o->color_pressed : o->color;
        LTDC_LayerDrawRect(cfg, lx, ly, o->w, o->h, fill);
        LTDC_LayerDrawRectBorder(cfg, lx, ly, o->w, o->h, 0x000000U);
    }
}

// -- Drawer System --

#define _DRAWER_ITEM_PAD_L   10
#define _DRAWER_ITEM_PAD_R   10
#define _DRAWER_SLIDER_H      8
#define _DRAWER_TOGGLE_W     44
#define _DRAWER_TOGGLE_H     24
#define _DRAWER_THUMB_R       7

void UI_Drawer_Init(UI_Drawer_TypeDef *d, uint8_t *buf_open, uint8_t *buf_closed)
{
    d->x = 0;
    d->y = 0;
    d->is_open = 0;
    d->color_bg    = 0x002B2B2BU;
    d->color_border= 0x00555555U;
    d->color_text  = 0x00FFFFFFU;
    d->color_accent= 0x0000DD55U;
    d->color_surface=0x00383838U;
    d->item_count = 0;
    d->active_item = -1;
    d->buf_open = buf_open;
    d->buf_closed = buf_closed;
}

void UI_Drawer_Prepare(UI_Drawer_TypeDef *d, const LTDC_LayerConfig_TypeDef *cfg)
{
    LTDC_LayerConfig_TypeDef tmp = *cfg;

    tmp.fb = d->buf_closed;
    LTDC_LayerFill(&tmp, LTDC_COLOR_BLACK);
    d->is_open = 0;
    UI_Drawer_Draw(d, &tmp);

    tmp.fb = d->buf_open;
    LTDC_LayerFill(&tmp, LTDC_COLOR_BLACK);
    d->is_open = 1;
    UI_Drawer_Draw(d, &tmp);

    d->is_open = 0;
    LTDC_Layer2Config.fb = d->buf_closed;
    LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

int UI_Drawer_AddItem(UI_Drawer_TypeDef *d, UI_DrawerItemType_TypeDef type,
                       const char *label, UI_DrawerItemCallback_TypeDef cb)
{
    if (d->item_count >= UI_DRAWER_MAX_ITEMS) return -1;
    UI_DrawerItem_TypeDef *it = &d->items[d->item_count];
    it->type = type;
    it->value = (type == UI_DRAWER_ITEM_TOGGLE || type == UI_DRAWER_ITEM_SELECTOR) ? 0 : 50;
    it->comp.visible = 1;
    it->comp.slider  = 50;
    it->callback = cb;
    it->context = NULL;

    uint8_t i;
    for (i = 0; i < sizeof(it->label) - 1 && label[i]; i++)
        it->label[i] = label[i];
    it->label[i] = '\0';

    return d->item_count++;
}

void UI_Drawer_Toggle(UI_Drawer_TypeDef *d)
{
    d->is_open = !d->is_open;
    LTDC_Layer2Config.fb = d->is_open ? d->buf_open : d->buf_closed;
    LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

void UI_Drawer_Open(UI_Drawer_TypeDef *d)
{
    d->is_open = 1;
    LTDC_Layer2Config.fb = d->buf_open;
    LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

void UI_Drawer_Close(UI_Drawer_TypeDef *d)
{
    d->is_open = 0;
    LTDC_Layer2Config.fb = d->buf_closed;
    LTDC_UpdateLayerAddress(&LTDC_Layer2Config);
}

uint8_t UI_Drawer_GetItemValue(const UI_Drawer_TypeDef *d, uint8_t idx)
{
    if (idx >= d->item_count) return 0;
    return d->items[idx].value;
}

static void _render_item_in_open_buf(UI_Drawer_TypeDef *d, uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg)
{
    LTDC_LayerConfig_TypeDef tmp = *cfg;
    tmp.fb = d->buf_open;
    UI_Drawer_DrawItem(d, idx, &tmp);
}

int UI_Drawer_HandleTouch(UI_Drawer_TypeDef *d, uint16_t tx, uint16_t ty, uint8_t pressed, const LTDC_LayerConfig_TypeDef *cfg)
{
    if (pressed)
    {
        /* Check button hit */
        if (tx >= d->x && tx < d->x + UI_DRAWER_BTN_SIZE &&
            ty >= d->y && ty < d->y + UI_DRAWER_BTN_SIZE)
        {
            d->active_item = -2;
            return -2;
        }

        /* Check items if open */
        if (d->is_open)
        {
            uint16_t item_y = d->y + UI_DRAWER_BTN_SIZE + 8;
            for (uint8_t i = 0; i < d->item_count; i++)
            {
                if (tx >= d->x + _DRAWER_ITEM_PAD_L &&
                    tx < d->x + UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_R &&
                    ty >= item_y && ty < item_y + UI_DRAWER_ITEM_HEIGHT)
                {
                    d->active_item = i;
                    return i;
                }
                item_y += UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP;
            }
        }
        d->active_item = -1;
    }
    else
    {
        if (d->active_item == -2)
        {
            d->active_item = -1;
            UI_Drawer_Toggle(d);
            return -2;
        }

        if (d->active_item >= 0 && (uint8_t)d->active_item < d->item_count)
        {
            UI_DrawerItem_TypeDef *it = &d->items[d->active_item];
            uint8_t idx = (uint8_t)d->active_item;

            if (it->type == UI_DRAWER_ITEM_TOGGLE)
            {
                it->value = !it->value;
                if (it->callback)
                    it->callback(idx, it->value, it->context);
                _render_item_in_open_buf(d, idx, cfg);
                d->active_item = -1;
                return 1;
            }
            else if (it->type == UI_DRAWER_ITEM_SLIDER)
            {
                uint16_t label_w = 0;
                for (uint8_t i = 0; it->label[i]; i++)
                    label_w += 8;
                uint16_t slider_x = d->x + _DRAWER_ITEM_PAD_L + label_w + 16;
                uint16_t slider_w = UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R - label_w - 16;
                if (slider_w < 10) slider_w = 10;

                int16_t rel_x;
                if (tx < slider_x) rel_x = 0;
                else if (tx >= slider_x + slider_w) rel_x = slider_w;
                else rel_x = tx - slider_x;

                it->value = (uint8_t)((uint32_t)rel_x * 100U / slider_w);
                if (it->value > 100) it->value = 100;

                if (it->callback)
                    it->callback(idx, it->value, it->context);
                _render_item_in_open_buf(d, idx, cfg);
                d->active_item = -1;
                return 1;
            }
            else if (it->type == UI_DRAWER_ITEM_SELECTOR)
            {
                uint16_t seg_x = d->x + _DRAWER_ITEM_PAD_L;
                uint16_t seg_w = (UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R) / 3;
                uint8_t s = (tx < seg_x) ? 0 : (tx - seg_x) / seg_w;
                if (s > 2) s = 2;
                it->value = s;
                if (it->callback)
                    it->callback(idx, it->value, it->context);
                _render_item_in_open_buf(d, idx, cfg);
                d->active_item = -1;
                return 1;
            }
            else if (it->type == UI_DRAWER_ITEM_COMPOSITE)
            {
                uint16_t cx = d->x + _DRAWER_ITEM_PAD_L;
                uint16_t aw = UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R;

                uint8_t ll = 0;
                for (uint8_t k = 0; it->label[k]; k++) ll++;
                uint16_t eye_x = cx + ll * 8 + 12;

                /* Hit-test eye icon */
                if (tx >= eye_x && tx < eye_x + 12)
                {
                    it->comp.visible = !it->comp.visible;
                    if (it->callback)
                        it->callback(idx, it->comp.visible, it->context);
                    _render_item_in_open_buf(d, idx, cfg);
                    d->active_item = -1;
                    return 1;
                }

                /* Hit-test slider */
                uint16_t slider_x = eye_x + 12 + 10;
                uint16_t slider_w = cx + aw - slider_x;
                if ((int16_t)slider_w > 20 && tx >= slider_x)
                {
                    int16_t rel = (tx >= slider_x + slider_w) ? slider_w : tx - slider_x;
                    if (rel < 0) rel = 0;
                    uint8_t val = (uint8_t)((uint32_t)rel * 100U / slider_w);
                    if (val > 100) val = 100;
                    it->comp.slider = val;
                    if (it->callback)
                        it->callback(idx, val, it->context);
                    _render_item_in_open_buf(d, idx, cfg);
                    d->active_item = -1;
                    return 1;
                }
            }

            d->active_item = -1;
        }
        d->active_item = -1;
    }
    return -1;
}

static void _draw_hamburger(const LTDC_LayerConfig_TypeDef *cfg,
                             int16_t bx, int16_t by, uint32_t color)
{
    uint16_t cx = (uint16_t)bx + (UI_DRAWER_BTN_SIZE / 2);
    uint16_t bar_w = 20;
    uint16_t bar_h = 3;
    uint16_t bar_x = cx - bar_w / 2;
    LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + 12, bar_w, bar_h, color);
    LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + 20, bar_w, bar_h, color);
    LTDC_LayerDrawRect(cfg, bar_x, (uint16_t)by + 28, bar_w, bar_h, color);
}

static void _draw_item(UI_Drawer_TypeDef *d, uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg)
{
    uint16_t iy = d->y + UI_DRAWER_BTN_SIZE + 8 + idx * (UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP);
    if (iy + UI_DRAWER_ITEM_HEIGHT > cfg->height)
        return;
    UI_DrawerItem_TypeDef *it = &d->items[idx];
    uint16_t lx = d->x + _DRAWER_ITEM_PAD_L;
    uint16_t ly = iy + (UI_DRAWER_ITEM_HEIGHT - 16) / 2;

    if (it->type == UI_DRAWER_ITEM_LABEL)
    {
        TEXT_String_draw(cfg, it->label, lx, ly, d->color_text);
    }
    else if (it->type == UI_DRAWER_ITEM_TOGGLE)
    {
        TEXT_String_draw(cfg, it->label, lx, ly, d->color_text);
        uint16_t tx = d->x + UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_R - _DRAWER_TOGGLE_W;
        uint16_t ty = iy + (UI_DRAWER_ITEM_HEIGHT - _DRAWER_TOGGLE_H) / 2;
        uint32_t tc = it->value ? d->color_accent : 0x00555555U;
        LTDC_LayerDrawRect(cfg, tx, ty, _DRAWER_TOGGLE_W, _DRAWER_TOGGLE_H, tc);
        LTDC_LayerDrawRectBorder(cfg, tx, ty, _DRAWER_TOGGLE_W, _DRAWER_TOGGLE_H, 0x00777777U);
        uint16_t cx = it->value ? tx + _DRAWER_TOGGLE_W - _DRAWER_THUMB_R - 3 : tx + _DRAWER_THUMB_R + 3;
        uint16_t cy = ty + _DRAWER_TOGGLE_H / 2;
        LTDC_LayerDrawCricle(cfg, cx, cy, _DRAWER_THUMB_R, d->color_text);
    }
    else if (it->type == UI_DRAWER_ITEM_SLIDER)
    {
        TEXT_String_draw(cfg, it->label, lx, ly, d->color_text);
        uint16_t lw = 0;
        for (uint8_t j = 0; it->label[j]; j++) lw += 8;
        uint16_t sx = lx + lw + 16;
        uint16_t sy = iy + (UI_DRAWER_ITEM_HEIGHT - _DRAWER_SLIDER_H) / 2;
        uint16_t sw = UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R - lw - 16;
        if ((int16_t)sw > 0)
        {
            LTDC_LayerDrawRect(cfg, sx, sy, sw, _DRAWER_SLIDER_H, 0x00555555U);
            uint16_t fw = (uint16_t)((uint32_t)sw * it->value / 100);
            if (fw > 0)
                LTDC_LayerDrawRect(cfg, sx, sy, fw, _DRAWER_SLIDER_H, d->color_accent);
            uint16_t cx = sx + fw;
            uint16_t cy = sy + _DRAWER_SLIDER_H / 2;
            LTDC_LayerDrawCricle(cfg, cx, cy, _DRAWER_THUMB_R, d->color_text);
        }
    }
    else if (it->type == UI_DRAWER_ITEM_SELECTOR)
    {
        const char *labels[3] = {"Palm", "Hand", "Sign"};
        uint16_t sx = d->x + _DRAWER_ITEM_PAD_L;
        uint16_t aw = UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R;
        uint16_t sw = aw / 3;
        uint16_t sh = 30;
        uint16_t sy = iy + (UI_DRAWER_ITEM_HEIGHT - sh) / 2;

        for (uint8_t s = 0; s < 3; s++)
        {
            uint16_t seg_x = sx + s * sw;
            uint32_t bg = (s == it->value) ? d->color_accent : 0x00444444U;
            LTDC_LayerDrawRect(cfg, seg_x, sy, sw - 2, sh, bg);
            LTDC_LayerDrawRectBorder(cfg, seg_x, sy, sw - 2, sh, 0x00777777U);

            uint8_t len = 0;
            for (uint8_t k = 0; labels[s][k]; k++) len++;
            uint16_t tx = seg_x + (sw - 2 - len * 8) / 2;
            uint16_t ty = sy + (sh - 16) / 2;
            TEXT_String_draw(cfg, labels[s], tx, ty, d->color_text);
        }
    }
    else if (it->type == UI_DRAWER_ITEM_COMPOSITE)
    {
        uint16_t cx = d->x + _DRAWER_ITEM_PAD_L;
        uint16_t aw = UI_DRAWER_WIDTH - _DRAWER_ITEM_PAD_L - _DRAWER_ITEM_PAD_R;
        uint16_t mid_y = iy + UI_DRAWER_ITEM_HEIGHT / 2;

        /* Label */
        TEXT_String_draw(cfg, it->label, cx, mid_y - 8, d->color_text);
        uint8_t ll = 0;
        for (uint8_t k = 0; it->label[k]; k++) ll++;

        /* Eye icon */
        uint16_t eye_x = cx + ll * 8 + 12;
        uint16_t eye_y = mid_y - 8;
        uint16_t eye_r = 6;
        if (it->comp.visible)
        {
            LTDC_LayerDrawCricle(cfg, eye_x + eye_r, eye_y + eye_r, eye_r, d->color_text);
            LTDC_LayerDrawCricle(cfg, eye_x + eye_r, eye_y + eye_r, 2, d->color_text);
        }
        else
        {
            LTDC_LayerDrawRect(cfg, eye_x + 2, eye_y + eye_r - 1, eye_r * 2 - 4, 2, d->color_text);
        }

        /* Slider */
        uint16_t slider_x = eye_x + eye_r * 2 + 10;
        uint16_t slider_w = cx + aw - slider_x;
        if ((int16_t)slider_w > 20)
        {
            uint16_t slider_y = mid_y - 4;
            uint16_t slider_h = 8;
            LTDC_LayerDrawRect(cfg, slider_x, slider_y, slider_w, slider_h, 0x00555555U);
            uint16_t fill_w = (uint16_t)((uint32_t)slider_w * it->comp.slider / 100);
            if (fill_w > 0)
                LTDC_LayerDrawRect(cfg, slider_x, slider_y, fill_w, slider_h, d->color_accent);
            LTDC_LayerDrawCricle(cfg, slider_x + fill_w, slider_y + slider_h / 2, 6, d->color_text);
        }
    }
}

void UI_Drawer_Draw(UI_Drawer_TypeDef *d, const LTDC_LayerConfig_TypeDef *cfg)
{
    /* Clear full drawer area to prevent stale pixels */
    LTDC_LayerDrawRect(cfg, d->x, d->y, UI_DRAWER_WIDTH, cfg->height - d->y, LTDC_COLOR_BLACK);

    if (d->is_open)
    {
        LTDC_LayerDrawRect(cfg, d->x, d->y, UI_DRAWER_WIDTH, cfg->height - d->y, d->color_bg);
        LTDC_LayerDrawRectBorder(cfg, d->x, d->y, UI_DRAWER_WIDTH, cfg->height - d->y, d->color_border);

        for (uint8_t i = 0; i < d->item_count; i++)
        {
            uint16_t iy = d->y + UI_DRAWER_BTN_SIZE + 8 + i * (UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP);
            if (iy + UI_DRAWER_ITEM_HEIGHT > cfg->height)
                break;
            LTDC_LayerDrawRect(cfg, d->x + 4, iy, UI_DRAWER_WIDTH - 8, UI_DRAWER_ITEM_HEIGHT, d->color_surface);
        }

        for (uint8_t i = 0; i < d->item_count; i++)
            _draw_item(d, i, cfg);
    }

    /* Draw button on top */
    uint32_t btn_color = 0x00444444U;
    LTDC_LayerDrawRect(cfg, d->x, d->y, UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, btn_color);
    LTDC_LayerDrawRectBorder(cfg, d->x, d->y, UI_DRAWER_BTN_SIZE, UI_DRAWER_BTN_SIZE, d->color_border);
    _draw_hamburger(cfg, (int16_t)d->x, (int16_t)d->y, d->color_text);
}

void UI_Drawer_DrawItem(UI_Drawer_TypeDef *d, uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg)
{
    if (idx >= d->item_count || !d->is_open)
        return;
    uint16_t item_y = d->y + UI_DRAWER_BTN_SIZE + 8 + idx * (UI_DRAWER_ITEM_HEIGHT + UI_DRAWER_ITEM_GAP);
    if (item_y + UI_DRAWER_ITEM_HEIGHT > cfg->height)
        return;
    LTDC_LayerDrawRect(cfg, d->x + 4, item_y, UI_DRAWER_WIDTH - 8, UI_DRAWER_ITEM_HEIGHT, d->color_surface);
    _draw_item(d, idx, cfg);
}
