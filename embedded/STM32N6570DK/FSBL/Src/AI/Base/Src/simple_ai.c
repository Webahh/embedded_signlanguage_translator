/*
 * simple_ai.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */

#include <string.h>

#include "simple_ai.h"
#include "simple_rcc.h"
#include "npu_cache.h"
#include "stm32n6xx_hal.h"
#include "ll_aton_runtime.h"

#include "palm_detection.h"
#include "hand_landmark.h"
#include "fingeralphabet.h"

static CACHEAXI_HandleTypeDef ai_cacheaxi_handle;
static bool ai_initialized = false;

static void AI_EnableNpuRam(void)
{
	npu_cache_enable();

    RCC->AHB2ENR |= RCC_AHB2ENR_RAMCFGEN;
    (void)RCC->AHB2ENR;

    RAMCFG_SRAM3_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM3_AXI->CR;

    RAMCFG_SRAM4_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM4_AXI->CR;

    RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM5_AXI->CR;

    RAMCFG_SRAM6_AXI->CR &= ~RAMCFG_CR_SRAMSD;
    (void)RAMCFG_SRAM6_AXI->CR;

    RCC->MEMENR |= 0x17FFU;
    (void)RCC->MEMENR;

    RCC->AHB5ENR |= 0xC0183022U;
    (void)RCC->AHB5ENR;
}

bool AI_RuntimeRunNetwork(NN_Instance_TypeDef *network)
{
    LL_ATON_RT_RetValues_t status;

    if (network == NULL) {
        return false;
    }

    do {
        status = LL_ATON_RT_RunEpochBlock(network);

        if (status == LL_ATON_RT_WFE) {
            LL_ATON_OSAL_WFE();
        }

    } while ((status == LL_ATON_RT_WFE) ||
             (status == LL_ATON_RT_NO_WFE));

    return status == LL_ATON_RT_DONE;
}

bool AI_IsInitialized(void)
{
    return ai_initialized;
}

AI_Status_TypeDef AI_Init(void)
{
    AI_Status_TypeDef status;

    if (ai_initialized) {
        return AI_STATUS_OK;
    }

    AI_EnableNpuRam();

    ai_cacheaxi_handle.Instance = CACHEAXI;

    if (HAL_CACHEAXI_Init(&ai_cacheaxi_handle) != HAL_OK) {
        return AI_STATUS_CACHEAXI_ERROR;
    }

    LL_ATON_RT_RuntimeInit();

    status = PALM_Init();
    if (status != AI_STATUS_OK) {
        return status;
    }

    status = LANDMARK_Init();
    if (status != AI_STATUS_OK) {
        return status;
    }

    status = FINGERALPHABET_Init();
    if (status != AI_STATUS_OK) {
        return status;
    }

    ai_initialized = true;
    return AI_STATUS_OK;
}





