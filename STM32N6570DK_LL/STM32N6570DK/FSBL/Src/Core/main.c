#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_ltdc.h"
#include "simple_rcc.h"

#define LED2_PIN 10

int main(void){
	delay_init();
	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);

	LCD_Init();

	while (1) {
		LCD_SetBackgroundColor(255, 0, 0);
	    GPIO_BSRR_set(GPIOG, LED2_PIN);
	    delay_ms(500);
		LCD_SetBackgroundColor(0, 255, 0);
	    GPIO_BSRR_reset(GPIOG, LED2_PIN);
	    delay_ms(500);
		LCD_SetBackgroundColor(0, 0, 255);
		delay_ms(500);

	}
}

__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
