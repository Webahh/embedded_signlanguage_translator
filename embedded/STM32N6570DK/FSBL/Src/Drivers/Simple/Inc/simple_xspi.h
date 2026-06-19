#ifndef SIMPLE_XSPI_H
#define SIMPLE_XSPI_H

#include <stdint.h>

typedef struct {
	uint8_t memory_type;
	uint8_t devsize;
	uint8_t chipselect_high_time;
	uint8_t prescaler;
	uint8_t chipselect_boundary;
	uint16_t maxtran_value;
	uint16_t refresh_cycles;
} XSPI_cfg_TypeDef;

typedef struct {
	uint8_t instruction_mode;
	uint8_t instruction_dtr;
	uint8_t instruction_size;
	uint8_t address_mode;
	uint8_t address_dtr;
	uint8_t address_size;
	uint8_t data_mode;
	uint8_t data_dtr;
	uint8_t data_qse;
} XSPI_CCR_cfg_TypeDef;

void PSRAM_Init(XSPI_cfg_TypeDef init_cfg);
void NOR_Init(XSPI_cfg_TypeDef init_cfg);

#endif
