#include <stdint.h>
#include <stddef.h>

#include "simple_touch.h"
#include "simple_i2c.h"
#include "simple_timer.h"
#include "simple_rcc.h"
#include "simple_gpio.h"
#include "simple_scheduler.h"
#include "config.h"
#include "stm32n657xx.h"

#define _CONFIG_START     0x8047
#define _CONFIG_END       0x80FF  /* exclusive: sum covers 0x8047..0x80FE */
#define _CONFIG_LEN       (_CONFIG_END - _CONFIG_START)

#define _X_RES_L          (_CONFIG_START + 1)
#define _X_RES_H          (_CONFIG_START + 2)
#define _Y_RES_L          (_CONFIG_START + 3)
#define _Y_RES_H          (_CONFIG_START + 4)
#define _MSW1_REG         (_CONFIG_START + 6)

#define _X_RES_VAL        800	/* Display Height */
#define _Y_RES_VAL        480	/* Display Width */
#define _MSW1_VAL         0x0C  /* bits 2|3 = Y/X reverse (Max-Min) */

#define _CONFIG_VERSION   0x82

static uint8_t _config_buf[_CONFIG_LEN];

static volatile uint8_t _touch_pending;
static volatile TOUCH_Data_TypeDef _saved_data;

/**
 * @brief calculates checksum of touch controller configuration
 *
 * @param [in]  buf			| buffer to perform checksum on
 * @param [in]  buf_len		| buffer length
 * @param [out] checksum	| resulting checksum
 */
static void _calc_checksum(const uint8_t *buf, uint16_t buf_len, uint8_t *checksum)
{
	uint8_t sum = 0;
	for (uint16_t i = 0; i < buf_len; i++)
		sum += buf[i];
	*checksum = ((~sum) + 1) & 0xFF;
}

/**
 * @brief Read from touch controller
 *
 * @param [in] h |Touch handle
 */
static TOUCH_Status_TypeDef _read_config(TOUCH_Handle_TypeDef *h)
{
	if (I2C_Mem_read(h->i2c, h->addr, _CONFIG_START, _config_buf, _CONFIG_LEN) != I2C_OK) {
		return TOUCH_ERROR;
	}
	return TOUCH_OK;
}

/**
 * @brief Writes to touch controller
 *
 * @param [in] h |Touch handle
 */
static TOUCH_Status_TypeDef _write_config(TOUCH_Handle_TypeDef *h)
{
	if (I2C_Mem_write(h->i2c, h->addr, _CONFIG_START, _config_buf, _CONFIG_LEN) != I2C_OK) {
		return TOUCH_ERROR;
	}
	return TOUCH_OK;
}

void TOUCH_ConfigIO(void)
{
	RCC_enable_GPIO(GPIOD);
	RCC_enable_GPIO(GPIOE);
	RCC_enable_GPIO(GPIOQ);

	GPIO_Config(GPIOD, 14, GPIO_I2C_cfg);
	GPIO_Config(GPIOD, 4, GPIO_I2C_cfg);

	GPIO_Config(GPIOE, 1, GPIO_TS_RST_cfg);
	GPIO_BSRR_reset(GPIOE, 1);
	TIMER_Delay_ms(10);
	GPIO_BSRR_set(GPIOE, 1);
	TIMER_Delay_ms(100);

	GPIO_Config(GPIOQ, 4, GPIO_TS_INT_cfg);

	I2C_Config(I2C2, 0, I2C_SENSOR_BUS_TIMING);

	_touch_pending = 0;
	_saved_data.x = 0;
	_saved_data.y = 0;
	_saved_data.pressed = 0;

	EXTI->RPR1 = EXTI_RPR1_RPIF4;

	EXTI->EXTICR[1] = (EXTI->EXTICR[1] & ~EXTI_EXTICR2_EXTI4) | 0x0BU;
	EXTI->RTSR1 |= EXTI_RTSR1_RT4;
	EXTI->IMR1 |= EXTI_IMR1_IM4;

	NVIC_SetPriority(EXTI4_IRQn, 0x80);
	NVIC_ClearPendingIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
}

TOUCH_Status_TypeDef TOUCH_Probe(TOUCH_Handle_TypeDef *h, I2C_TypeDef *i2c)
{
	if (h == NULL || i2c == NULL)
		return TOUCH_ERROR;

	h->i2c = i2c;
	h->addr = GT911_I2C_ADDR;
	h->initialized = 0;

	uint8_t id[4];
	if (I2C_Mem_read(i2c, h->addr, GT911_REG_CHIP_ID_H, id, 4) != I2C_OK)
		return TOUCH_ERROR;

	if (id[0] == '9' && id[1] == '1' && id[2] == '1' && id[3] == '\0') {
		h->initialized = 1;
		return TOUCH_OK;
	}

	return TOUCH_ERROR;
}

TOUCH_Status_TypeDef TOUCH_Init(TOUCH_Handle_TypeDef *h)
{
	if (h == NULL || !h->initialized)
		return TOUCH_ERROR;

	uint8_t tmp;

	/* Read factory config */
	if (_read_config(h) != TOUCH_OK)
		return TOUCH_ERROR;

	/* Override config version to force update */
	_config_buf[0] = _CONFIG_VERSION;

	/* Override resolution: X=800, Y=480 */
	_config_buf[_X_RES_L - _CONFIG_START] = _X_RES_VAL & 0xFF;
	_config_buf[_X_RES_H - _CONFIG_START] = (_X_RES_VAL >> 8) & 0x0F;
	_config_buf[_Y_RES_L - _CONFIG_START] = _Y_RES_VAL & 0xFF;
	_config_buf[_Y_RES_H - _CONFIG_START] = (_Y_RES_VAL >> 8) & 0x0F;

	/* Force MSW1: rising edge, no Y-reverse, no swap */
	_config_buf[_MSW1_REG - _CONFIG_START] = _MSW1_VAL;

	/* Write patched config */
	if (_write_config(h) != TOUCH_OK)
		return TOUCH_ERROR;

	/* Calculate and write checksum from patched table */
	_calc_checksum(_config_buf, _CONFIG_LEN, &tmp);
	if (I2C_Mem_write(h->i2c, h->addr, GT911_REG_CONFIG_CHKSUM, &tmp, 1) != I2C_OK) {
		return TOUCH_ERROR;
	}

	/* Mark config fresh */
	tmp = 1;
	if (I2C_Mem_write(h->i2c, h->addr, GT911_REG_CONFIG_FRESH, &tmp, 1) != I2C_OK) {
		return TOUCH_ERROR;
	}

	/* Factory calibration (non-fatal - matches ST BSP behaviour) */
	I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
	tmp &= ~0x70;
	tmp |= (0x04 << 4);
	I2C_Mem_write(h->i2c, h->addr, 0x00, &tmp, 1);
	TIMER_Delay_ms(300);

	I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
	if (((tmp >> 4) & 0x07) == 0x04) {
		tmp = 0x04;
		I2C_Mem_write(h->i2c, h->addr, GT911_REG_TD_STATUS, &tmp, 1);
		TIMER_Delay_ms(300);

		for (uint16_t i = 0; i < 100; i++) {
			I2C_Mem_read(h->i2c, h->addr, 0x00, &tmp, 1);
			if (((tmp >> 4) & 0x07) == 0x00) {
				break;
			}
			TIMER_Delay_ms(200);
		}
	}

	tmp = _MSW1_VAL;
	I2C_Mem_write(h->i2c, h->addr, GT911_REG_MSW1, &tmp, 1);

	tmp = 0;
	I2C_Mem_write(h->i2c, h->addr, GT911_REG_TD_STATUS, &tmp, 1);

	return TOUCH_OK;
}

void TOUCH_GetPending(uint8_t *p)
{
	if (p) {
		*p = _touch_pending;
		_touch_pending = 0;
	}
}

TOUCH_Status_TypeDef TOUCH_GetState(TOUCH_Handle_TypeDef *h, TOUCH_Data_TypeDef *data)
{
	if (data == NULL)
		return TOUCH_ERROR;

	data->x = _saved_data.x;
	data->y = _saved_data.y;
	data->pressed = _saved_data.pressed;

	_saved_data.pressed = 0;

	return TOUCH_OK;
}

// -------------------------------------------------------------------------
// Interrupt
// -------------------------------------------------------------------------

void EXTI4_IRQHandler(void)
{
	SCHEDULER_ISR_enter();
	if (EXTI->RPR1 & EXTI_RPR1_RPIF4) {
		EXTI->RPR1 = EXTI_RPR1_RPIF4;

		uint8_t status;
		I2C_Mem_read(I2C2, GT911_I2C_ADDR, GT911_REG_TD_STATUS, &status, 1);

		status &= 0x0F;
		if (status > 0 && status <= 5) {
			uint8_t buf[4];
			I2C_Mem_read(I2C2, GT911_I2C_ADDR, GT911_REG_TOUCH1_XL, buf, 4);
			_saved_data.x = (uint16_t) buf[0] | ((uint16_t) (buf[1] & 0x0F) << 8);
			_saved_data.y = (uint16_t) buf[2] | ((uint16_t) (buf[3] & 0x0F) << 8);
			_saved_data.pressed = 1;
		} else {
			_saved_data.pressed = 0;
		}

		uint8_t zero = 0;
		I2C_Mem_write(I2C2, GT911_I2C_ADDR, GT911_REG_TD_STATUS, &zero, 1);

		_touch_pending = 1;
	}
	SCHEDULER_ISR_exit();
}
