/**
 * @file    simple_ae.h
 * @author  Weber
 * @date    15.06.2026
 * @brief   Auto-exposure (AE) control header
 *
 * Usage
 * -----
 * 1. AE_Init()           - initialise with starting exposure
 * 2. AE_OnFrameStats()   - call from DCMIPP frame ISR
 * 3. AE_Process()        - run control loop (task context)
 */

#ifndef SIMPLE_AE_H
#define SIMPLE_AE_H

#include <stdint.h>

#include "simple_camera.h"

// ---- Return codes ----

/** AE function return codes */
typedef enum {
    AE_OK    = 0, /**< Success              */
    AE_ERROR = 1, /**< Unspecified error    */
} AE_Status_TypeDef;

// ---- API ----

/**
 * @brief  Initialise AE with a starting exposure value
 * @param  [in] start_exposure_us | Initial exposure in microseconds
 */
void AE_Init(uint32_t start_exposure_us);

/**
 * @brief  Frame-statistics callback (call from DCMIPP frame ISR)
 */
void AE_OnFrameStats(void);

/**
 * @brief  Run the AE control loop (exposure / gain adjustment)
 * @param  [in] h | Camera handle
 */
void AE_Process(CAM_Handle_TypeDef *h);

/**
 * @brief  Get current exposure value
 * @param  [out] exposure | Current exposure in microseconds
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetExposureUs(uint32_t *exposure);

/**
 * @brief  Get current scene brightness estimate
 * @param  [out] brightness | Brightness estimate (0-255 scale)
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetBrightness(uint32_t *brightness);

/**
 * @brief  Get raw DCMIPP stat1 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat1Raw(uint32_t *stat);

/**
 * @brief  Get raw DCMIPP stat2 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat2Raw(uint32_t *stat);

/**
 * @brief  Get raw DCMIPP stat3 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat3Raw(uint32_t *stat);

/**
 * @brief  Get current analog gain
 * @param  [out] gain_mdb | Gain in millidB
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetGainMdB(uint32_t *gain_mdb);

#endif /* SIMPLE_AE_H */
