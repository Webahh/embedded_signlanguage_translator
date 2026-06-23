/**
 * @file    simple_gpio.h
 * @author  Weber
 * @date    May 21, 2026
 * @brief   GPIO control driver header
 *
 * Usage
 * -----
 * 1. RCC_enable_GPIO()   – enable GPIO clock
 * 2. GPIO_Config()        – set pin mode / speed / AF
 * 3. GPIO_BSRR_*()        – set / reset / toggle pin
 * 4. GPIO_get()           – read pin state
 */

#ifndef SIMPLE_GPIO_H
#define SIMPLE_GPIO_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"

#define GPIO_MODE_INPUT     0
#define GPIO_MODE_OUTPUT    1
#define GPIO_MODE_AF        2
#define GPIO_MODE_ANALOG    3

#define GPIO_OTYPE_PP       0
#define GPIO_OTYPE_OD       1

#define GPIO_PUPD_NONE      0
#define GPIO_PUPD_UP        1
#define GPIO_PUPD_DOWN      2

#define GPIO_SPEED_LOW      0U
#define GPIO_SPEED_MEDIUM   1U
#define GPIO_SPEED_HIGH     2U
#define GPIO_SPEED_VERY_HIGH 3U

#define GPIO_AF_NONE        -1
#define GPIO_AF_I2C         4
#define GPIO_AF_LTDC        14
#define GPIO_AF_XSPI        9
#define GPIO_AF_USART       7

/** GPIO operation status codes */
typedef enum {
    GPIO_OK    = 0,
    GPIO_ERROR = 1,
} GPIO_Status_TypeDef;

/** GPIO pin configuration structure */
typedef struct {
    uint32_t mode;
    uint32_t otyp;
    uint32_t pupdr;
    uint32_t af;
    uint32_t speed;
} GPIO_cfg_TypeDef;

// ---- API ----

/**
 * @brief  Configure a GPIO pin (mode, output type, pull, AF, speed)
 * @param [in] GPIOX | GPIO port instance (e.g. GPIOA)
 * @param [in] pinNr | Pin number (0-15)
 * @param [in] cfg   | Pin configuration parameters
 */
void GPIO_Config(GPIO_TypeDef *GPIOX, uint32_t pinNr, GPIO_cfg_TypeDef cfg);

/**
 * @brief  Read the input state of a GPIO pin
 * @param [in]			GPIOX    | GPIO port instance
 * @param [in]		  	pinNr    | Pin number (0-15)
 * @param [out]		  	pinState | 1 = high, 0 = low
 * @retval GPIO_OK	  	State read successfully
 * @retval GPIO_ERROR 	Invalid pin number
 */
GPIO_Status_TypeDef GPIO_get(GPIO_TypeDef *GPIOX, uint32_t pinNr, uint32_t *pinState);

/**
 * @brief  Set a GPIO pin high
 * @param [in] GPIOX | GPIO port instance
 * @param [in] pinNr | Pin number (0-15)
 */
void GPIO_BSRR_set(GPIO_TypeDef *GPIOX, uint32_t pinNr);

/**
 * @brief  Set a GPIO pin low
 * @param [in] GPIOX | GPIO port instance
 * @param [in] pinNr | Pin number (0-15)
 */
void GPIO_BSRR_reset(GPIO_TypeDef *GPIOX, uint32_t pinNr);

/**
 * @brief  Toggle a GPIO pin
 * @param [in] GPIOX | GPIO port instance
 * @param [in] pinNr | Pin number (0-15)
 */
void GPIO_BSRR_toggle(GPIO_TypeDef *GPIOX, uint32_t pinNr);

#endif /* SIMPLE_GPIO_H */
