#include "gpio_driver.h"

void gpio_init_output(gpio_pin_t *gpio)
{
    uint32_t pin = gpio->pin;
    GPIO_TypeDef *port = gpio->port;
    uint32_t pos = __builtin_ctz(pin);

    uint32_t port_idx = ((uint32_t)port - GPIOA_BASE) / 0x400U;
    RCC->AHB4ENR |= (1UL << port_idx);
    (void)RCC->AHB4ENR;

    port->MODER = (port->MODER & ~(3UL << (pos * 2))) | (1UL << (pos * 2));

    port->OTYPER &= ~(1UL << pos);

    port->OSPEEDR = (port->OSPEEDR & ~(3UL << (pos * 2))) | (3UL << (pos * 2));

    port->PUPDR &= ~(3UL << (pos * 2));

    gpio_reset_pin(gpio);
}

void gpio_set_pin(gpio_pin_t *gpio)
{
    gpio->port->BSRR = gpio->pin;
}

void gpio_reset_pin(gpio_pin_t *gpio)
{
    gpio->port->BRR = gpio->pin;
}

void gpio_toggle_pin(gpio_pin_t *gpio)
{
    gpio->port->ODR ^= gpio->pin;
}

uint8_t gpio_read_pin(gpio_pin_t *gpio)
{
    return (gpio->port->IDR & gpio->pin) != 0;
}
