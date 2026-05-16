#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>
#include "stm32n6xx.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_pin_t;

void gpio_init_output(gpio_pin_t *gpio);
void gpio_set_pin(gpio_pin_t *gpio);
void gpio_reset_pin(gpio_pin_t *gpio);
void gpio_toggle_pin(gpio_pin_t *gpio);
uint8_t gpio_read_pin(gpio_pin_t *gpio);

#endif
