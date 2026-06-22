/*
 * simple_ae.c
 *
 *  Created on: 15.06.2026
 *      Author: Weber
 */

#include "simple_ae.h"
#include "simple_imx335.h"
#include "stm32n657xx.h"

#define AE_TARGET_LUMA         		   160U

#define AE_FINE_TOLERANCE              5U
#define AE_COARSE_TOLERANCE            10U

#define AE_EXPOSURE_MIN_US             3200U
#define AE_EXPOSURE_MAX_US             18000U

#define AE_EXPOSURE_COARSE_INCREMENT   500U
#define AE_EXPOSURE_COARSE_DECREMENT   300U
#define AE_EXPOSURE_FINE_INCREMENT     150U
#define AE_EXPOSURE_FINE_DECREMENT     100U

#define AE_UPDATE_EVERY_N_FRAMES       5U

#define AE_GAIN_MIN_MDB                0U
#define AE_GAIN_MAX_MDB                12000U

#define AE_GAIN_COARSE_INCREMENT       2000U
#define AE_GAIN_COARSE_DECREMENT       1500U
#define AE_GAIN_FINE_INCREMENT         500U
#define AE_GAIN_FINE_DECREMENT         300U

static uint32_t ae_exposure_us = 10000U;
static volatile uint32_t ae_pending = 0U;
static volatile uint32_t ae_frame_counter = 0U;

static volatile uint32_t ae_stat1_raw = 0U;
static volatile uint32_t ae_stat2_raw = 0U;
static volatile uint32_t ae_stat3_raw = 0U;
static volatile uint32_t ae_brightness = 0U;

static uint32_t ae_gain_mdb = 0U;

static uint32_t clamp_u32(uint32_t v, uint32_t min, uint32_t max){
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static void AE_ComputeExposureGain(uint32_t average_luma,
                                   uint32_t *new_exposure_us,
                                   uint32_t *new_gain_mdb){
    uint32_t exposure = ae_exposure_us;
    uint32_t gain     = ae_gain_mdb;

    if (average_luma == 0U) {
        if (exposure < AE_EXPOSURE_MAX_US) {
            exposure += AE_EXPOSURE_COARSE_INCREMENT;
        } else {
            gain += AE_GAIN_COARSE_INCREMENT;
        }
    }
    else if (average_luma > (AE_TARGET_LUMA + AE_COARSE_TOLERANCE)) {
        /*
         * Too bright:
         * reduce gain first, then exposure.
         */
        if (gain > AE_GAIN_COARSE_DECREMENT) {
            gain -= AE_GAIN_COARSE_DECREMENT;
        } else {
            gain = AE_GAIN_MIN_MDB;

            if (exposure > (AE_EXPOSURE_MIN_US + AE_EXPOSURE_COARSE_DECREMENT)) {
                exposure -= AE_EXPOSURE_COARSE_DECREMENT;
            } else {
                exposure = AE_EXPOSURE_MIN_US;
            }
        }
    }
    else if (average_luma < (AE_TARGET_LUMA - AE_COARSE_TOLERANCE)) {
        /*
         * Too dark:
         * increase exposure first, then gain.
         */
        if (exposure < AE_EXPOSURE_MAX_US) {
            exposure += AE_EXPOSURE_COARSE_INCREMENT;
        } else {
            gain += AE_GAIN_COARSE_INCREMENT;
        }
    }
    else if (average_luma > (AE_TARGET_LUMA + AE_FINE_TOLERANCE)) {
        if (gain > AE_GAIN_FINE_DECREMENT) {
            gain -= AE_GAIN_FINE_DECREMENT;
        } else {
            gain = AE_GAIN_MIN_MDB;

            if (exposure > (AE_EXPOSURE_MIN_US + AE_EXPOSURE_FINE_DECREMENT)) {
                exposure -= AE_EXPOSURE_FINE_DECREMENT;
            } else {
                exposure = AE_EXPOSURE_MIN_US;
            }
        }
    }
    else if (average_luma < (AE_TARGET_LUMA - AE_FINE_TOLERANCE)) {
        if (exposure < AE_EXPOSURE_MAX_US) {
            exposure += AE_EXPOSURE_FINE_INCREMENT;
        } else {
            gain += AE_GAIN_FINE_INCREMENT;
        }
    }

    exposure = clamp_u32(exposure, AE_EXPOSURE_MIN_US, AE_EXPOSURE_MAX_US);
    gain     = clamp_u32(gain, AE_GAIN_MIN_MDB, AE_GAIN_MAX_MDB);

    *new_exposure_us = exposure;
    *new_gain_mdb    = gain;
}

static uint32_t AE_ExtractStatValue(uint32_t raw)
{
    /*
     * Erstmal bewusst simpel.
     * Wenn die Werte nicht sinnvoll reagieren, müssen wir hier das richtige Feld maskieren.
     */
    return raw & 0xFFU;
}

void AE_Init(uint32_t start_exposure_us){
    ae_exposure_us = clamp_u32(start_exposure_us,
                               AE_EXPOSURE_MIN_US,
                               AE_EXPOSURE_MAX_US);
    ae_gain_mdb = 0U;

    ae_pending = 0U;
    ae_frame_counter = 0U;

    ae_stat1_raw = 0U;
    ae_stat2_raw = 0U;
    ae_stat3_raw = 0U;
    ae_brightness = 0U;
}

void AE_OnFrameStats(void){
    ae_frame_counter++;

    if (ae_frame_counter < AE_UPDATE_EVERY_N_FRAMES) {
        return;
    }

    ae_frame_counter = 0U;

    ae_stat1_raw = DCMIPP->P1ST1SR;
    ae_stat2_raw = DCMIPP->P1ST2SR;
    ae_stat3_raw = DCMIPP->P1ST3SR;

    uint32_t s1 = AE_ExtractStatValue(ae_stat1_raw);
    uint32_t s2 = AE_ExtractStatValue(ae_stat2_raw);
    uint32_t s3 = AE_ExtractStatValue(ae_stat3_raw);

    uint32_t avg_luma = (s1 + s2 + s3) / 3U;
    uint32_t max_luma = s1;

    if (s2 > max_luma) max_luma = s2;
    if (s3 > max_luma) max_luma = s3;

    ae_brightness = ((avg_luma * 3U) + max_luma) / 4U;

    ae_pending = 1U;
}

void AE_Process(CAM_Handle *h){
    if (!h) {
        return;
    }

    if (!ae_pending) {
        return;
    }

    ae_pending = 0U;

    uint32_t new_exposure_us;
    uint32_t new_gain_mdb;

    AE_ComputeExposureGain(ae_brightness,
                           &new_exposure_us,
                           &new_gain_mdb);

    if (new_exposure_us != ae_exposure_us) {
        ae_exposure_us = new_exposure_us;
        IMX335_SetExposureUs(&h->imx335, ae_exposure_us);
    }

    if (new_gain_mdb != ae_gain_mdb) {
        ae_gain_mdb = new_gain_mdb;
        IMX335_SetGainMdB(&h->imx335, ae_gain_mdb);
    }
}

uint32_t AE_GetExposureUs(void){
    return ae_exposure_us;
}

uint32_t AE_GetBrightness(void){
    return ae_brightness;
}

uint32_t AE_GetStat1Raw(void){
    return ae_stat1_raw;
}

uint32_t AE_GetStat2Raw(void){
    return ae_stat2_raw;
}

uint32_t AE_GetStat3Raw(void){
    return ae_stat3_raw;
}

uint32_t AE_GetGainMdB(void){
    return ae_gain_mdb;
}
