/**
  ******************************************************************************
  * @file    gpio_driver.h
  * @author  Oliver Groß
  * @brief   GPIO low-level driver header
  ******************************************************************************
  */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>
#include "stm32n6xx.h"

/**
  * @brief  GPIO pin descriptor structure
  */
typedef struct {
    GPIO_TypeDef *port;		/*!< Pointer to GPIO port register base */
    uint16_t pin;      		/*!< Pin mask (e.g. GPIO_PIN_5)         */
} gpio_pin_t;


void gpio_init_output(gpio_pin_t *gpio);
void gpio_set_pin(gpio_pin_t *gpio);
void gpio_reset_pin(gpio_pin_t *gpio);
void gpio_toggle_pin(gpio_pin_t *gpio);
uint8_t gpio_read_pin(gpio_pin_t *gpio);

#endif /* GPIO_DRIVER_H */
