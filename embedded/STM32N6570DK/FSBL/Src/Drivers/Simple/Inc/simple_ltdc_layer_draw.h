/**
 * @file simple_ltdc_layer_draw.h
 * @author Groß
 * @date 02.07.2026
 * @brief
 *
 * Usage
 * -----
 * Methods can only be used on LTDC layers when the layer is initialized before.
 * All methods take in ARGB pixel colors convert them into layer appropriate representation
 * and writes the desired pattern
 */
#ifndef LTDC_LAYER_DRAW_H
#define LTDC_LAYER_DRAW_H

/**
 * @brief Get bytes per pixel for a layer configuration
 *
 * @param [in]  cfg Layer configuration
 * @param [out] bpp Bytes per pixel
 *
 * @retval LTDC_OK    Success
 * @retval LTDC_ERROR Unknown pixel format or missing flexible format descriptor
 */
LTDC_Status_TypeDef LTDC_BytesPerPixel(const LTDC_LayerConfig_TypeDef *cfg, int *bpp);

/**
 * @brief Convert an ARGB colour to the native pixel value for a layer
 *
 * @param [in]  cfg   Layer configuration
 * @param [in]  color ARGB colour value
 * @param [out] pixel Native pixel value
 *
 * @retval LTDC_OK    Success
 * @retval LTDC_ERROR Conversion failed
 */
LTDC_Status_TypeDef LTDC_ColorToPixel(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color, uint32_t *pixel);

/**
 * @brief Fill an entire layer with a single colour
 *
 * @param [in] cfg   Layer configuration
 * @param [in] color ARGB fill colour
 */
void LTDC_LayerFill(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color);

/**
 * @brief Fill a layer with two colours side by side
 *
 * @param [in] cfg    Layer configuration
 * @param [in] color1 Left-side ARGB colour
 * @param [in] color2 Right-side ARGB colour
 */
void LTDC_LayerFill2Sides(const LTDC_LayerConfig_TypeDef *cfg, uint32_t color1, uint32_t color2);

/**
 * @brief Draws circle on Layer at position (x,y) with color and radius
 *
 * @param [in] cfg		Layer configuration
 * @param [in] pos_x	Circle center position x
 * @param [in] pos_y	Circle center position y
 * @param [in] radius	radius in pixels
 * @param [in] color	color of the circle
 */
void LTDC_LayerDrawCricle(const LTDC_LayerConfig_TypeDef* cfg, uint16_t pos_x, uint16_t pos_y, uint16_t radius, uint32_t color);

/**
 * @brief Draw a filled rectangle on a layer
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB fill colour
 */
void LTDC_LayerDrawRect(const LTDC_LayerConfig_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Draw a 1-pixel border of a rectangle
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB border colour
 */
void LTDC_LayerDrawRectBorder(const LTDC_LayerConfig_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Blit an image onto a layer at a destination position
 *
 * @param [in] cfg   Layer configuration
 * @param [in] img   Source image data
 * @param [in] img_w Source image width (pixels)
 * @param [in] img_h Source image height (pixels)
 * @param [in] dst_x Destination X offset (pixels)
 * @param [in] dst_y Destination Y offset (pixels)
 */
void LTDC_BlitImage(const LTDC_LayerConfig_TypeDef *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y);


#endif /* LTDC_LAYER_DRAW_H */
