#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "simple_ltdc.h"

#define UI_MAX_OBJECTS     16

#define UI_COLOR_YELLOW       0xFFFF00U
#define UI_COLOR_YELLOW_DARK  0xC89600U
#define UI_COLOR_BLACK        0x000000U

// Interactive State
typedef enum {
    UI_STATE_IDLE = 0,
    UI_STATE_PRESSED,
} UI_State_TypeDef;

// Callback on touch
typedef void (*UI_Callback_TypeDef)(void);

// Object
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    UI_State_TypeDef state;
    uint32_t color;
    uint32_t color_pressed;
    UI_Callback_TypeDef callback;
} UI_Object_TypeDef;

void UI_Init(void);
int  UI_AddButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color, UI_Callback_TypeDef cb);
int  UI_HandleTouch(uint16_t x, uint16_t y, uint8_t pressed);
void UI_DrawAll(const LTDC_LayerConfig_TypeDef *cfg);

// -- Drawer System --

#define UI_DRAWER_MAX_ITEMS    12
#define UI_DRAWER_WIDTH        220
#define UI_DRAWER_BTN_SIZE     44
#define UI_DRAWER_ITEM_HEIGHT  46
#define UI_DRAWER_ITEM_GAP      4

typedef enum {
    UI_DRAWER_ITEM_LABEL = 0,
    UI_DRAWER_ITEM_TOGGLE,
    UI_DRAWER_ITEM_SLIDER,
    UI_DRAWER_ITEM_SELECTOR,
    UI_DRAWER_ITEM_COMPOSITE,
} UI_DrawerItemType_TypeDef;

typedef void (*UI_DrawerItemCallback_TypeDef)(uint8_t item_idx, uint8_t value, void *context);

typedef struct {
    uint8_t visible;
    uint8_t slider;
} UI_Composite_TypeDef;

typedef struct {
    UI_DrawerItemType_TypeDef type;
    char label[24];
    uint8_t value;
    UI_Composite_TypeDef comp;
    UI_DrawerItemCallback_TypeDef callback;
    void *context;
} UI_DrawerItem_TypeDef;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t is_open;
    uint32_t color_bg;
    uint32_t color_border;
    uint32_t color_text;
    uint32_t color_accent;
    uint32_t color_surface;
    UI_DrawerItem_TypeDef items[UI_DRAWER_MAX_ITEMS];
    uint8_t item_count;
    int8_t active_item;
    uint8_t *buf_open;
    uint8_t *buf_closed;
} UI_Drawer_TypeDef;

void UI_Drawer_Init(UI_Drawer_TypeDef *drawer, uint8_t *buf_open, uint8_t *buf_closed);
int  UI_Drawer_AddItem(UI_Drawer_TypeDef *drawer, UI_DrawerItemType_TypeDef type, const char *label, UI_DrawerItemCallback_TypeDef cb);
void UI_Drawer_Prepare(UI_Drawer_TypeDef *drawer, const LTDC_LayerConfig_TypeDef *cfg);
void UI_Drawer_Toggle(UI_Drawer_TypeDef *drawer);
void UI_Drawer_Open(UI_Drawer_TypeDef *drawer);
void UI_Drawer_Close(UI_Drawer_TypeDef *drawer);
int  UI_Drawer_HandleTouch(UI_Drawer_TypeDef *drawer, uint16_t tx, uint16_t ty, uint8_t pressed, const LTDC_LayerConfig_TypeDef *cfg);
void UI_Drawer_Draw(UI_Drawer_TypeDef *drawer, const LTDC_LayerConfig_TypeDef *cfg);
void UI_Drawer_DrawItem(UI_Drawer_TypeDef *drawer, uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg);
uint8_t UI_Drawer_GetItemValue(const UI_Drawer_TypeDef *drawer, uint8_t item_idx);

#endif
