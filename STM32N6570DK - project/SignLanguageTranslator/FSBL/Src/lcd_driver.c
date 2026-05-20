/**
  ******************************************************************************
  * @file    lcd_driver.c
  * @author  Oliver Groß
  * @brief   LCD display driver implementation for RK050HR18 panel via LTDC.
  *          Initialization, clock configuration, pixel/fill/line primitives,
  *          and test pattern generation.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lcd_driver.h"
#include "main.h"

LCD_Ctx_t LcdCtx;

static void LCD_MspInit(LTDC_HandleTypeDef *hltdc);
static void LCD_ClockConfig(void);
static uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b);

/**
  * @brief  Initialize the LCD panel and LTDC peripheral.
  *         Configures timing, pixel format (RGB565), layer 0 framebuffer at
  *         @ref LCD_LAYER_0_ADDR, and enables the display.
  * @retval None
  */
void LCD_Init(void)
{
    LTDC_LayerCfgTypeDef layer_cfg = {0};

    LcdCtx.width = LCD_WIDTH;
    LcdCtx.height = LCD_HEIGHT;
    LcdCtx.active_layer = LCD_LAYER_0;
    LcdCtx.pixel_format = LCD_FORMAT_RGB565;
    LcdCtx.bpp = 2;

    LcdCtx.hltdc.Instance = LTDC;

    LCD_MspInit(&LcdCtx.hltdc);
    LCD_ClockConfig();

    LcdCtx.hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    LcdCtx.hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    LcdCtx.hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    LcdCtx.hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

    LcdCtx.hltdc.Init.HorizontalSync     = LCD_HSYNC - 1;
    LcdCtx.hltdc.Init.AccumulatedHBP     = LCD_HSYNC + LCD_HBP - 1;
    LcdCtx.hltdc.Init.AccumulatedActiveW = LCD_HSYNC + LCD_WIDTH + LCD_HBP - 1;
    LcdCtx.hltdc.Init.TotalWidth         = LCD_HSYNC + LCD_WIDTH + LCD_HBP + LCD_HFP - 1;
    LcdCtx.hltdc.Init.VerticalSync       = LCD_VSYNC - 1;
    LcdCtx.hltdc.Init.AccumulatedVBP     = LCD_VSYNC + LCD_VBP - 1;
    LcdCtx.hltdc.Init.AccumulatedActiveH = LCD_VSYNC + LCD_HEIGHT + LCD_VBP - 1;
    LcdCtx.hltdc.Init.TotalHeigh         = LCD_VSYNC + LCD_HEIGHT + LCD_VBP + LCD_VFP - 1;

    LcdCtx.hltdc.Init.Backcolor.Blue  = 0;
    LcdCtx.hltdc.Init.Backcolor.Green = 0;
    LcdCtx.hltdc.Init.Backcolor.Red   = 0;

    if (HAL_LTDC_Init(&LcdCtx.hltdc) != HAL_OK)
    {
        Error_Handler();
    }

    layer_cfg.WindowX0      = 0;
    layer_cfg.WindowX1      = LCD_WIDTH;
    layer_cfg.WindowY0      = 0;
    layer_cfg.WindowY1      = LCD_HEIGHT;
    layer_cfg.PixelFormat   = LTDC_PIXEL_FORMAT_RGB565;
    layer_cfg.Alpha         = 255;
    layer_cfg.Alpha0        = 0;
    layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    layer_cfg.FBStartAdress = LCD_LAYER_0_ADDR;
    layer_cfg.ImageWidth    = LCD_WIDTH;
    layer_cfg.ImageHeight   = LCD_HEIGHT;
    layer_cfg.Backcolor.Blue  = 0;
    layer_cfg.Backcolor.Green = 0;
    layer_cfg.Backcolor.Red   = 0;

    if (HAL_LTDC_ConfigLayer(&LcdCtx.hltdc, &layer_cfg, LCD_LAYER_0) != HAL_OK)
    {
        Error_Handler();
    }

    LCD_DisplayOn();
}

/**
  * @brief  Enable the LCD display: start LTDC and assert display enable pin.
  * @retval None
  */
void LCD_DisplayOn(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_LTDC_ENABLE(&LcdCtx.hltdc);

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin  = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &gpio);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
  * @brief  Disable the LCD display: display disable and stop LTDC.
  * @retval None
  */
void LCD_DisplayOff(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
    gpio.Pin  = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &gpio);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);

    __HAL_LTDC_DISABLE(&LcdCtx.hltdc);
}

/**
  * @brief  Select the active layer for drawing operations.
  * @param  layer: layer index (@ref LCD_LAYER_0 or @ref LCD_LAYER_1)
  * @retval None
  */
void LCD_SetLayer(uint32_t layer)
{
    if (layer <= LCD_LAYER_1)
    {
        LcdCtx.active_layer = layer;
    }
}

/**
  * @brief  Draw a single pixel at (x, y) on the active layer.
  * @param  x: horizontal coordinate
  * @param  y: vertical coordinate
  * @param  color: 16-bit RGB565 color value
  * @retval None
  */
void LCD_DrawPixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t addr;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    addr = LcdCtx.hltdc.LayerCfg[LcdCtx.active_layer].FBStartAdress
           + (LcdCtx.bpp * ((LcdCtx.width * y) + x));

    *(__IO uint16_t *)addr = (uint16_t)color;
}

/**
  * @brief  Read a single pixel color from the active layer at (x, y).
  * @param  x: horizontal coordinate
  * @param  y: vertical coordinate
  * @param  color: pointer to store the 16-bit RGB565 color value
  * @retval None
  */
void LCD_ReadPixel(uint32_t x, uint32_t y, uint32_t *color)
{
    uint32_t addr;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || !color) return;

    addr = LcdCtx.hltdc.LayerCfg[LcdCtx.active_layer].FBStartAdress
           + (LcdCtx.bpp * ((LcdCtx.width * y) + x));

    *color = *(__IO uint16_t *)addr;
}

/**
  * @brief  Fill a rectangular region with a solid color on the active layer.
  * @param  x: left edge coordinate
  * @param  y: top edge coordinate
  * @param  w: rectangle width
  * @param  h: rectangle height
  * @param  color: 16-bit RGB565 fill color
  * @retval None
  */
void LCD_FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    uint32_t i, j;
    uint16_t *fb;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;

    fb = (uint16_t *)(LcdCtx.hltdc.LayerCfg[LcdCtx.active_layer].FBStartAdress);

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            fb[(y + j) * LcdCtx.width + (x + i)] = (uint16_t)color;
        }
    }
}

/**
  * @brief  Draw a horizontal line on the active layer.
  * @param  x: starting x coordinate
  * @param  y: y coordinate
  * @param  len: line length in pixels
  * @param  color: 16-bit RGB565 color
  * @retval None
  */
void LCD_DrawHLine(uint32_t x, uint32_t y, uint32_t len, uint32_t color)
{
    uint32_t i;
    uint16_t *fb;

    if (y >= LCD_HEIGHT || x >= LCD_WIDTH) return;
    if (x + len > LCD_WIDTH) len = LCD_WIDTH - x;

    fb = (uint16_t *)(LcdCtx.hltdc.LayerCfg[LcdCtx.active_layer].FBStartAdress);

    for (i = 0; i < len; i++)
    {
        fb[y * LcdCtx.width + (x + i)] = (uint16_t)color;
    }
}

/**
  * @brief  Draw a vertical line on the active layer.
  * @param  x: x coordinate
  * @param  y: starting y coordinate
  * @param  len: line length in pixels
  * @param  color: 16-bit RGB565 color
  * @retval None
  */
void LCD_DrawVLine(uint32_t x, uint32_t y, uint32_t len, uint32_t color)
{
    uint32_t i;
    uint16_t *fb;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (y + len > LCD_HEIGHT) len = LCD_HEIGHT - y;

    fb = (uint16_t *)(LcdCtx.hltdc.LayerCfg[LcdCtx.active_layer].FBStartAdress);

    for (i = 0; i < len; i++)
    {
        fb[(y + i) * LcdCtx.width + x] = (uint16_t)color;
    }
}

/**
  * @brief  Draw a bilinear four-corner gradient test pattern.
  *         Color mapping:
  *           TL = blue   (0,   0,   255)
  *           TR = cyan   (0,   255, 255)  (turquoise)
  *           BL = red    (255, 0,   0)
  *           BR = yellow (255, 255, 0)
  * @note   Each pixel is computed by interpolating horizontally between the
  *         top corners and bottom corners, then vertically between the results.
  *         Written directly to the framebuffer at @ref LCD_LAYER_0_ADDR.
  * @retval None
  */
void LCD_ShowTestPattern(void)
{
    uint32_t x, y;
    uint16_t *fb = (uint16_t *)LCD_LAYER_0_ADDR;

    for (y = 0; y < LCD_HEIGHT; y++)
    {
        for (x = 0; x < LCD_WIDTH; x++)
        {
            float fx = (float)x / (LCD_WIDTH - 1);
            float fy = (float)y / (LCD_HEIGHT - 1);

            float r_top = 0.0f * (1.0f - fx) + 0.0f * fx;
            float g_top = 0.0f * (1.0f - fx) + 255.0f * fx;
            float b_top = 255.0f * (1.0f - fx) + 255.0f * fx;

            float r_bot = 255.0f * (1.0f - fx) + 255.0f * fx;
            float g_bot = 0.0f * (1.0f - fx) + 255.0f * fx;
            float b_bot = 0.0f * (1.0f - fx) + 0.0f * fx;

            uint8_t r = (uint8_t)(r_top * (1.0f - fy) + r_bot * fy);
            uint8_t g = (uint8_t)(g_top * (1.0f - fy) + g_bot * fy);
            uint8_t b = (uint8_t)(b_top * (1.0f - fy) + b_bot * fy);

            fb[y * LCD_WIDTH + x] = RGB565(r, g, b);
        }
    }
}

/**
  * @brief  LTDC MSP initialization: clock enable, GPIO alternate-function
  *         configuration for LTDC pins, and display control pin setup.
  * @param  hltdc: LTDC handle pointer
  * @retval None
  */
static void LCD_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef gpio = {0};

    if (hltdc->Instance != LTDC) return;

    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOQ_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_15;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_15;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_11;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOG, &gpio);

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6;
    gpio.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOQ, &gpio);

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOG, &gpio);

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOQ, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOQ, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
  * @brief  Configure PLL4 to generate 25 MHz pixel clock for LTDC
  *         via IC16 with divider 2.
  * @note   Clock source: HSI -> PLL4 -> IC16 (div 2) -> LTDC
  * @retval None
  */
static void LCD_ClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL4.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL4.PLLM = 1;
    RCC_OscInitStruct.PLL4.PLLN = 25;
    RCC_OscInitStruct.PLL4.PLLFractional = 0;
    RCC_OscInitStruct.PLL4.PLLP1 = 1;
    RCC_OscInitStruct.PLL4.PLLP2 = 1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    RCC_PeriphCLKInitStruct.LtdcClockSelection = RCC_LTDCCLKSOURCE_IC16;
    RCC_PeriphCLKInitStruct.ICSelection[RCC_IC16].ClockSelection = RCC_ICCLKSOURCE_PLL4;
    RCC_PeriphCLKInitStruct.ICSelection[RCC_IC16].ClockDivider = 2;
    if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  Pack 8-bit R/G/B components into a 16-bit RGB565 value.
  * @param  r: red component (0-255)
  * @param  g: green component (0-255)
  * @param  b: blue component (0-255)
  * @retval RGB565 color value
  */
static uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
