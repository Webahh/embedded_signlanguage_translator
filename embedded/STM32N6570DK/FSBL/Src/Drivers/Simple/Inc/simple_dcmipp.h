/**
 * @file    simple_dcmipp.h
 * @author  Gross
 * @date    21.05.2026
 * @brief   Register-level DCMIPP + CSI-2 driver header for STM32N6
 *          Supports two-pipe setup: PIPE1 (display) and PIPE2 (NN)
 *
 * Usage
 * -----
 * 1. DCMIPP_Init()                 – reset / enable DCMIPP
 * 2. DCMIPP_Pipe_Config()          – configure pipe (cropping, downscale)
 * 3. DCMIPP_CSI_Pipe_Config()      – link CSI-2 data type to pipe
 * 4. DCMIPP_Pipe_Start()           – start capture
 */

#ifndef SIMPLE_DCMIPP_H
#define SIMPLE_DCMIPP_H

#include <stdint.h>
#include <stddef.h>

#include "stm32n657xx.h"
#include "simple_csi.h"

/** DCMIPP operation status codes */
typedef enum {
    DCMIPP_OK    = 0,
    DCMIPP_ERROR = 1,
} DCMIPP_Status_TypeDef;

// ---- Pipe identifiers ----

#define DCMIPP_PIPE0                    0U
#define DCMIPP_PIPE1                    1U
#define DCMIPP_PIPE2                    2U

// ---- IPPlug client identifiers ----

#define DCMIPP_CLIENT1                  1U
#define DCMIPP_CLIENT2                  2U
#define DCMIPP_CLIENT3                  3U
#define DCMIPP_CLIENT4                  4U
#define DCMIPP_CLIENT5                  5U

// ---- Pixel Packer formats (PxPPCR.FORMAT field) ----

#define DCMIPP_PP_FORMAT_RGB888         0U
#define DCMIPP_PP_FORMAT_RGB565         1U
#define DCMIPP_PP_FORMAT_ARGB8888       2U
#define DCMIPP_PP_FORMAT_RGBA888        3U
#define DCMIPP_PP_FORMAT_MONO8          4U
#define DCMIPP_PP_FORMAT_YUV444         5U
#define DCMIPP_PP_FORMAT_YUV422_YUYV    6U

// ---- IPPlug memory page sizes ----

#define DCMIPP_MEM_PAGE_64B             0U
#define DCMIPP_MEM_PAGE_128B            1U
#define DCMIPP_MEM_PAGE_256B            2U
#define DCMIPP_MEM_PAGE_512B            3U

// ---- IPPlug traffic burst sizes ----

#define DCMIPP_TRAFFIC_8B               0U
#define DCMIPP_TRAFFIC_16B              1U
#define DCMIPP_TRAFFIC_32B              2U
#define DCMIPP_TRAFFIC_64B              3U
#define DCMIPP_TRAFFIC_128B             4U

// ---- Pipe sharing modes for PIPE2 ----

#define DCMIPP_PIPE_SHARE_SAME          0
#define DCMIPP_PIPE_SHARE_DIFFERENT     1

// ---- Interrupt masks (match CMSIS P1IER/P2IER register bits) ----

#define DCMIPP_PIPE_IT_FRAMEIE          DCMIPP_P1IER_FRAMEIE
#define DCMIPP_PIPE_IT_VSYNCIE          DCMIPP_P1IER_VSYNCIE
#define DCMIPP_PIPE_IT_LINEIE           DCMIPP_P1IER_LINEIE
#define DCMIPP_PIPE_IT_OVRIE            DCMIPP_P1IER_OVRIE

// ---- Raw Bayer types for demosaic ----

#define DCMIPP_RAWBAYER_RGGB            0U
#define DCMIPP_RAWBAYER_GRBG            2U
#define DCMIPP_RAWBAYER_GBRG            4U
#define DCMIPP_RAWBAYER_BGGR            6U

// ---- ISP decimation ratios ----

#define DCMIPP_HDEC_ALL                 0U
#define DCMIPP_HDEC_1_OUT_2             2U
#define DCMIPP_HDEC_1_OUT_4             4U
#define DCMIPP_HDEC_1_OUT_8             6U
#define DCMIPP_VDEC_ALL                 0U
#define DCMIPP_VDEC_1_OUT_2             8U
#define DCMIPP_VDEC_1_OUT_4             16U
#define DCMIPP_VDEC_1_OUT_8             24U

// ---- Types ----

/** Pipe configuration structure */
typedef struct {
    uint32_t output_width;       /**< Output frame width (pixels)              */
    uint32_t output_height;      /**< Output frame height (pixels)             */
    uint32_t output_format;      /**< Pixel format (DCMIPP_PP_FORMAT_*)        */
    uint32_t output_bpp;         /**< Bytes per pixel (3 for RGB888, 4 for ARGB8888) */
    uint16_t crop_x;             /**< Crop start X (pixels)                    */
    uint16_t crop_y;             /**< Crop start Y (pixels)                    */
    uint16_t crop_width;         /**< Crop window width (pixels)               */
    uint16_t crop_height;        /**< Crop window height (pixels)              */
    uint8_t  enable_crop;        /**< Enable cropping                          */
    uint8_t  enable_swap;        /**< Red-blue swap enable                     */
    uint8_t  enable_downsize;    /**< Enable downscaling                       */
    uint8_t  enable_decimate;    /**< Enable decimation                        */
    uint8_t  decimate_h;         /**< Horizontal decimation: 0=1x, 1=1/2, 2=1/4, 3=1/8 */
    uint8_t  decimate_v;         /**< Vertical decimation:   0=1x, 1=1/2, 2=1/4, 3=1/8 */
    uint8_t  enable_dbm;         /**< Enable DCMIPP hardware double-buffer mode */
    uint8_t  enable_gamma;       /**< Enable gamma                             */
} DCMIPP_Pipe_cfg_TypeDef;

/** IPPlug client configuration structure */
typedef struct {
    uint32_t client_id;          /**< Client identifier (DCMIPP_CLIENT*)       */
    uint32_t traffic;            /**< Burst size (DCMIPP_TRAFFIC_*)            */
    uint32_t outstanding;        /**< Outstanding transactions (0-15)          */
    uint16_t dpreg_start;        /**< AHB address region start (1-KB units)    */
    uint16_t dpreg_end;          /**< AHB address region end (1-KB units)      */
    uint8_t  wlru_ratio;         /**< WLRU ratio (0-15: 1/16 to 16/16 of BW)  */
} DCMIPP_IPPlug_cfg_TypeDef;

// ---- API ----

void DCMIPP_Init(void);

/**
 * @brief  Configure CSI data type and virtual channel for a pipe
 * @param [in] pipe      | Pipe index (DCMIPP_PIPE0/1/2)
 * @param [in] data_type | CSI-2 data type value
 */
void DCMIPP_CSI_Pipe_Config(uint32_t pipe, uint32_t data_type);

/**
 * @brief  Configure a DCMIPP pipe (crop, downsizer, pixel packer, colour)
 * @param [in]  pipe      | Pipe index (DCMIPP_PIPE1/2)
 * @param [in]  conf      | Pipe configuration parameters
 * @param [out] out_pitch | Aligned line pitch in bytes (may be NULL)
 */
void DCMIPP_Pipe_Config(uint32_t pipe, DCMIPP_Pipe_cfg_TypeDef *conf, uint32_t *out_pitch);

/**
 * @brief  Enable or disable pipe data sharing mode
 * @param [in] pipe | Pipe index
 * @param [in] mode | DCMIPP_PIPE_SHARE_SAME or DCMIPP_PIPE_SHARE_DIFFERENT
 */
void DCMIPP_Pipe_EnableShare(uint32_t pipe, uint32_t mode);

/**
 * @brief  Configure an IPPlug AXI client
 * @param [in] conf | IPPlug client configuration
 */
void DCMIPP_IPPlug_Config(DCMIPP_IPPlug_cfg_TypeDef *conf);

/**
 * @brief  Update the buffer address for a pipe
 * @param [in] pipe     | Pipe index (DCMIPP_PIPE1/2)
 * @param [in] buf_addr | New buffer base address
 */
void DCMIPP_Pipe_UpdateBufAddr(uint32_t pipe, uint32_t buf_addr);

/**
 * @brief  Enable ISP demosaic on a pipe
 * @param [in] pipe       | Pipe index
 * @param [in] bayer_type | DCMIPP_RAWBAYER_* type
 */
void DCMIPP_Pipe_EnableISP(uint32_t pipe, uint32_t bayer_type);

/**
 * @brief  Set black level correction values
 * @param [in] pipe  | Pipe index
 * @param [in] blk_r | Red black level
 * @param [in] blk_g | Green black level
 * @param [in] blk_b | Blue black level
 */
void DCMIPP_Pipe_SetBlackLevel(uint32_t pipe, uint32_t blk_r, uint32_t blk_g, uint32_t blk_b);

/**
 * @brief  Enable black level correction
 * @param [in] pipe | Pipe index
 */
void DCMIPP_Pipe_EnableBlackLevel(uint32_t pipe);

/**
 * @brief  Reduce spurious line-event state after pipe config
 *         Workaround from ST reference implementation
 */
void DCMIPP_ReduceSpurious(void);

/**
 * @brief  Start capture on a pipe
 * @param [in] pipe     | Pipe index (DCMIPP_PIPE1/2)
 * @param [in] buf_addr | Buffer address (0 to use previously configured)
 * @param [in] mode     | Capture mode (0 = continuous, 1 = snapshot)
 */
void DCMIPP_Pipe_Start(uint32_t pipe, uint32_t buf_addr, uint32_t mode);

/**
 * @brief  Stop capture on a pipe
 * @param [in] pipe | Pipe index (DCMIPP_PIPE1/2)
 */
void DCMIPP_Pipe_Stop(uint32_t pipe);

/**
 * @brief  Suspend capture on a pipe (disable pipe)
 * @param [in] pipe | Pipe index (DCMIPP_PIPE1/2)
 */
void DCMIPP_Pipe_Suspend(uint32_t pipe);

/**
 * @brief  Resume capture on a pipe (re-enable pipe)
 * @param [in] pipe | Pipe index (DCMIPP_PIPE1/2)
 */
void DCMIPP_Pipe_Resume(uint32_t pipe);

/**
 * @brief  Enable pipe interrupts
 * @param [in] pipe    | Pipe index (DCMIPP_PIPE1/2)
 * @param [in] it_mask | ORed DCMIPP_PIPE_IT_* flags
 */
void DCMIPP_EnableInterrupts(uint32_t pipe, uint32_t it_mask);

/**
 * @brief  Disable pipe interrupts
 * @param [in] pipe    | Pipe index (DCMIPP_PIPE1/2)
 * @param [in] it_mask | ORed DCMIPP_PIPE_IT_* flags
 */
void DCMIPP_DisableInterrupts(uint32_t pipe, uint32_t it_mask);

/**
 * @brief  Clear pipe interrupt flags
 * @param [in] pipe    | Pipe index (DCMIPP_PIPE1/2)
 * @param [in] it_mask | ORed DCMIPP_PIPE_IT_* flags
 */
void DCMIPP_ClearInterrupt(uint32_t pipe, uint32_t it_mask);

/**
 * @brief  Read pipe status register
 * @param [in]  pipe   | Pipe index (DCMIPP_PIPE1/2)
 * @param [out] status | Pipe status register value
 * @retval DCMIPP_OK    Status read successfully
 * @retval DCMIPP_ERROR Invalid pipe index
 */
DCMIPP_Status_TypeDef DCMIPP_GetStatus(uint32_t pipe, uint32_t *status);

/** Interrupt handler - call from IRQ */
void DCMIPP_IRQHandler(void);

/** Weak callbacks - override in application */
void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe);
void DCMIPP_PIPE_VsyncEventCallback(uint32_t pipe);
void DCMIPP_PIPE_ErrorCallback(uint32_t pipe);

#endif /* SIMPLE_DCMIPP_H */
