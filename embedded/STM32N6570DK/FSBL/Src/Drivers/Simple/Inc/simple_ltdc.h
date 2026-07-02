/**
 * @file    simple_ltdc.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   LTDC display controller driver header
 *
 * Usage
 * -----
 * 1. LTDC_Init()				- initialise LTDC peripheral
 * 2. LTDC_ConfigLayer1/2()		- configure layer parameters
 * 3. LTDC_UpdateLayerAddress() - update framebuffer pointer
 * 4. LTDC_FillLayer()			- fill layer with colour
 */

#ifndef SIMPLE_LTDC_H
#define SIMPLE_LTDC_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"

#include "simple_ltdc_layer.h"
#include "simple_ltdc_color.h"
#include "simple_ltdc_layer_draw.h"

typedef enum {
    LTDC_OK    = 0,
    LTDC_ERROR = 1
} LTDC_Status_TypeDef;

/**
 * @brief Initialise the LTDC peripheral and display
 */
void LTDC_Init(void);

/**
 * @brief Set the background colour
 *
 * @param [in] r Red component (0-255)
 * @param [in] g Green component (0-255)
 * @param [in] b Blue component (0-255)
 */
void LTDC_BackgroundColor_Set(uint8_t r, uint8_t g, uint8_t b);

void LTDC_BlitRGB888ToARGB4444(
    const LTDC_Layer_Config_TypeDef *cfg,
    const uint8_t *source,
    uint16_t source_width,
    uint16_t source_height,
    uint16_t destination_x,
    uint16_t destination_y
);

#endif /* SIMPLE_LTDC_H */
