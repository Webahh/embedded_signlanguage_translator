/*
 * simple_ae.h
 *
 *  Created on: 15.06.2026
 *      Author: Weber
 */

#ifndef SIMPLE_AE_H
#define SIMPLE_AE_H

#include "simple_camera.h"
#include <stdint.h>

void AE_process(CAM_Handle_TypeDef* h);
void AE_Init(uint32_t start_exposure_us);
void AE_OnFrameStats(void);
void AE_Process(CAM_Handle_TypeDef *h);

uint32_t AE_GetExposureUs(void);
uint32_t AE_GetBrightness(void);
uint32_t AE_GetStat1Raw(void);
uint32_t AE_GetStat2Raw(void);
uint32_t AE_GetStat3Raw(void);
uint32_t AE_GetGainMdB(void);


#endif /* SIMPLE_AE_H */
