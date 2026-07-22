/**
 * @file    simple_gpio.h
 * @author  Weber
 * @date    May 21, 2026
 * @brief   GPIO control driver source
 */

#include <stdint.h>
#include <stddef.h>

#include "simple_gpio.h"
#include "simple_rcc.h"

// ---- Private helpers ----

/**
 * @brief  Configure alternate function and output speed for one pin
 * @param [in] GPIOX | GPIO port instance
 * @param [in] pinNr | Pin number (0-15)
 * @param [in] af    | Alternate function number
 * @param [in] speed | Output speed setting
 */
static void GPIO_set_af(GPIO_TypeDef *GPIOX, uint32_t pinNr, uint32_t af, uint32_t speed)
{
    GPIOX->OSPEEDR =
        (GPIOX->OSPEEDR & ~(3U << (2U * pinNr))) |
        (((uint32_t)speed & 3U) << (2U * pinNr));

    if (pinNr < 8) {
        GPIOX->AFR[0] &= ~(0xFU << (4U * pinNr));
        GPIOX->AFR[0] |=  ((uint32_t)af << (4U * pinNr));
    } else {
        GPIOX->AFR[1] &= ~(0xFU << (4U * (pinNr - 8U)));
        GPIOX->AFR[1] |=  ((uint32_t)af << (4U * (pinNr - 8U)));
    }
}

// ---- API ----

void GPIO_Config(GPIO_TypeDef *GPIOX, uint32_t pinNr, GPIO_cfg_TypeDef cfg)
{
    RCC_enable_GPIO(GPIOX);
    GPIOX->MODER  = (GPIOX->MODER  & ~(3U << (2U * pinNr))) | (cfg.mode  << (2U * pinNr));
    GPIOX->OTYPER = (GPIOX->OTYPER & ~(1U << (pinNr)))     | (cfg.otyp  << (pinNr));
    GPIOX->PUPDR  = (GPIOX->PUPDR  & ~(3U << (2U * pinNr))) | (cfg.pupdr << (2U * pinNr));

    if (cfg.mode == GPIO_MODE_AF) {
        GPIO_set_af(GPIOX, pinNr, cfg.af, cfg.speed);
    }
}

GPIO_Status_TypeDef GPIO_get(GPIO_TypeDef *GPIOX, uint32_t pinNr, uint32_t *pinState)
{
    if (pinNr > 15U || pinState == NULL)
        return GPIO_ERROR;

    *pinState = (GPIOX->IDR >> pinNr) & 1U;
    return GPIO_OK;
}

void GPIO_BSRR_toggle(GPIO_TypeDef *GPIOX, uint32_t pinNr)
{
    if (GPIOX->ODR & (1U << pinNr))
        GPIOX->BSRR = (1U << (pinNr + 16U));
    else
        GPIOX->BSRR = (1U << pinNr);
}

void GPIO_BSRR_set(GPIO_TypeDef *GPIOX, uint32_t pinNr)
{
    GPIOX->BSRR = (1U << pinNr);
}

void GPIO_BSRR_reset(GPIO_TypeDef *GPIOX, uint32_t pinNr)
{
    GPIOX->BSRR = (1U << (pinNr + 16U));
}
