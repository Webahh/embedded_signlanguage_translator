#ifndef SIMPLE_XSPI_H
#define SIMPLE_XSPI_H

#include <stdint.h>

void XSPI1_GPIO_Init(void);
void XSPI1_Init(void);
void XSPI1_WriteReg(uint8_t reg_addr, uint8_t value);
void XSPI1_EnableMemoryMappedMode(void);
void XSPI2_GPIO_Init(void);
void XSPI2_Init(void);
void XSPI2_WriteReg(uint8_t reg_addr, uint8_t value);
void XSPI2_EnableMemoryMappedMode(void);
void PSRAM_Init(void);
void NOR_Init(void);

#endif
