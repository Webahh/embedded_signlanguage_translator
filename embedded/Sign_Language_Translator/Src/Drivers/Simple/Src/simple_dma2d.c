#include <stddef.h>

#include "stm32n657xx.h"

#include "simple_dma2d.h"
#include "simple_scheduler.h"
#include "simple_rcc.h"

#define DMA2D_TIMEOUT_MS 100UL

static volatile int          _busy          = 0;
static DMA2D_Handle_TypeDef *_active_handle = NULL;

void DMA2D_Init(DMA2D_Handle_TypeDef *h)
{
    if (!h) return;

    RCC_enable_DMA2D();
    RCC_reset_DMA2D();

    NVIC_SetPriority(DMA2D_IRQn, 7);
    NVIC_EnableIRQ(DMA2D_IRQn);

    h->completed = 0;
    _busy = 0;
    _active_handle = h;
}

DMA2D_Status_TypeDef DMA2D_Transfer(DMA2D_Handle_TypeDef *h)
{
	if (!h) return DMA2D_ERROR_PARAM;
	if (_busy) return DMA2D_ERROR_BUSY;

	_busy = 1;
	h->completed = 0;

	// Save owning task BEFORE starting the HW (ISR may fire immediately)
	int task_idx;
	SCHEDULER_GetCurrentTask(&task_idx);
	h->task_owner = (uint8_t)task_idx;

	DMA2D->FGMAR  = h->cfg.src.address;
	DMA2D->FGOR   = h->cfg.src.line_offset;

	DMA2D->FGPFCCR = (uint32_t)h->cfg.src.format << DMA2D_FGPFCCR_CM_Pos;

	DMA2D->OMAR    = h->cfg.dst.address;
	DMA2D->OOR     = h->cfg.dst.line_offset;
	DMA2D->OPFCCR  = (uint32_t)h->cfg.dst.format << DMA2D_OPFCCR_CM_Pos;

	DMA2D->NLR = (h->cfg.height_lines << DMA2D_NLR_NL_Pos)
	           | (h->cfg.width_pixels << DMA2D_NLR_PL_Pos);

	DMA2D->IFCR = DMA2D_IFCR_CTCIF
	            | DMA2D_IFCR_CTEIF
	            | DMA2D_IFCR_CCEIF;

	DMA2D->CR = ((uint32_t)h->cfg.mode << DMA2D_CR_MODE_Pos)
	          | DMA2D_CR_TCIE
	          | DMA2D_CR_START;

	// Atomically check for early completion and suspend
	__disable_irq();
	if (!h->completed) {
		SCHEDULER_Task_suspend_self();
		// After DMA2D ISR resumes us: execution continues here
	}
	__enable_irq();

	_busy = 0;
	return DMA2D_OK;
}

void DMA2D_IRQHandler(void)
{
	SCHEDULER_ISR_enter();

	DMA2D_Handle_TypeDef *h = _active_handle;
	if (!h) {
		SCHEDULER_ISR_exit();
		return;
	}

	uint32_t isr = DMA2D->ISR;

	if (isr & DMA2D_ISR_TCIF) {
		DMA2D->IFCR = DMA2D_IFCR_CTCIF;
		h->completed = 1;
		SCHEDULER_Task_resume(h->task_owner);
	}

	if (isr & DMA2D_ISR_TEIF) {
		DMA2D->IFCR = DMA2D_IFCR_CTEIF;
		h->completed = 1;
		SCHEDULER_Task_resume(h->task_owner);
	}

	if (isr & DMA2D_ISR_CEIF) {
		DMA2D->IFCR = DMA2D_IFCR_CCEIF;
		h->completed = 1;
		SCHEDULER_Task_resume(h->task_owner);
	}

	if (isr & DMA2D_ISR_CAEIF) {
		DMA2D->IFCR = DMA2D_IFCR_CAECIF;
		h->completed = 1;
		SCHEDULER_Task_resume(h->task_owner);
	}

	SCHEDULER_ISR_exit();
}
