/**
 * @file    simple_ae.h
 * @author  Weber
 * @date    15.06.2026
 * @brief   Auto-exposure (AE) control source
 */

#include <stddef.h>

#include "simple_ae.h"
#include "simple_imx335.h"
#include "stm32n657xx.h"
#include "config.h"
#include "simple_debug_log.h"

// ---- Private defines ----

#define _TARGET_LUMA               56U
#define _FINE_TOLERANCE            5U
#define _COARSE_TOLERANCE          10U
#define _EXPOSURE_MIN_US           15U
#define _EXPOSURE_MAX_US           33266U
#define _STATS_WINDOW_PIXELS       1259712U
#define _EXPOSURE_COARSE_INCREMENT 500U
#define _EXPOSURE_COARSE_DECREMENT 300U
#define _EXPOSURE_FINE_INCREMENT   150U
#define _EXPOSURE_FINE_DECREMENT   100U
#define _UPDATE_EVERY_N_FRAMES     5U
#define _GAIN_MIN_MDB              0U
#define _GAIN_MAX_MDB              72000U
#define _GAIN_COARSE_INCREMENT     2000U
#define _GAIN_COARSE_DECREMENT     1500U
#define _GAIN_FINE_INCREMENT       500U
#define _GAIN_FINE_DECREMENT       300U

// ---- Statistics component IDs ----
#define _AE_STAT_RED    0U
#define _AE_STAT_GREEN  1U
#define _AE_STAT_BLUE   2U

// ---- Private state ----

static uint32_t       _exposure_us    = 10000U;
static volatile uint32_t _pending     = 0U;
static volatile uint32_t _frame_counter = 0U;
static volatile uint32_t _stat1_raw   = 0U;
static volatile uint32_t _stat2_raw   = 0U;
static volatile uint32_t _stat3_raw   = 0U;
static volatile uint32_t _brightness  = 0U;
static uint32_t       _gain_mdb       = 0U;

// ---- Private helpers ----

/**
 * @brief  Clamp a value to [min, max]
 * @param  [in]  v      | Value to clamp
 * @param  [in]  min    | Minimum bound
 * @param  [in]  max    | Maximum bound
 * @param  [out] clamped | Clamped output value
 */
static void AE_clamp_u32(uint32_t v, uint32_t min, uint32_t max, uint32_t *clamp){
    if (v < min) {
    	*clamp = min;
    } else if (v > max) {
    	*clamp = max;
    } else {
    	*clamp = v;
    }
}

/**
 * @brief  Compute new exposure and gain based on average luma
 * @param  [in]  average_luma    | Measured average luma
 * @param  [out] new_exposure_us | Computed exposure in microseconds
 * @param  [out] new_gain_mdb    | Computed gain in millidB
 */
static void AE_ComputeExposureGain(uint32_t average_luma,
                                   uint32_t *new_exposure_us,
                                   uint32_t *new_gain_mdb){
    uint32_t exposure = _exposure_us;
    uint32_t gain     = _gain_mdb;

    if (average_luma == 0U) {
        if (exposure < _EXPOSURE_MAX_US) {
            exposure += _EXPOSURE_COARSE_INCREMENT;
        } else {
            gain += _GAIN_COARSE_INCREMENT;
        }
    }
    else if (average_luma > (_TARGET_LUMA + _COARSE_TOLERANCE)) {
        // Too bright: reduce gain first, then exposure.
        if (gain > _GAIN_COARSE_DECREMENT) {
            gain -= _GAIN_COARSE_DECREMENT;
        } else {
            gain = _GAIN_MIN_MDB;

            if (exposure > (_EXPOSURE_MIN_US + _EXPOSURE_COARSE_DECREMENT)) {
                exposure -= _EXPOSURE_COARSE_DECREMENT;
            } else {
                exposure = _EXPOSURE_MIN_US;
            }
        }
    }
    else if (average_luma < (_TARGET_LUMA - _COARSE_TOLERANCE)) {
        // Too dark: increase exposure first, then gain.
        if (exposure < _EXPOSURE_MAX_US) {
            exposure += _EXPOSURE_COARSE_INCREMENT;
        } else {
            gain += _GAIN_COARSE_INCREMENT;
        }
    }
    else if (average_luma > (_TARGET_LUMA + _FINE_TOLERANCE)) {
        if (gain > _GAIN_FINE_DECREMENT) {
            gain -= _GAIN_FINE_DECREMENT;
        } else {
            gain = _GAIN_MIN_MDB;

            if (exposure > (_EXPOSURE_MIN_US + _EXPOSURE_FINE_DECREMENT)) {
                exposure -= _EXPOSURE_FINE_DECREMENT;
            } else {
                exposure = _EXPOSURE_MIN_US;
            }
        }
    }
    else if (average_luma < (_TARGET_LUMA - _FINE_TOLERANCE)) {
        if (exposure < _EXPOSURE_MAX_US) {
            exposure += _EXPOSURE_FINE_INCREMENT;
        } else {
            gain += _GAIN_FINE_INCREMENT;
        }
    }

    AE_clamp_u32(exposure, _EXPOSURE_MIN_US, _EXPOSURE_MAX_US, &exposure);
    AE_clamp_u32(gain, _GAIN_MIN_MDB, _GAIN_MAX_MDB, &gain);

    *new_exposure_us = exposure;
    *new_gain_mdb    = gain;
}

/**
 * @brief  Extract per-component average from raw DCMIPP statistics register
 *         The hardware ACCU field contains (sum_of_8bit_pixel_values / 256).
 *         For raw Bayer data, each colour component occupies only a fraction
 *         of the total pixels (R=1/4, G=1/2, B=1/4).
 * @param  [in] raw | Raw DCMIPP statistics register value
 * @param  [in] comp | Colour component (_AE_STAT_RED/GREEN/BLUE)
 * @retval Average 8-bit value for that component
 */
static uint32_t AE_ExtractStatValue(uint32_t raw, uint32_t comp){
    uint32_t comp_pixels = _STATS_WINDOW_PIXELS;

    if (comp == _AE_STAT_GREEN) {
        comp_pixels /= 2U;          // 2 of 4 Bayer pixels are green
    } else {
        comp_pixels /= 4U;          // 1 of 4 Bayer pixels are R or B
    }

    uint32_t accu = raw & DCMIPP_P1ST1SR_ACCU_Msk;
    return ((accu * 256U) + (comp_pixels / 2U)) / comp_pixels;
}

// ---- API ----

/**
 * @brief  Initialise AE with a starting exposure value
 * @param  [in] start_exposure_us | Initial exposure in microseconds
 */
void AE_Init(uint32_t start_exposure_us){
    AE_clamp_u32(start_exposure_us, _EXPOSURE_MIN_US, _EXPOSURE_MAX_US, &_exposure_us);
    _gain_mdb = 0U;

    _pending = 0U;
    _frame_counter = 0U;

    _stat1_raw = 0U;
    _stat2_raw = 0U;
    _stat3_raw = 0U;
    _brightness = 0U;

    DEBUG_PRINTF("[AE] Init: exposure=%lu us, gain=%lu mdB, target=%lu\r\n",
                 _exposure_us, _gain_mdb, _TARGET_LUMA);
}

/**
 * @brief  Frame-statistics callback (call from DCMIPP frame ISR)
 */
void AE_OnFrameStats(void){
    _frame_counter++;

    if (_frame_counter < _UPDATE_EVERY_N_FRAMES) {
        return;
    }

    _frame_counter = 0U;

    _stat1_raw = DCMIPP->P1ST1SR;
    _stat2_raw = DCMIPP->P1ST2SR;
    _stat3_raw = DCMIPP->P1ST3SR;

    uint32_t s1 = AE_ExtractStatValue(_stat1_raw, _AE_STAT_RED);
    uint32_t s2 = AE_ExtractStatValue(_stat2_raw, _AE_STAT_GREEN);
    uint32_t s3 = AE_ExtractStatValue(_stat3_raw, _AE_STAT_BLUE);

    uint32_t avg_luma = (s1 + s2 + s3) / 3U;
    uint32_t max_luma = s1;

    if (s2 > max_luma) max_luma = s2;
    if (s3 > max_luma) max_luma = s3;

    _brightness = ((avg_luma * 3U) + max_luma) / 4U;

    _pending = 1U;
}

/**
 * @brief  Run the AE control loop (exposure / gain adjustment)
 * @param  [in] h | Camera handle
 */
void AE_Process(CAM_Handle_TypeDef *h){
    if (!h) {
        return;
    }

    if (!_pending) {
        return;
    }

    _pending = 0U;

    uint32_t s1 = AE_ExtractStatValue(_stat1_raw, _AE_STAT_RED);
    uint32_t s2 = AE_ExtractStatValue(_stat2_raw, _AE_STAT_GREEN);
    uint32_t s3 = AE_ExtractStatValue(_stat3_raw, _AE_STAT_BLUE);
    DEBUG_PRINTF("[AE] bright=%lu target=%lu | R=%lu G=%lu B=%lu | exp=%lu gain=%lu\r\n",
                 _brightness, _TARGET_LUMA,
                 s1, s2, s3,
                 _exposure_us, _gain_mdb);

    uint32_t new_exposure_us;
    uint32_t new_gain_mdb;

    AE_ComputeExposureGain(_brightness,
                           &new_exposure_us,
                           &new_gain_mdb);

    if (new_exposure_us != _exposure_us) {
        DEBUG_PRINTF("[AE] Exposure: %lu -> %lu us\r\n", _exposure_us, new_exposure_us);
        _exposure_us = new_exposure_us;
        IMX335_SetExposureUs(&h->imx335, _exposure_us);
    }

    if (new_gain_mdb != _gain_mdb) {
        DEBUG_PRINTF("[AE] Gain: %lu -> %lu mdB\r\n", _gain_mdb, new_gain_mdb);
        _gain_mdb = new_gain_mdb;
        IMX335_SetGainMdB(&h->imx335, _gain_mdb);
    }
}

/**
 * @brief  Get current exposure value
 * @param  [out] exposure | Current exposure in microseconds
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetExposureUs(uint32_t *exposure){
    if (!exposure) return AE_ERROR;
    *exposure = _exposure_us;
    return AE_OK;
}

/**
 * @brief  Get current scene brightness estimate
 * @param  [out] brightness | Brightness estimate (0-255 scale)
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetBrightness(uint32_t *brightness){
    if (!brightness) return AE_ERROR;
    *brightness = _brightness;
    return AE_OK;
}

/**
 * @brief  Get raw DCMIPP stat1 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat1Raw(uint32_t *stat){
    if (!stat) return AE_ERROR;
    *stat = _stat1_raw;
    return AE_OK;
}

/**
 * @brief  Get raw DCMIPP stat2 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat2Raw(uint32_t *stat){
    if (!stat) return AE_ERROR;
    *stat = _stat2_raw;
    return AE_OK;
}

/**
 * @brief  Get raw DCMIPP stat3 register value
 * @param  [out] stat | Raw statistics value
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetStat3Raw(uint32_t *stat){
    if (!stat) return AE_ERROR;
    *stat = _stat3_raw;
    return AE_OK;
}

/**
 * @brief  Get current analog gain
 * @param  [out] gain_mdb | Gain in millidB
 * @retval AE_OK   | Success
 * @retval AE_ERROR | NULL pointer
 */
AE_Status_TypeDef AE_GetGainMdB(uint32_t *gain_mdb){
    if (!gain_mdb) return AE_ERROR;
    *gain_mdb = _gain_mdb;
    return AE_OK;
}
