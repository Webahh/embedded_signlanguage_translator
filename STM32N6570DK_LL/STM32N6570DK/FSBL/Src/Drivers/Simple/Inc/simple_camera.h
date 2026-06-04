#ifndef SIMPLE_CAMERA_H
#define SIMPLE_CAMERA_H

#include "stm32n657xx.h"
#include "simple_dcmipp.h"
#include "simple_imx335.h"

#define CAM_PIPE_DISPLAY        DCMIPP_PIPE1
#define CAM_PIPE_NN             DCMIPP_PIPE2
#define CAM_CLIENT_DISPLAY      DCMIPP_CLIENT5
#define CAM_CLIENT_NN           DCMIPP_CLIENT2

#define CAM_DISPLAY_WIDTH       800U
#define CAM_DISPLAY_HEIGHT      480U
#define CAM_NN_WIDTH            192U
#define CAM_NN_HEIGHT           144U

/* Default buffer addresses (SRAM at 0x34000000) */
#define CAM_DISPLAY_BUF0        0x34000000U
#define CAM_DISPLAY_BUF1        0x340C0000U
#define CAM_NN_BUF              0x34180000U

typedef enum {
    CAM_OK = 0,
    CAM_ERROR,
    CAM_ERROR_ID,
    CAM_ERROR_INIT,
} CAM_Status;

typedef struct {
    IMX335_Handle imx335;
    uint32_t display_buf0;
    uint32_t display_buf1;
    uint32_t nn_buf;
    uint32_t active_write_buf;
    uint32_t active_display_buf;
    int32_t display_pitch;
    int32_t nn_pitch;
    uint8_t initialized;
} CAM_Handle;

CAM_Status CAM_Init(CAM_Handle *h, uint32_t nn_buf);
CAM_Status CAM_DisplayPipe_Start(CAM_Handle *h);
CAM_Status CAM_NNPipe_Start(CAM_Handle *h);

#endif
