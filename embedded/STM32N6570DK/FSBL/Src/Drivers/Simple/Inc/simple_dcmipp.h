/*
 * simple_dcmipp.h
 *
 * Register-level DCMIPP + CSI-2 driver for STM32N6
 * Supports two-pipe setup: PIPE1 (display) and PIPE2 (NN)
 *
 *  Created on: May 29, 2026
 *      Author: Groß
 */

#ifndef SIMPLE_DCMIPP_H
#define SIMPLE_DCMIPP_H

#include "stm32n657xx.h"
#include "simple_csi.h"

/* DCMIPP Pipe identifiers */
#define DCMIPP_PIPE0                    0U
#define DCMIPP_PIPE1                    1U
#define DCMIPP_PIPE2                    2U

/* DCMIPP IPPlug Client identifiers */
#define DCMIPP_CLIENT1                  1U
#define DCMIPP_CLIENT2                  2U
#define DCMIPP_CLIENT3                  3U
#define DCMIPP_CLIENT4                  4U
#define DCMIPP_CLIENT5                  5U

/* Pixel Packer formats (PxPPCR.FORMAT field) */
#define DCMIPP_PP_FORMAT_RGB888         0U
#define DCMIPP_PP_FORMAT_RGB565         1U
#define DCMIPP_PP_FORMAT_ARGB8888       2U
#define DCMIPP_PP_FORMAT_RGBA888        3U
#define DCMIPP_PP_FORMAT_MONO8          4U
#define DCMIPP_PP_FORMAT_YUV444         5U
#define DCMIPP_PP_FORMAT_YUV422_YUYV    6U

/* IPPlug memory page sizes */
#define DCMIPP_MEM_PAGE_64B             0U
#define DCMIPP_MEM_PAGE_128B            1U
#define DCMIPP_MEM_PAGE_256B            2U
#define DCMIPP_MEM_PAGE_512B            3U

/* IPPlug traffic burst sizes */
#define DCMIPP_TRAFFIC_8B               0U
#define DCMIPP_TRAFFIC_16B              1U
#define DCMIPP_TRAFFIC_32B              2U
#define DCMIPP_TRAFFIC_64B              3U
#define DCMIPP_TRAFFIC_128B             4U

/* DCMIPP error codes */
#define DCMIPP_OK                       0
#define DCMIPP_ERROR                   -1

/* Pipe sharing modes for PIPE2 */
#define DCMIPP_PIPE_SHARE_SAME          0  /* PIPE2 gets same data as PIPE1 */
#define DCMIPP_PIPE_SHARE_DIFFERENT     1  /* PIPE2 can use different VC/DT */

/* Interrupt masks (match CMSIS P1IER/P2IER register bits) */
#define DCMIPP_PIPE_IT_FRAMEIE          DCMIPP_P1IER_FRAMEIE
#define DCMIPP_PIPE_IT_VSYNCIE          DCMIPP_P1IER_VSYNCIE
#define DCMIPP_PIPE_IT_LINEIE           DCMIPP_P1IER_LINEIE
#define DCMIPP_PIPE_IT_OVRIE            DCMIPP_P1IER_OVRIE

/*
 * Pipe configuration structure
 */
typedef struct DCMIPP_Pipe_Conf {
    uint32_t output_width;
    uint32_t output_height;
    uint32_t output_format;      /* DCMIPP_PP_FORMAT_* */
    uint32_t output_bpp;         /* bytes per pixel (3 for RGB888, 4 for ARGB8888) */
    uint16_t crop_x;
    uint16_t crop_y;
    uint16_t crop_width;
    uint16_t crop_height;
    uint8_t  enable_crop;
    uint8_t  enable_swap;        /* red-blue swap */
    uint8_t  enable_downsize;
} DCMIPP_Pipe_Conf;

/*
 * IPPlug client configuration structure
 */
typedef struct DCMIPP_IPPlug_Conf {
    uint32_t client_id;          /* DCMIPP_CLIENT1-5 */
    uint32_t traffic;            /* DCMIPP_TRAFFIC_* */
    uint32_t outstanding;        /* 0-15 outstanding transactions */
    uint16_t dpreg_start;
    uint16_t dpreg_end;
    uint8_t  wlru_ratio;         /* 0-15: 1/16 to 16/16 of BW */
} DCMIPP_IPPlug_Conf;

/* Raw Bayer type for demosaic */
#define DCMIPP_RAWBAYER_RGGB                0U
#define DCMIPP_RAWBAYER_GRBG                2U
#define DCMIPP_RAWBAYER_GBRG                4U
#define DCMIPP_RAWBAYER_BGGR                6U

/* ISP decimation ratios */
#define DCMIPP_HDEC_ALL                     0U
#define DCMIPP_HDEC_1_OUT_2                 2U
#define DCMIPP_HDEC_1_OUT_4                 4U
#define DCMIPP_HDEC_1_OUT_8                 6U
#define DCMIPP_VDEC_ALL                     0U
#define DCMIPP_VDEC_1_OUT_2                 8U
#define DCMIPP_VDEC_1_OUT_4                 16U
#define DCMIPP_VDEC_1_OUT_8                 24U

/* Function prototypes */
void     DCMIPP_Init(void);
void     DCMIPP_DeInit(void);
void     DCMIPP_CSI_Pipe_Config(uint32_t pipe, uint32_t data_type);
void     DCMIPP_Pipe_Config(uint32_t pipe, DCMIPP_Pipe_Conf *conf, uint32_t *out_pitch);
void     DCMIPP_Pipe_EnableShare(uint32_t pipe, uint32_t mode);
void     DCMIPP_IPPlug_Config(DCMIPP_IPPlug_Conf *conf);
void     DCMIPP_Pipe_Start(uint32_t pipe, uint32_t buf_addr, uint32_t mode);
void     DCMIPP_Pipe_Stop(uint32_t pipe);
void     DCMIPP_Pipe_Suspend(uint32_t pipe);
void     DCMIPP_Pipe_Resume(uint32_t pipe);
void     DCMIPP_EnableInterrupts(uint32_t pipe, uint32_t it_mask);
void     DCMIPP_DisableInterrupts(uint32_t pipe, uint32_t it_mask);
void     DCMIPP_ClearInterrupt(uint32_t pipe, uint32_t it_mask);
uint32_t DCMIPP_GetStatus(uint32_t pipe);
void     DCMIPP_Pipe_UpdateBufAddr(uint32_t pipe, uint32_t buf_addr);

/* ISP configuration on PIPE1 */
void     DCMIPP_Pipe_EnableISP(uint32_t pipe, uint32_t bayer_type);
void     DCMIPP_Pipe_SetBlackLevel(uint32_t pipe, uint32_t blk_r, uint32_t blk_g, uint32_t blk_b);
void     DCMIPP_Pipe_EnableBlackLevel(uint32_t pipe);

/* Interrupt handlers - call from IRQ */
void     DCMIPP_IRQHandler(void);

/* Weak callbacks - override in application */
void     DCMIPP_PIPE_FrameEventCallback(uint32_t pipe);
void     DCMIPP_PIPE_VsyncEventCallback(uint32_t pipe);
void     DCMIPP_PIPE_ErrorCallback(uint32_t pipe);

/** Reduce spurious line-event state after pipe config */
void     DCMIPP_ReduceSpurious(void);

/* Helper to get 16-byte aligned pitch */
static inline uint32_t DCMIPP_AlignPitch(uint32_t pitch) {
    return (pitch + 15) & ~15U;
}

extern volatile uint32_t dbg_p1_vsync_count;
extern volatile uint32_t dbg_p1_frame_count;
extern volatile uint32_t dbg_p1_ovr_count;

extern volatile uint32_t dbg_p2_vsync_count;
extern volatile uint32_t dbg_p2_frame_count;
extern volatile uint32_t dbg_p2_ovr_count;

void DCMIPP_DebugResetCounters(void);

#endif /* SIMPLE_DCMIPP_H */
