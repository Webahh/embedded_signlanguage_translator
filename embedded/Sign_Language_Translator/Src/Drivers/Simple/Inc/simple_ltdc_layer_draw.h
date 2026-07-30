/**
 * @file simple_ltdc_layer_draw.h
 * @author Groß
 * @date 02.07.2026
 * @brief Drawing primitives for LTDC layers (fill, rect, circle, blit, landmarks)
 *
 * Usage
 * -----
 * Methods can only be used on LTDC layers when the layer is initialized before.
 * All methods take in ARGB pixel colors convert them into layer appropriate representation
 * and writes the desired pattern
 */
#ifndef SIMPLE_LTDC_LAYER_DRAW_H
#define SIMPLE_LTDC_LAYER_DRAW_H

#include "simple_ltdc_layer.h"
#include "simple_ltdc_color.h"
#include "palm_detection.h"
#include "hand_landmark.h"

/**
 * @brief Fill an entire layer with a single colour
 *
 * @param [in] cfg   Layer configuration
 * @param [in] color ARGB fill colour
 *
 * @note D-cache: caller must clean the entire cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_Fill(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color);

/**
 * @brief Fill a layer with two colours side by side
 *
 * @param [in] cfg    Layer configuration
 * @param [in] color1 Left-side ARGB colour
 * @param [in] color2 Right-side ARGB colour
 *
 * @note D-cache: caller must clean cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_Fill_2Sides(const LTDC_Layer_Config_TypeDef *cfg, uint32_t color1, uint32_t color2);

/**
 * @brief Draw a filled circle on a layer using Bresenham's algorithm
 *
 * @param [in] cfg    Layer configuration
 * @param [in] pos_x  Circle center X (pixels)
 * @param [in] pos_y  Circle center Y (pixels)
 * @param [in] radius Circle radius (pixels)
 * @param [in] color  ARGB fill colour
 *
 * @note D-cache: caller must clean the circle bounding box on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_Circle(const LTDC_Layer_Config_TypeDef* cfg, uint16_t pos_x, uint16_t pos_y, uint16_t radius, uint32_t color);

/**
 * @brief Draw a filled rectangle on a layer
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB fill colour
 *
 * @note D-cache: caller must clean the rect on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_Rect(const LTDC_Layer_Config_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Draw a 1-pixel border of a rectangle
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x     Top-left X (pixels)
 * @param [in] y     Top-left Y (pixels)
 * @param [in] w     Width (pixels)
 * @param [in] h     Height (pixels)
 * @param [in] color ARGB border colour
 *
 * @note D-cache: caller must clean the border region on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_RectBorder(const LTDC_Layer_Config_TypeDef *cfg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);

/**
 * @brief Blit an image onto a layer at a destination position
 *
 * @param [in] cfg   Layer configuration
 * @param [in] img   Source image data
 * @param [in] img_w Source image width (pixels)
 * @param [in] img_h Source image height (pixels)
 * @param [in] dst_x Destination X offset (pixels)
 * @param [in] dst_y Destination Y offset (pixels)
 *
 * @note D-cache: caller must clean the destination region on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_BlitImage(const LTDC_Layer_Config_TypeDef *cfg, const void *img, uint16_t img_w, uint16_t img_h, uint16_t dst_x, uint16_t dst_y);


/**
 * @brief Draw a line between two points on a layer
 *
 * @param [in] cfg   Layer configuration
 * @param [in] x0    Start X (pixels)
 * @param [in] y0    Start Y (pixels)
 * @param [in] x1    End X (pixels)
 * @param [in] y1    End Y (pixels)
 * @param [in] color ARGB line colour
 *
 * @note D-cache: caller must clean the line bounding box on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_Line(const LTDC_Layer_Config_TypeDef *cfg, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

/**
 * @brief Draw hand landmarks directly onto a layer config
 *
 * @param [in] cfg    Target layer configuration
 * @param [in] points Array of landmark points
 *
 * @note D-cache: caller must clean the landmark region on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_LandmarksDirect(const LTDC_Layer_Config_TypeDef *cfg, const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT]);

/**
 * @brief Draw hand landmarks directly with custom line and point colours
 *
 * @param [in] cfg        Target layer configuration
 * @param [in] points     Array of landmark points
 * @param [in] line_color ARGB colour for connection lines
 * @param [in] point_color ARGB colour for landmark dots
 *
 * @note D-cache: caller must clean the landmark region on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_LandmarksDirectAlpha(const LTDC_Layer_Config_TypeDef *cfg, const LandmarkPoint_TypeDef points[LANDMARK_POINT_COUNT], uint32_t line_color, uint32_t point_color);

/**
 * @brief Draw a hand ROI bounding box directly onto a layer config
 *
 * @param [in] cfg   Target layer configuration
 * @param [in] roi   Hand ROI data
 * @param [in] color ARGB border colour
 *
 * @note D-cache: caller must clean the ROI region on cfg->fb (SCB_CleanDCache_by_Addr) before LTDC reads it.
 */
void LTDC_Layer_Draw_ROIDirect(const LTDC_Layer_Config_TypeDef *cfg, const HandROI_TypeDef *roi, uint32_t color);

#endif /* SIMPLE_LTDC_LAYER_DRAW_H */
