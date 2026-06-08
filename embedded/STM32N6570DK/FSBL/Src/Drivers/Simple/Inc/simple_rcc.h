/*
 * simple_rcc.h
 *
 *  Created on: May 22, 2026
 *      Author: Weber
 */

#ifndef SIMPLE_RCC_H
#define SIMPLE_RCC_H

#include "stm32n657xx.h"

#define RCC_HSI_VALUE_HZ  				64000000UL

#define LTDC_CLOCK_SOURCE_PCLK5      	0U
#define LTDC_CLOCK_SOURCE_PER_CK     	1U
#define LTDC_CLOCK_SOURCE_IC16_CK    	2U
#define LTDC_CLOCK_SOURCE_HSI_DIV_CK 	3U

typedef struct {
    uint32_t CFGR1;     /* PLLxCFGR1 value */
    uint32_t CFGR2;     /* PLLxCFGR2 value */
    uint32_t CFGR3;     /* PLLxCFGR3 value */
} RCC_PLL_ConfigTypeDef;

void RCC_SystemClock_Config(void);
void RCC_config_PLLs(const RCC_PLL_ConfigTypeDef pll[4]);

uint32_t RCC_GetHSI(void);
uint32_t RCC_GetSYSCLK(void);
uint32_t RCC_GetCPUCLK(void);
uint32_t RCC_GetHCLK(void);
uint32_t RCC_GetPCLK1(void);
uint32_t RCC_GetPCLK2(void);
uint32_t RCC_GetPCLK4(void);
uint32_t RCC_GetPCLK5(void);
uint32_t RCC_GetTIMClock(TIM_TypeDef *TIMX);
uint32_t RCC_GetI2CClock(I2C_TypeDef *I2CX);

void RCC_enable_GPIO(GPIO_TypeDef* GPIOX);
void RCC_enable_I2C(I2C_TypeDef* I2CX);
void RCC_reset_I2C(I2C_TypeDef* I2CX);
void RCC_setI2C_clock_source(I2C_TypeDef* I2CX, uint32_t source);
void RCC_enable_TIM(TIM_TypeDef *TIMX);

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
void RCC_config_CSI_clock_IC18(void);

void RCC_enable_PWR(void);
void RCC_enable_XSPI1(void);
void RCC_reset_XSPI1(void);
void RCC_enable_XSPI2(void);
void RCC_reset_XSPI2(void);
void RCC_enable_XSPIM(void);
void RCC_reset_XSPIM(void);
void RCC_enable_VDDIO2(void);
void RCC_config_VDDIO2_1V8(void);

#endif // SIMPLE_RCC_H
