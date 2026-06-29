#include "ui.h"

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
            active_idx = -1;
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
