#include <stdint.h>
#include "simple_gpio.h"
#include "simple_timer.h"
#include "simple_scheduler.h"
#include "simple_ltdc.h"
#include "simple_rcc.h"

#define LED2_PIN 10

static uint8_t bg_color_state = 0;

static void vLEDTask(void) {
	GPIO_BSRR_Toggle(GPIOG, LED2_PIN);
}

static void vBackgroundTask(void) {
	switch (bg_color_state) {
		case 0: LCD_SetBackgroundColor(255, 0, 0); break;
		case 1: LCD_SetBackgroundColor(0, 255, 0); break;
		case 2: LCD_SetBackgroundColor(0, 0, 255); break;
	}
	bg_color_state = (bg_color_state + 1) % 3;
}

int main(void){
	delay_init();
	GPIO_Config(GPIOG, LED2_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_PUPD_NONE, GPIO_AF_NONE);

	LCD_Init();

	vTaskStartScheduler();

	xTaskCreate(vLEDTask, "LED", 500);
	xTaskCreate(vBackgroundTask, "BgColor", 1000);

	while (1) {
		xTaskSchedulerRun();
	}
}

__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
