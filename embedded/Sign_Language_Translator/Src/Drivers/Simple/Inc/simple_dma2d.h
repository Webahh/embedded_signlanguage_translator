#ifndef SIMPLE_DMA2D_H
#define SIMPLE_DMA2D_H

#include <stdint.h>

typedef enum {
    DMA2D_OK           =  0,
    DMA2D_ERROR_BUSY   = -1,
    DMA2D_ERROR_PARAM  = -2,
    DMA2D_ERROR_HW     = -3,
} DMA2D_Status_TypeDef;

typedef enum {
    DMA2D_FORMAT_ARGB8888 = 0x0,
    DMA2D_FORMAT_RGB888   = 0x1,
    DMA2D_FORMAT_RGB565   = 0x2,
    DMA2D_FORMAT_ARGB1555 = 0x3,
    DMA2D_FORMAT_ARGB4444 = 0x4,
    DMA2D_FORMAT_L8       = 0x5,
    DMA2D_FORMAT_AL44     = 0x6,
    DMA2D_FORMAT_AL88     = 0x7,
    DMA2D_FORMAT_L4       = 0x8,
    DMA2D_FORMAT_A8       = 0x9,
    DMA2D_FORMAT_A4       = 0xA,
    DMA2D_FORMAT_YCbCr    = 0xB,
} DMA2D_ColorFormat_TypeDef;

typedef enum {
    DMA2D_MODE_MEM_TO_MEM            = 0,
    DMA2D_MODE_MEM_TO_MEM_PFC        = 1,
    DMA2D_MODE_MEM_TO_MEM_BLEND      = 2,
    DMA2D_MODE_REG_TO_MEM            = 3,
    DMA2D_MODE_MEM_TO_MEM_FIXED_FG   = 4,
    DMA2D_MODE_MEM_TO_MEM_FIXED_BG   = 5,
} DMA2D_TransferMode_TypeDef;

typedef struct {
    uint32_t                   address;
    uint32_t                   line_offset;
    DMA2D_ColorFormat_TypeDef  format;
} DMA2D_Buffer_TypeDef;

typedef struct {
    DMA2D_Buffer_TypeDef      src;
    DMA2D_Buffer_TypeDef      dst;
    uint32_t                  width_pixels;
    uint32_t                  height_lines;
    DMA2D_TransferMode_TypeDef mode;
} DMA2D_Config_TypeDef;

typedef struct {
    DMA2D_Config_TypeDef  cfg;
    volatile uint8_t      completed;
    uint8_t               task_owner;
} DMA2D_Handle_TypeDef;

void     DMA2D_Init(DMA2D_Handle_TypeDef *h);
DMA2D_Status_TypeDef DMA2D_Transfer(DMA2D_Handle_TypeDef *h);

#endif
