#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

#include "stm32n6xx_hal.h"

#include "stm32n6570_discovery.h"

#if defined ( __ICCARM__ )
typedef void (CMSE_NS_CALL *funcptr)(void);
#else
typedef void CMSE_NS_CALL (*funcptr)(void);
#endif

typedef funcptr funcptr_NS;

void Error_Handler(void);

#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOO

#define LCD_BL_CTRL_Pin GPIO_PIN_6
#define LCD_BL_CTRL_GPIO_Port GPIOQ
#define LCD_ONOFF_Pin GPIO_PIN_3
#define LCD_ONOFF_GPIO_Port GPIOQ

#define LAYER_SIZE_Y 170
#define LAYER_SIZE_X 120
#define LAYER_BYTE_PER_PIXEL 2

#ifdef __cplusplus
}
#endif

#endif
