#ifndef SIMPLE_TEXT_H
#define SIMPLE_TEXT_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

#include "simple_ltdc.h"

#define TEXT_COLOR_WHITE   0x00FFFFFFU
#define TEXT_COLOR_BLACK   0x00000000U
#define TEXT_COLOR_RED     0x00FF0000U
#define TEXT_COLOR_GREEN   0x0000FF00U
#define TEXT_COLOR_BLUE    0x000000FFU
#define TEXT_COLOR_YELLOW  0x00FFFF00U
#define TEXT_COLOR_CYAN    0x0000FFFFU
#define TEXT_COLOR_MAGENTA 0x00FF00FFU

/**
 * @brief Draw a single character at a given position
 *
 * @param [in] cfg     | Layer configuration
 * @param [in] c       | Character to draw
 * @param [in] x       | X position (top-left)
 * @param [in] y       | Y position (top-left)
 * @param [in] fg_color| Foreground colour in ARGB8888
 */
void TEXT_Char_draw(const LCD_LayerConfig *cfg, char c, int16_t x, int16_t y, uint32_t fg_color);

/**
 * @brief Draw a null-terminated string
 *
 * @param [in] cfg     | Layer configuration
 * @param [in] str     | String to draw
 * @param [in] x       | Starting X position
 * @param [in] y       | Starting Y position
 * @param [in] fg_color| Foreground colour in ARGB8888
 */
void TEXT_String_draw(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color);

/**
 * @brief Draw a string with foreground / background colours
 *
 * @param [in] cfg     | Layer configuration
 * @param [in] str     | String to draw
 * @param [in] x       | Starting X position
 * @param [in] y       | Starting Y position
 * @param [in] fg_color| Foreground colour in ARGB8888
 * @param [in] bg_color| Background colour in ARGB8888
 */
void TEXT_StringBg_draw(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint32_t bg_color);

/**
 * @brief Draw a scaled string (nearest-neighbour)
 *
 * @param [in] cfg     | Layer configuration
 * @param [in] str     | String to draw
 * @param [in] x       | Starting X position
 * @param [in] y       | Starting Y position
 * @param [in] fg_color| Foreground colour in ARGB8888
 * @param [in] scale   | Scale factor (0/1 = unscaled, 2+ = scaled)
 */
void TEXT_StringScaled_draw(const LCD_LayerConfig *cfg, const char *str, int16_t x, int16_t y, uint32_t fg_color, uint8_t scale);

#endif /* SIMPLE_TEXT_H */
