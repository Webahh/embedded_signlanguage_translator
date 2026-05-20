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

#define GPIO_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected    */
#define GPIO_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected    */
#define GPIO_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected    */
#define GPIO_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected    */
#define GPIO_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected    */
#define GPIO_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected    */
#define GPIO_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected    */
#define GPIO_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected    */
#define GPIO_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected    */
#define GPIO_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected    */
#define GPIO_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_ALL               ((uint16_t)0xFFFF)  /* All pins selected */

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
