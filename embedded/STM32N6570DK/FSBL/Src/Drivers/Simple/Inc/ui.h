#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "simple_ltdc.h"

#define UI_MAX_OBJECTS     16

#define UI_COLOR_YELLOW       0xFFFF00U
#define UI_COLOR_YELLOW_DARK  0xC89600U
#define UI_COLOR_BLACK        0x000000U

typedef enum {
    UI_STATE_IDLE = 0,
    UI_STATE_PRESSED,
} UI_State_TypeDef;

typedef void (*UI_Callback_TypeDef)(void);

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

#endif
