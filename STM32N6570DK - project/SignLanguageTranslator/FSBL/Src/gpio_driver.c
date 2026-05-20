/**
  ******************************************************************************
  * @file    gpio_driver.c
  * @author  Oliver Groß
  * @brief   GPIO low-level driver implementation.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gpio_driver.h"

/**
  * @brief  Initialize a GPIO pin as push-pull output (high speed, no pull).
  *         Enables the GPIO port clock and configures MODER, OTYPER, OSPEEDR,
  *         and PUPDR registers.
  * @param  gpio: pointer to @ref gpio_pin_t descriptor
  * @retval None
  */
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

/**
  * @brief  Set the GPIO pin to high level.
  * @param  gpio: pointer to @ref gpio_pin_t descriptor
  * @retval None
  */
void gpio_set_pin(gpio_pin_t *gpio)
{
    gpio->port->BSRR = gpio->pin;
}

/**
  * @brief  Set the GPIO pin to low level.
  * @param  gpio: pointer to @ref gpio_pin_t descriptor
  * @retval None
  */
void gpio_reset_pin(gpio_pin_t *gpio)
{
    gpio->port->BRR = gpio->pin;
}

/**
  * @brief  Toggle the GPIO pin output level.
  * @param  gpio: pointer to @ref gpio_pin_t descriptor
  * @retval None
  */
void gpio_toggle_pin(gpio_pin_t *gpio)
{
    gpio->port->ODR ^= gpio->pin;
}

/**
  * @brief  Read the current logic level of the GPIO pin.
  * @param  gpio: pointer to @ref gpio_pin_t descriptor
  * @retval 1 if pin is high, 0 if low
  */
uint8_t gpio_read_pin(gpio_pin_t *gpio)
{
    return (gpio->port->IDR & gpio->pin) != 0;
}
