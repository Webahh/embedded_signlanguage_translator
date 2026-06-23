/**
  ******************************************************************************
  * @file    simple_camera.h
  * @author  Groß
  * @brief   Top-level camera subsystem header
  ******************************************************************************
  */

#ifndef SIMPLE_CAMERA_H
#define SIMPLE_CAMERA_H

#include "stm32n657xx.h"
#include "simple_dcmipp.h"
#include "simple_imx335.h"

/* ---------------------------------------------------------------------------
 * Pipe / client aliases
 * ------------------------------------------------------------------------- */

#define CAM_PIPE_DISPLAY        DCMIPP_PIPE1
#define CAM_PIPE_NN             DCMIPP_PIPE2
#define CAM_CLIENT_NN           DCMIPP_CLIENT2
#define CAM_CLIENT_DISPLAY      DCMIPP_CLIENT5

/* ---------------------------------------------------------------------------
 * Sensor native output resolution
 * ------------------------------------------------------------------------- */

#define CAM_SENSOR_WIDTH        2592U
#define CAM_SENSOR_HEIGHT       1944U

/* ---------------------------------------------------------------------------
 * Display output dimensions (pixels)
 * ------------------------------------------------------------------------- */

#define CAM_DISPLAY_WIDTH       800U
#define CAM_DISPLAY_HEIGHT      480U

/* ---------------------------------------------------------------------------
 * NN input dimensions (pixels)
 * ------------------------------------------------------------------------- */

#define CAM_NN_WIDTH            192U
#define CAM_NN_HEIGHT           144U

/** NN inference buffer base address */
#define CAM_NN_BUF              0x34180000U

/* ---------------------------------------------------------------------------
 * Return codes
 * ------------------------------------------------------------------------- */

/** CAM function return codes */
typedef enum {
    CAM_OK         = 0, /**< Success              */
    CAM_ERROR      = 1, /**< Unspecified error    */
    CAM_ERROR_ID   = 2, /**< IMX335 Probe/ID fail */
    CAM_ERROR_INIT = 3, /**< IMX335 init failure  */
} CAM_Status;

/* ---------------------------------------------------------------------------
 * Camera instance handle
 * ------------------------------------------------------------------------- */

/** Top-level camera instance handle */
typedef struct {
    IMX335_Handle_TypeDef imx335;             /**< IMX335 sensor handle              */
    uint32_t      display_buf;        /**< Display buffer base address       */
    uint32_t      nn_buf;             /**< NN buffer base address            */
    int32_t       display_pitch;      /**< Display pipe line pitch (bytes)   */
    int32_t       nn_pitch;           /**< NN pipe line pitch (bytes)        */
    int           display_disp_idx;   /**< Current LTDC display buffer index */
    int           display_capt_idx;   /**< Current DCMIPP capture buffer idx */
    uint8_t       initialized;        /**< Non-zero after successful Init    */
} CAM_Handle;

/* ---------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
  * @brief  Initialise the entire camera subsystem
  * @param  h      Camera handle (output)
  * @param  nn_buf NN buffer base address (0 = use default CAM_NN_BUF)
  * @retval CAM_OK on success
  */
CAM_Status CAM_Init(CAM_Handle *h);

/**
  * @brief  Start the display pipe and sensor streaming
  * @param  h Camera handle
  * @retval CAM_OK on success
  */
CAM_Status CAM_DisplayPipe_Start(CAM_Handle *h);

/**
  * @brief  Start the NN pipe (capture to nn_buf)
  * @param  h Camera handle
  * @retval CAM_OK on success
  */
CAM_Status CAM_NNPipe_Start(CAM_Handle *h);

#endif /* SIMPLE_CAMERA_H */
