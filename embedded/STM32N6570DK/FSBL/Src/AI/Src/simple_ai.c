/*
 * simple_ai.c
 *
 *  Created on: 23.06.2026
 *      Author: Weber
 */


#include "simple_ai.h"

#include "stm32n6xx_hal.h"
#include "stm32n657xx.h"
#include "ll_aton_rt_user_api.h"
#include "ll_aton_runtime.h"
#include "simple_rcc.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(fingeralphabet_model);

static CACHEAXI_HandleTypeDef ai_cacheaxi_handle;
static bool ai_initialized = false;

AI_Status_TypeDef AI_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_RAMCFGEN;
	(void)RCC->AHB2ENR;

	RAMCFG_SRAM5_AXI->CR &= ~RAMCFG_CR_SRAMSD;
	(void)RAMCFG_SRAM5_AXI->CR;

	RCC->MEMENR |= 0x17ff;
	(void)RCC->MEMENR;
	RCC->AHB5ENR |= 0xc0183022;		//CAPSULATE SOON IN RCC!!!!
	(void)RCC->AHB5ENR;

    ai_cacheaxi_handle.Instance = CACHEAXI;

    if (HAL_CACHEAXI_Init(&ai_cacheaxi_handle) != HAL_OK) {
        return AI_STATUS_CACHEAXI_ERROR;
    }

    LL_ATON_RT_RuntimeInit();
    LL_ATON_RT_Init_Network(&NN_Instance_fingeralphabet_model);

    ai_initialized = true;

    return AI_STATUS_OK;
}

uint8_t *AI_GetInputBuffer(void)
{
    if (!ai_initialized) {
        return NULL;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return NULL;
    }

    return LL_Buffer_addr_start(info);
}

uint8_t *AI_GetOutputBuffer(void)
{
    if (!ai_initialized) {
        return NULL;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return NULL;
    }

    return LL_Buffer_addr_start(info);
}

uint32_t AI_GetInputSize(void)
{
    if (!ai_initialized) {
        return 0U;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Input_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return 0U;
    }

    return LL_Buffer_len(info);
}

uint32_t AI_GetOutputSize(void)
{
    if (!ai_initialized) {
        return 0U;
    }

    const LL_Buffer_InfoTypeDef *info =
        LL_ATON_Output_Buffers_Info(&NN_Instance_fingeralphabet_model);

    if ((info == NULL) || (info->name == NULL)) {
        return 0U;
    }

    return LL_Buffer_len(info);
}
