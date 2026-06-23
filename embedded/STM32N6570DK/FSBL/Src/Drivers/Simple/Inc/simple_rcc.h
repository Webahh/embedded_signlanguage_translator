/*
 * simple_rcc.h
 *
 *  Created on: May 22, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_RCC_H
#define SIMPLE_RCC_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "stm32n657xx.h"

#define RCC_HSI_VALUE_HZ  				64000000UL

#define RCC_LTDC_CLOCK_SOURCE_PCLK5     0U
#define RCC_LTDC_CLOCK_SOURCE_PER_CK    1U
#define RCC_LTDC_CLOCK_SOURCE_IC16_CK   2U
#define RCC_LTDC_CLOCK_SOURCE_HSI_DIV_CK 3U

typedef enum {
    RCC_OK    = 0,
    RCC_ERROR = 1
} RCC_Status_TypeDef;

typedef struct {
    uint32_t CFGR1;     /* PLLxCFGR1 value */
    uint32_t CFGR2;     /* PLLxCFGR2 value */
    uint32_t CFGR3;     /* PLLxCFGR3 value */
} RCC_PLL_cfg_TypeDef;

typedef struct {
    uint32_t CFGR;      /* ICxCFGR value */
} RCC_IC_cfg_TypeDef;

/**
 * @brief Configure a minimal system clock setup (HSI, prescalers /1)
 */
void RCC_SystemClock_Config(void);

/**
 * @brief Program all PLLs from configuration table
 *
 * @param [in] pll Array of 4 PLL configurations
 */
void RCC_config_PLLs(const RCC_PLL_cfg_TypeDef pll[4]);

/**
 * @brief Program internal clock dividers (ICs) from configuration table
 *
 * @param [in] ic Array of 20 IC configurations
 */
void RCC_config_ICs(const RCC_IC_cfg_TypeDef ic[20]);

/**
 * @brief Full board clock configuration
 *
 * Calls RCC_SystemClock_Config, RCC_config_PLLs, RCC_config_ICs,
 * then applies explicit IC1/CFGR1/CFGR2 overrides.
 */
void RCC_BoardClock_Config(void);

/**
 * @brief Get the HSI oscillator frequency
 *
 * @param [out] hz HSI frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetHSI(uint32_t* hz);

/**
 * @brief Get the SYSCLK frequency
 *
 * @param [out] hz SYSCLK frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetSYSCLK(uint32_t* hz);

/**
 * @brief Get the CPU clock frequency
 *
 * @param [out] hz CPU clock frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetCPUCLK(uint32_t* hz);

/**
 * @brief Get the AXI bus clock frequency
 *
 * @param [out] hz AXI clock frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetAXICLK(uint32_t* hz);

/**
 * @brief Get the HCLK (AHB bus) frequency
 *
 * @param [out] hz HCLK frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetHCLK(uint32_t* hz);

/**
 * @brief Get the PCLK1 (APB1 bus) frequency
 *
 * @param [out] hz PCLK1 frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetPCLK1(uint32_t* hz);

/**
 * @brief Get the PCLK2 (APB2 bus) frequency
 *
 * @param [out] hz PCLK2 frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetPCLK2(uint32_t* hz);

/**
 * @brief Get the PCLK4 (APB4 bus) frequency
 *
 * @param [out] hz PCLK4 frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetPCLK4(uint32_t* hz);

/**
 * @brief Get the PCLK5 (APB5 bus) frequency
 *
 * @param [out] hz PCLK5 frequency in Hz
 *
 * @retval RCC_OK Always succeeds
 */
RCC_Status_TypeDef RCC_GetPCLK5(uint32_t* hz);

/**
 * @brief Get the timer input clock frequency
 *
 * @param [in]  TIMX Timer peripheral instance
 * @param [out] hz   Timer clock in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unsupported timer
 */
RCC_Status_TypeDef RCC_GetTIMClock(TIM_TypeDef* TIMX, uint32_t* hz);

/**
 * @brief Get the I2C peripheral clock frequency
 *
 * @param [in]  I2CX I2C peripheral instance
 * @param [out] hz   I2C clock in Hz
 *
 * @retval RCC_OK    Success
 * @retval RCC_ERROR Unsupported I2C instance
 */
RCC_Status_TypeDef RCC_GetI2CClock(I2C_TypeDef* I2CX, uint32_t* hz);

void RCC_enable_GPIO(GPIO_TypeDef* GPIOX);
void RCC_enable_I2C(I2C_TypeDef* I2CX);
void RCC_reset_I2C(I2C_TypeDef* I2CX);
void RCC_setI2C_clock_source(I2C_TypeDef* I2CX, uint32_t source);
void RCC_enable_TIM(TIM_TypeDef* TIMX);

void RCC_enable_LTDC_memory(void);
void RCC_enable_LTDC(void);
void RCC_reset_LTDC(void);
void RCC_setLTDC_clock_source(uint32_t source);
void RCC_config_LTDC_25MHz_clock(void);

void RCC_enable_DCMIPP(void);
void RCC_config_DCMIPP_clock_IC17(void);
void RCC_reset_DCMIPP(void);
void RCC_enable_CSI(void);
void RCC_reset_CSI(void);

void RCC_enable_PWR(void);
void RCC_config_PWR(void);
void RCC_enable_XSPI1(void);
void RCC_reset_XSPI1(void);
void RCC_enable_XSPI2(void);
void RCC_reset_XSPI2(void);
void RCC_enable_XSPIM(void);
void RCC_reset_XSPIM(void);
void RCC_enable_RIFSC(void);
void RCC_setXSPI1_clock_source(uint32_t source);

#endif /* SIMPLE_RCC_H */
