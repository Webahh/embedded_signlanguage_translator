/**
  ******************************************************************************
  * @file    lcd_driver.h
  * @author  Oliver Groß
  * @brief   LCD display driver header
  *          This file provides defines and function prototypes for controlling
  *          the RK050HR18 800x480 LCD panel via the LTDC peripheral.
  ******************************************************************************
  */

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>
#include "stm32n6xx_hal.h"

/* Panel dimensions ---------------------------------------------------------*/
#define LCD_WIDTH           800U             /*!< LCD panel width in pixels  */
#define LCD_HEIGHT          480U             /*!< LCD panel height in pixels */

/* Timing parameters (RK050HR18) -------------------------------------------*/
#define LCD_HSYNC           4U               /*!< Horizontal sync length     */
#define LCD_HBP             4U               /*!< Horizontal back porch      */
#define LCD_HFP             4U               /*!< Horizontal front porch     */
#define LCD_VSYNC           4U               /*!< Vertical sync length       */
#define LCD_VBP             4U               /*!< Vertical back porch        */
#define LCD_VFP             4U               /*!< Vertical front porch       */

/* Pixel format identifiers ------------------------------------------------*/
#define LCD_FORMAT_RGB565   0U               /*!< RGB565 pixel format        */
#define LCD_FORMAT_RGB888   1U               /*!< RGB888 pixel format        */
#define LCD_FORMAT_ARGB8888 2U               /*!< ARGB8888 pixel format      */

/* Framebuffer addresses ---------------------------------------------------*/
#define LCD_LAYER_0_ADDR    0x34200000U      /*!< Layer 0 framebuffer in SRAM3_AXI */
#define LCD_LAYER_1_ADDR    0x32100000U      /*!< Layer 1 framebuffer address       */

/* Layer indices -----------------------------------------------------------*/
#define LCD_LAYER_0         0U               /*!< Layer 0 index               */
#define LCD_LAYER_1         1U               /*!< Layer 1 index               */

/**
  * @brief  LCD context structure
  */
typedef struct {
    uint32_t width;                          /*!< Active display width        */
    uint32_t height;                         /*!< Active display height       */
    uint32_t active_layer;                   /*!< Currently active layer      */
    uint32_t pixel_format;                   /*!< Active pixel format         */
    uint32_t bpp;                            /*!< Bytes per pixel             */
    LTDC_HandleTypeDef hltdc;                /*!< LTDC handle                 */
} LCD_Ctx_t;


extern LCD_Ctx_t LcdCtx;


void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_SetLayer(uint32_t layer);
void LCD_DrawPixel(uint32_t x, uint32_t y, uint32_t color);
void LCD_ReadPixel(uint32_t x, uint32_t y, uint32_t *color);
void LCD_FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void LCD_DrawHLine(uint32_t x, uint32_t y, uint32_t len, uint32_t color);
void LCD_DrawVLine(uint32_t x, uint32_t y, uint32_t len, uint32_t color);
void LCD_ShowTestPattern(void);

#endif /* LCD_DRIVER_H */
