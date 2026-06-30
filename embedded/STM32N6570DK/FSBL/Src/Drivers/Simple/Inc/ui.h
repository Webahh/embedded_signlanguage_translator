/**
 * @file    ui.h
 * @author  Groß
 * @date    23.06.2026
 * @brief   Simple UI: slide-out drawer with toggle / slider / selector / composite items
 *
 * Usage
 * -----
 * 1. UI_Drawer_Init(...)      					- initialise drawer with two framebuffers
 * 2. UI_Drawer_AddItem(...)   					- populate drawer items
 * 3. UI_Drawer_Prepare(...)   					- pre-render both open/closed states
 * 4. UI_Drawer_HandleTouch    					- dispatch touch
 * 5. UI_Drawer_Draw           					- render (Prepare handles initial draw)
 */

#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

#include "stm32n657xx.h"

#include "simple_ltdc.h"

// -------------------------------------------------------------------------
// Status codes
// -------------------------------------------------------------------------

typedef enum {
	UI_OK            =  0,
	UI_ERR_FULL      = -1,
	UI_ERR_NOT_FOUND = -2,
	UI_ERR_RANGE     = -3,
} UI_Status_TypeDef;

// -------------------------------------------------------------------------
// Drawer System
// -------------------------------------------------------------------------

#define UI_DRAWER_MAX_ITEMS    12
#define UI_DRAWER_WIDTH        220
#define UI_DRAWER_BTN_SIZE     44
#define UI_DRAWER_ITEM_HEIGHT  46
#define UI_DRAWER_ITEM_GAP      4

/** @brief Types of items that can be placed in the drawer */
typedef enum {
	UI_DRAWER_ITEM_LABEL = 0,
	UI_DRAWER_ITEM_TOGGLE,
	UI_DRAWER_ITEM_SLIDER,
	UI_DRAWER_ITEM_SELECTOR,
	UI_DRAWER_ITEM_COMPOSITE,
} UI_DrawerItemType_TypeDef;

/** @brief Callback on drawer item value change */
typedef void (*UI_DrawerItemCallback_TypeDef)(uint8_t item_idx,
	uint8_t value, void *context);

/** @brief Extra state for UI_DRAWER_ITEM_COMPOSITE */
typedef struct {
	uint8_t visible;
	uint8_t slider_value;
} UI_Composite_TypeDef;

/** @brief A single drawer item (label, toggle, slider, selector, composite) */
typedef struct {
	UI_DrawerItemType_TypeDef type;
	char label[24];
	uint8_t value;
	UI_Composite_TypeDef composite;
	const char **seg_labels;
	uint8_t seg_count;
	UI_DrawerItemCallback_TypeDef callback;
	void *context;
} UI_DrawerItem_TypeDef;

/** @brief Slide-out drawer instance */
typedef struct {
	uint16_t x_pos;
	uint16_t y_pos;
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

/**
 * @brief Initialise a drawer instance with two framebuffers
 *
 * @param [in] drawer     | Pointer to drawer struct
 * @param [in] buf_open   | Framebuffer for open state
 * @param [in] buf_closed | Framebuffer for closed state
 */
void UI_Drawer_Init(UI_Drawer_TypeDef *drawer, uint8_t *buf_open, uint8_t *buf_closed);

/**
 * @brief Add an item to the drawer
 *
 * @param [in]  drawer  | Drawer instance
 * @param [in]  type    | Item type (label / toggle / slider / selector / composite)
 * @param [in]  label   | Text label (max 23 chars)
 * @param [in]  cb      | Callback on value change (may be NULL)
 * @param [in]  context | User context pointer passed to callback (may be NULL)
 * @param [out] out_idx | Receives index of new item (may be NULL)
 *
 * @retval UI_OK       Item added
 * @retval UI_ERR_FULL Drawer item array full
 */
UI_Status_TypeDef UI_Drawer_AddItem(UI_Drawer_TypeDef *drawer,
	UI_DrawerItemType_TypeDef type, const char *label,
	UI_DrawerItemCallback_TypeDef cb, void *context, int *out_idx);

/**
 * @brief Pre-render both open and closed states into their framebuffers
 *
 * @param [in] drawer | Drawer instance
 * @param [in] cfg    | LTDC layer config for dimensions and format
 */
void UI_Drawer_Prepare(UI_Drawer_TypeDef *drawer, LTDC_LayerConfig_TypeDef *cfg);

/**
 * @brief Toggle drawer open/closed (swaps framebuffer pointer)
 *
 * @param [in] drawer | Drawer instance
 * @param [in] cfg    | LTDC layer config
 */
void UI_Drawer_Toggle(UI_Drawer_TypeDef *drawer, LTDC_LayerConfig_TypeDef *cfg);

/**
 * @brief Dispatch touch event to drawer
 *
 * @param [in]  drawer  | Drawer instance
 * @param [in]  touch_x | Touch x
 * @param [in]  touch_y | Touch y
 * @param [in]  pressed | 1 = press, 0 = release
 * @param [in]  cfg     | LTDC layer config (needed to re-render touched items)
 * @param [out] out_idx | Receives item index that changed, or -2 if toggle
 *                        button was hit (may be NULL)
 *
 * @retval UI_OK          	An item was changed or the drawer was toggled
 * @retval UI_ERR_NOT_FOUND No interaction
 */
UI_Status_TypeDef UI_Drawer_HandleTouch(UI_Drawer_TypeDef *drawer,
	uint16_t touch_x, uint16_t touch_y, uint8_t pressed,
	LTDC_LayerConfig_TypeDef *cfg, int *out_idx);

/**
 * @brief Full redraw of the drawer
 *
 * @param [in] drawer | Drawer instance
 * @param [in] cfg    | LTDC layer config
 */
void UI_Drawer_Draw(UI_Drawer_TypeDef *drawer,
	const LTDC_LayerConfig_TypeDef *cfg);

/**
 * @brief Redraw a single drawer item
 *
 * @param [in] drawer | Drawer instance
 * @param [in] idx    | Item index
 * @param [in] cfg    | LTDC layer config
 */
void UI_Drawer_DrawItem(UI_Drawer_TypeDef *drawer, uint8_t idx, const LTDC_LayerConfig_TypeDef *cfg);

/**
 * @brief Read the current value of a drawer item
 *
 * @param [in]  drawer  | Drawer instance
 * @param [in]  idx     | Item index
 * @param [out] out_val | Receives item value (toggle 0/1, slider 0-100, selector 0-2)
 *
 * @retval UI_OK        Value written
 * @retval UI_ERR_RANGE Index out of range
 */
UI_Status_TypeDef UI_Drawer_GetItemValue(const UI_Drawer_TypeDef *drawer, uint8_t idx, uint8_t *out_val);

#endif /* UI_H */
