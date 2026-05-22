#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_ltdc.h"
#include "simple_rcc.h"
#include "simple_lcd_framebuffer.h"

#define LED2_PIN 10

int main(void){
	delay_init();
	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOQ, 3, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOE, 1, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);
    GPIO_Config(GPIOQ, 6, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);

    GPIO_BSRR_reset(GPIOE, 1);
    delay_ms(20);

    GPIO_BSRR_set(GPIOE, 1);
    delay_ms(20);

    GPIO_BSRR_reset(GPIOQ, 3);
    GPIO_BSRR_set(GPIOQ, 6);

	while (1) {
	    GPIO_BSRR_set(GPIOG, LED2_PIN);
	    //GPIO_BSRR_set(GPIOQ, 6);
	    delay_ms(1);

	    GPIO_BSRR_reset(GPIOG, LED2_PIN);
	    //GPIO_BSRR_reset(GPIOQ, 6);
	    delay_ms(19);
	}
}

/**
 * This function is Secure and is call-able from Non-Secure regions.
 */
__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
