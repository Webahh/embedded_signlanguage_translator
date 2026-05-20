#include "main.h"
#include "gpio_driver.h"
#include "smpte_163x200_YUV420_FullPlanar.h"

LTDC_HandleTypeDef hltdc;

static gpio_pin_t led1 = { .port = LED1_GPIO_Port, .pin = LED1_Pin };

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LTDC_Init(void);

int main(void)
{
  /* Enable SRAM3 & SRAM4 clocks (framebuffer at 0x34200000 spans both) */
  RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN;
  (void)RCC->MEMENR;

  /* Configure MPU: framebuffer region (0x34200000, 1 MB) as non-cacheable */
  ARM_MPU_Disable();
  ARM_MPU_SetMemAttr(0, 0x44U);
  ARM_MPU_SetRegion(0,
      ARM_MPU_RBAR(0x34200000U, ARM_MPU_SH_OUTER, 0, 0, 0),
      ARM_MPU_RLAR(0x342FFFFFU, 0));
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

  SCB_EnableICache();
  SCB_EnableDCache();

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  MX_LTDC_Init();

  gpio_init_output(&led1);

  while (1)
  {
    gpio_toggle_pin(&led1);
    HAL_Delay(100);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable HSI */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL4.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
  if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
     (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11))
  {
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
    RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 4;
  RCC_OscInitStruct.PLL1.PLLN = 75;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL1.PLLP1 = 1;
  RCC_OscInitStruct.PLL1.PLLP2 = 1;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
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

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_CPUCLK|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_PCLK5
                              |RCC_CLOCKTYPE_PCLK4;
  RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_IC1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
  RCC_ClkInitStruct.IC1Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC1Selection.ClockDivider = 2;
  RCC_ClkInitStruct.IC2Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC2Selection.ClockDivider = 3;
  RCC_ClkInitStruct.IC6Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC6Selection.ClockDivider = 4;
  RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
  RCC_ClkInitStruct.IC11Selection.ClockDivider = 3;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_LTDC_Init(void)
{
  LTDC_LayerFlexYUVFullPlanarTypeDef pLayerFlexYUVFullPlanar = {0};

  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 4;
  hltdc.Init.VerticalSync = 4;
  hltdc.Init.AccumulatedHBP = 12;
  hltdc.Init.AccumulatedVBP = 12;
  hltdc.Init.AccumulatedActiveW = 812;
  hltdc.Init.AccumulatedActiveH = 492;
  hltdc.Init.TotalWidth = 820;
  hltdc.Init.TotalHeigh = 500;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerFlexYUVFullPlanar.Layer.WindowX0 = 0;
  pLayerFlexYUVFullPlanar.Layer.WindowX1 = 200;
  pLayerFlexYUVFullPlanar.Layer.WindowY0 = 0;
  pLayerFlexYUVFullPlanar.Layer.WindowY1 = 163;
  pLayerFlexYUVFullPlanar.Layer.Alpha = 255;
  pLayerFlexYUVFullPlanar.Layer.Alpha0 = 0;
  pLayerFlexYUVFullPlanar.Layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerFlexYUVFullPlanar.Layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerFlexYUVFullPlanar.Layer.ImageWidth = 200;
  pLayerFlexYUVFullPlanar.Layer.ImageHeight = 163;
  pLayerFlexYUVFullPlanar.Layer.Backcolor.Blue = 0;
  pLayerFlexYUVFullPlanar.Layer.Backcolor.Green = 0;
  pLayerFlexYUVFullPlanar.Layer.Backcolor.Red = 0;
  pLayerFlexYUVFullPlanar.FlexYUV.YUVOrder = LTDC_YUV_ORDER_CHROMINANCE_FIRST;
  pLayerFlexYUVFullPlanar.FlexYUV.LuminanceOrder = LTDC_YUV_LUMINANCE_ORDER_EVEN_FIRST;
  pLayerFlexYUVFullPlanar.FlexYUV.ChrominanceOrder = 0;
  pLayerFlexYUVFullPlanar.FlexYUV.LuminanceRescale = LTDC_YUV_LUMINANCE_RESCALE_DISABLE;
  pLayerFlexYUVFullPlanar.YUVFullPlanarAddress.YAddress = (uint32_t) smpte_163x200_YUV420_FullPlanar;
  pLayerFlexYUVFullPlanar.YUVFullPlanarAddress.UAddress = (uint32_t)smpte_163x200_YUV420_FullPlanar + (width*height);
  pLayerFlexYUVFullPlanar.YUVFullPlanarAddress.VAddress = (uint32_t)smpte_163x200_YUV420_FullPlanar + (width*height) + (width*height >> 2);
  pLayerFlexYUVFullPlanar.ColorConverter = LTDC_YUV2RGBCONVERTOR_BT709_FULL_RANGE;
  if (HAL_LTDC_ConfigLayerFlexYUVFullPlanar(&hltdc, &pLayerFlexYUVFullPlanar, 0) != HAL_OK)
  {
    Error_Handler();
  }
  RIMC_MasterConfig_t RIMC_master = {0};
  RIMC_master.MasterCID = RIF_CID_1;
  RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;
  HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_LTDC1 , &RIMC_master);

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LTDCL1 , RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOQ_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOQ, LCD_BL_CTRL_Pin|LCD_ONOFF_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin|LCD_ONOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOQ, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
