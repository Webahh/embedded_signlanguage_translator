#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"



#define LED2_PIN 10

int main(void){
	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE);
	delay_init();

	while (1) {
	    GPIO_BSRR_set(GPIOG, LED2_PIN);
	    delay_ms(1000);

	    GPIO_BSRR_reset(GPIOG, LED2_PIN);
	    delay_ms(1000);
	}
}

/**
 * This function is Secure and is call-able from Non-Secure regions.
 */
__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
