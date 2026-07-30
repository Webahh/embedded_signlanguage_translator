/*
 * tasks.c
 *
 *  Created on: 05.06.2026
 *      Author: Weber
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "tasks.h"
#include "app.h"
#include "simple_gpio.h"
#include "simple_scheduler.h"

#include "simple_ltdc.h"
#include "simple_ltdc_layer.h"
#include "simple_ltdc_color.h"
#include "simple_ltdc_layer_draw.h"

#include "simple_ae.h"
#include "config.h"
#include "simple_timer.h"
#include "simple_text.h"
#include "simple_touch.h"
#include "palm_detection.h"
#include "hand_landmark.h"
#include "fingeralphabet.h"
#include "simple_text.h"
#include "simple_dma2d.h"
#include "ui.h"

/* D-Cache coherency: LTDC and DMA2D read PSRAM directly (bypass D-cache),
   so CPU-written data must be flushed to memory before DMA can see it,
   and DMA-written data must be invalidated before CPU reads it.          */
#define CACHE_CLEAN(addr, size)  SCB_CleanDCache_by_Addr((void*)(addr), (int32_t)(size))
#define CACHE_INVAL(addr, size)  SCB_InvalidateDCache_by_Addr((void*)(addr), (int32_t)(size))

/* ISR-safe copy of landmarks for per-frame redraw in DCMIPP frame callback */
static volatile uint8_t 		_isr_landmark_valid = 0U;
static LandmarkPoint_TypeDef 	_isr_landmark_points[LANDMARK_POINT_COUNT];
static HandROI_TypeDef 			_isr_landmark_roi;

/* ISR-safe mirrors of UI visibility toggles (updated by AI pipeline task) */
static volatile uint8_t _isr_palm_vis = 1U;
static volatile uint8_t _isr_hand_vis = 1U;
volatile uint8_t isr_systime_vis = 0U;
volatile uint8_t isr_sysinfo_vis = 0U;
volatile uint8_t isr_sign_vis = 1U;
static char _systime_str[11];
static char _sysinfo_p[12];
static char _sysinfo_h[12];
static char _sysinfo_s[12];
static char _sysinfo_t[12];
static char _sign_str[16];

static volatile int 	ltdc_fg_disp_idx = 1;
static volatile uint8_t nn_frame_ready = 0U;
static volatile uint8_t nn_completed_buffer_idx = 0U;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1) {
        AE_OnFrameStats();

        int next_disp_idx = (ltdc_layer_bg_buffer_disp_idx + 1) % LTDC_LAYER_DISPLAY_BUFFER_NB;
        int next_capt_idx = (ltdc_layer_bg_buffer_capt_idx + 1) % LTDC_LAYER_DISPLAY_BUFFER_NB;

        DCMIPP_Pipe_UpdateBufAddr(CAM_PIPE_DISPLAY, (uint32_t)&ltdc_layer_bg_buffer[next_capt_idx]);

        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[next_disp_idx];
        LTDC_Layer_Address_Set(&LTDC_Layer1Config);

        int ai_idx =
            (next_disp_idx + LTDC_LAYER_AI_LOOKAHEAD_FRAMES) %
            LTDC_LAYER_DISPLAY_BUFFER_NB;

        int draw_idx = (next_disp_idx + 1U) % LTDC_LAYER_DISPLAY_BUFFER_NB;

        ltdc_layer_bg_buffer_ai_idx   = ai_idx;
        ltdc_layer_bg_buffer_disp_idx = next_disp_idx;
        ltdc_layer_bg_buffer_capt_idx = next_capt_idx;
        ltdc_layer_bg_buffer_draw_idx = draw_idx;

        LTDC_Layer_Config_TypeDef isr_draw_cfg = LTDC_Layer1Config;
        isr_draw_cfg.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[next_disp_idx];

        // Redraw last known landmarks into the next-to-be-displayed buffer
        // to prevent flicker when the AI pipeline misses a frame.
        if (_isr_landmark_valid) {
            if (_isr_hand_vis) {
                LTDC_Layer_Draw_LandmarksDirect(&isr_draw_cfg, _isr_landmark_points);
            }
            if (_isr_palm_vis) {
                LTDC_Layer_Draw_ROIDirect(&isr_draw_cfg, &_isr_landmark_roi, LTDC_LAYER_COLOR_BLUE);
            }
        }

        if (isr_systime_vis) {
            TEXT_StringBg_draw(&isr_draw_cfg, _systime_str, 720, 0, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
        }

        if (isr_sysinfo_vis) {
            LTDC_Layer_Draw_Rect(&isr_draw_cfg, 720, 16, 80, 64, 0x00000000U);
            TEXT_StringBg_draw(&isr_draw_cfg, _sysinfo_p, 720, 16, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
            TEXT_StringBg_draw(&isr_draw_cfg, _sysinfo_h, 720, 32, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
            TEXT_StringBg_draw(&isr_draw_cfg, _sysinfo_s, 720, 48, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
            TEXT_StringBg_draw(&isr_draw_cfg, _sysinfo_t, 720, 64, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
        }

        if (isr_sign_vis) {
            LTDC_Layer_Draw_Rect(&isr_draw_cfg, 720, 80, 80, 16, 0x00000000U);
            TEXT_StringBg_draw(&isr_draw_cfg, _sign_str, 720, 80, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
        }

        // Clean so LTDC sees drawn content (Landmarks/Palm/Systemtime/Systeminfo/Sign)
        CACHE_CLEAN(&ltdc_layer_bg_buffer[next_disp_idx], sizeof(ltdc_layer_bg_buffer[0]));

    } else if (pipe == DCMIPP_PIPE2) {
        nn_completed_buffer_idx = (DCMIPP->P2SR & DCMIPP_P2SR_LSTFRM) ? 1U : 0U;
        nn_frame_ready = 1U;
    }
}

void vSystemTimeTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now); // MAX:     4294967296
	snprintf(_systime_str, sizeof(_systime_str), "%lu", (unsigned long)now);
	isr_systime_vis = 1U;
}

void vLEDTask(void) {
	GPIO_BSRR_toggle(GPIOG, LED2_PIN);
}

void vAETask(void){
	AE_Process(&h_cam);
}

// Poll touch, dispatch to drawer, draw a blue dot on press
void vTouchTask(void){
    static uint32_t last_ms;
    uint32_t now;
    SCHEDULER_Tick_get(&now);
//    if (now - last_ms < 30) return;

    uint8_t pending = 0;
    TOUCH_GetPending(&pending);
    if (pending)
    {
        last_ms = now;

        TOUCH_Data_TypeDef data;
        TOUCH_GetState(NULL, &data);

        // Route touch events to drawer (toggle, slider, selector, composite)
        UI_Drawer_HandleTouch(&_drawer, data.x, data.y, data.pressed,
            &LTDC_Layer2Config, NULL);

        // Paint touch feedback
        if (data.pressed)
        {
        	int next_idx = ltdc_layer_bg_buffer_disp_idx;
            LTDC_Layer_Config_TypeDef tmp = LTDC_Layer1Config;
            tmp.fb = (void *)&ltdc_layer_bg_buffer[next_idx];
            LTDC_Layer_Draw_Circle(&tmp, data.x, data.y, 5, LTDC_LAYER_COLOR_BLUE);
            int32_t x0 = (data.x > 5) ? data.x - 5 : 0;
            int32_t y0 = (data.y > 5) ? data.y - 5 : 0;
            uint32_t fb = (uint32_t)&ltdc_layer_bg_buffer[next_idx];
            CACHE_CLEAN(fb + ((uint32_t)y0 * LTDC_LAYER_BG_WIDTH + (uint32_t)x0) * 3U, 11U * 11U * 3U);
        }
    }
}

/* --------------------------------------------------------------------------
 * AI Pipeline Defs & Helper
 * -------------------------------------------------------------------------- */

typedef enum {
    HAND_STATE_PALM_SEARCH = 0,
    HAND_STATE_LANDMARK_TRACKING
} HandTrackingState_TypeDef;

typedef struct {
    uint8_t ai_mode;
    uint8_t palm_vis;
    uint8_t hand_vis;
    uint8_t sign_vis;
} AIPipelineUi_TypeDef;


#define LANDMARK_PRESENCE_THRESHOLD  	0.5f
#define LANDMARK_LOST_FRAME_COUNT    	3U
#define PALM_SEARCH_INTERVAL_MS      	200U
#define FINGERALPHABET_INTERVAL_MS   	100U
#define LANDMARK_TRACK_INTERVAL_MS   	0U
#define LANDMARK_PREDICTION_TIME_MS     25.0f
#define LANDMARK_PREDICTION_MAX_DELTA   0.08f
#define LANDMARK_SMOOTHING_ALPHA        0.75f

static PalmNetworkOutput_TypeDef palm_output;
static PalmDetection_TypeDef palm_detection;
static PalmDetectionFilter_TypeDef palm_filter;
static HandROI_TypeDef landmark_roi;

static LandmarkNetworkOutput_TypeDef landmark_output;
static uint8_t landmark_preprocessed_input[LANDMARK_INPUT_SIZE] __attribute__((aligned(32)));
static LandmarkPoint_TypeDef landmark_points[LANDMARK_POINT_COUNT];

static uint8_t fingeralphabet_input[FINGERALPHABET_INPUT_SIZE];
static uint8_t fingeralphabet_output[FINGERALPHABET_OUTPUT_SIZE];
static FingeralphabetResult_TypeDef fingeralphabet_result = { .class_index = 0, .score = 0, .label = "NONE" };

static DMA2D_Handle_TypeDef _dma2d;
static int _dma2d_initialized = 0;

static HandTrackingState_TypeDef hand_state = HAND_STATE_PALM_SEARCH;

static uint32_t last_valid_landmark_output_tick = 0U;
static uint8_t landmark_lost_count = 0U;
static uint32_t last_palm_search_tick = 0U;
static uint32_t last_landmark_tick = 0U;
static uint32_t last_fingeralphabet_tick = 0U;

static uint8_t _saved_camera_buffer_idx;
static uint32_t _palm_duration_ms;
static uint32_t _landmark_duration_ms;
static uint32_t _fingeralphabet_duration_ms;
static uint32_t _pipeline_duration_ms;
static uint32_t _palm_start_tick;
static uint32_t _landmark_start_tick;
static uint32_t _fingeralphabet_start_tick;
static uint32_t _pipeline_start_tick;

static LandmarkPoint_TypeDef previous_landmark_points[LANDMARK_POINT_COUNT];
static LandmarkPoint_TypeDef smoothed_landmark_points[LANDMARK_POINT_COUNT];
static LandmarkPoint_TypeDef predicted_landmark_points[LANDMARK_POINT_COUNT];

static uint8_t previous_landmarks_valid = 0U;
static uint32_t previous_landmark_tick = 0U;

static LandmarkPoint_TypeDef last_current_landmark_points[LANDMARK_POINT_COUNT];
static float last_landmark_rotation = 0.0f;

static void DMA2D_EnsureInit(void)
{
    if (!_dma2d_initialized) {
        DMA2D_Init(&_dma2d);
        _dma2d_initialized = 1;
    }
}

static float _clampf(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void _predictLandmarksTimed(const LandmarkPoint_TypeDef current[LANDMARK_POINT_COUNT],
								   LandmarkPoint_TypeDef predicted[LANDMARK_POINT_COUNT])
{
    uint32_t now;
    SCHEDULER_Tick_get(&now);

    if ((current == NULL) || (predicted == NULL)) {
        return;
    }

    if (previous_landmarks_valid == 0U) {
        memcpy(previous_landmark_points, current, sizeof(previous_landmark_points));
        memcpy(smoothed_landmark_points, current, sizeof(smoothed_landmark_points));
        memcpy(predicted, current, sizeof(predicted_landmark_points));

        previous_landmark_tick = now;
        previous_landmarks_valid = 1U;
        return;
    }

    uint32_t dt_ms_u32 = now - previous_landmark_tick;
    previous_landmark_tick = now;

    if (dt_ms_u32 == 0U) {
        dt_ms_u32 = 1U;
    }

    float dt_ms = (float)dt_ms_u32;
    float prediction_factor = LANDMARK_PREDICTION_TIME_MS / dt_ms;

    prediction_factor = _clampf(prediction_factor, 0.0f, 1.5f);

    for (uint32_t i = 0U; i < LANDMARK_POINT_COUNT; i++) {
        float dx = current[i].x - previous_landmark_points[i].x;
        float dy = current[i].y - previous_landmark_points[i].y;

        dx = _clampf(dx, -LANDMARK_PREDICTION_MAX_DELTA, LANDMARK_PREDICTION_MAX_DELTA);
        dy = _clampf(dy, -LANDMARK_PREDICTION_MAX_DELTA, LANDMARK_PREDICTION_MAX_DELTA);

        float px = current[i].x + dx * prediction_factor;
        float py = current[i].y + dy * prediction_factor;

        px = _clampf(px, 0.0f, 1.0f);
        py = _clampf(py, 0.0f, 1.0f);

        smoothed_landmark_points[i].x = LANDMARK_SMOOTHING_ALPHA * px + (1.0f - LANDMARK_SMOOTHING_ALPHA) * smoothed_landmark_points[i].x;
        smoothed_landmark_points[i].y = LANDMARK_SMOOTHING_ALPHA * py + (1.0f - LANDMARK_SMOOTHING_ALPHA) * smoothed_landmark_points[i].y;

        smoothed_landmark_points[i].z = current[i].z;

        predicted[i] = smoothed_landmark_points[i];
    }

    memcpy(previous_landmark_points, current, sizeof(previous_landmark_points));

    // Mirror to ISR-safe copy for flicker-free redraw
    _isr_landmark_valid = 0U;
    memcpy((void *)_isr_landmark_points, predicted, sizeof(_isr_landmark_points));
    memcpy((void *)&_isr_landmark_roi, (const void *)&landmark_roi, sizeof(_isr_landmark_roi));
    _isr_landmark_valid = 1U;
}

static void _clearPredictedOverlay(void)
{
    previous_landmarks_valid = 0U;
    _isr_landmark_valid = 0U;
}

static bool _isLandmarkValid(const LandmarkNetworkOutput_TypeDef *output)
{
    if (output == NULL) {
        return false;
    }

    return output->presence >= LANDMARK_PRESENCE_THRESHOLD;
}

static void _resetTracking(void)
{
    hand_state = HAND_STATE_PALM_SEARCH;
    landmark_lost_count = 0U;

    _clearPredictedOverlay();
}

static void _handleInvalidLandmark(void)
{
    if (landmark_lost_count < LANDMARK_LOST_FRAME_COUNT) {
        landmark_lost_count++;
    }

    if (landmark_lost_count >= LANDMARK_LOST_FRAME_COUNT) {
        DEBUG_PRINTF("AI state: PALM_SEARCH\r\n");
        _resetTracking();
    }
}

/* --------------------------------------------------------------------------
 * AI Pipeline State Machine
 * -------------------------------------------------------------------------- */

static bool _aiRunLandmarkBlocking(uint8_t camera_buffer_idx)
{
    // Invalidate camera buffer before CPU reads it (camera/DMA wrote to it)
    CACHE_INVAL(&ltdc_layer_bg_buffer[camera_buffer_idx], sizeof(ltdc_layer_bg_buffer[0]));
    AI_Status_TypeDef status = LANDMARK_PreprocessROI((const uint8_t *)ltdc_layer_bg_buffer[camera_buffer_idx],
			 	 	 	 	 	 	 	 	 	 	   LTDC_Layer1Config.width, LTDC_Layer1Config.height,
													   LTDC_Layer1Config.buf_width * LANDMARK_INPUT_CHANNELS,
													   &landmark_roi, landmark_preprocessed_input);
    if (status != AI_STATUS_OK) {
        return false;
    }

    DMA2D_EnsureInit();

    uint8_t *landmark_npu_buf = LANDMARK_GetInputBuffer();

    if (landmark_npu_buf == NULL) {
        return false;
    }

    _dma2d.cfg.src.address = (uint32_t)landmark_preprocessed_input;
    _dma2d.cfg.src.line_offset = 0;
    _dma2d.cfg.src.format = DMA2D_FORMAT_RGB888;
    _dma2d.cfg.dst.address = (uint32_t)landmark_npu_buf;
    _dma2d.cfg.dst.line_offset = 0;
    _dma2d.cfg.dst.format = DMA2D_FORMAT_RGB888;
    _dma2d.cfg.width_pixels = LANDMARK_INPUT_WIDTH;
    _dma2d.cfg.height_lines = LANDMARK_INPUT_HEIGHT;
    _dma2d.cfg.mode = DMA2D_MODE_MEM_TO_MEM;

    // Clean preprocessed input so DMA2D reads CPU-written data
    CACHE_CLEAN((void*)(uint32_t)landmark_preprocessed_input,
                LANDMARK_INPUT_WIDTH * LANDMARK_INPUT_HEIGHT * LANDMARK_INPUT_CHANNELS);

    if (DMA2D_Transfer(&_dma2d) != DMA2D_OK) {
        return false;
    }

    SCHEDULER_Tick_get(&_landmark_start_tick);
    if (LANDMARK_Run(&landmark_output) != AI_STATUS_OK) {
        return false;
    }

    return true;
}

static void _aiRunFingeralphabetIfDue(const AIPipelineUi_TypeDef *ui)
{
    uint32_t now;
    SCHEDULER_Tick_get(&now);

    if ((now - last_fingeralphabet_tick) < FINGERALPHABET_INTERVAL_MS) {
        return;
    }

    last_fingeralphabet_tick = now;

    if (FINGERALPHABET_Preprocess(landmark_points, landmark_output.handedness, fingeralphabet_input) != AI_STATUS_OK) {
        DEBUG_PRINTF("Fingeralphabet preprocessing failed\r\n");
        return;
    }

    SCHEDULER_Tick_get(&_fingeralphabet_start_tick);

    if (FINGERALPHABET_Run(fingeralphabet_input, fingeralphabet_output) != AI_STATUS_OK) {
        DEBUG_PRINTF("Fingeralphabet inference failed\r\n");
        return;
    }

    {
        uint32_t now;
        SCHEDULER_Tick_get(&now);
        _fingeralphabet_duration_ms = now - _fingeralphabet_start_tick;
    }

    fingeralphabet_result = FINGERALPHABET_GetResult(fingeralphabet_output);

    if ((ui != NULL) && ui->sign_vis) {
        DEBUG_PRINTF("Index: %d Result: %s\r\n",
                     fingeralphabet_result.class_index,
                     fingeralphabet_result.label);
    }
}

static void _aiUpdateUiContext(AIPipelineUi_TypeDef *ui)
{
    if (ui == NULL) {
        return;
    }

    ui->ai_mode  = _drawer.items[0].value;
    ui->palm_vis = _drawer.items[1].composite.visible;
    ui->hand_vis = _drawer.items[2].composite.visible;
    ui->sign_vis = _drawer.items[3].composite.visible;

    /* Mirror visibility toggles to ISR-safe copies for per-frame redraw */
    _isr_palm_vis = ui->palm_vis;
    _isr_hand_vis = ui->hand_vis;
}

static bool _aiCopyPalmInputFromCamera(uint8_t completed_idx)
{
    uint8_t *palm_input = PALM_GetInputBuffer();

    if (palm_input == NULL) {
        return false;
    }

    DMA2D_EnsureInit();

    /* DCMIPP wrote this buffer directly. Discard stale D-cache copy. */
    CACHE_INVAL(&ltdc_layer_nn_raw_buffer[completed_idx], sizeof(ltdc_layer_nn_raw_buffer[0]));

    _dma2d.cfg.src.address = (uint32_t)ltdc_layer_nn_raw_buffer[completed_idx];
    _dma2d.cfg.src.line_offset = 0;
    _dma2d.cfg.src.format = DMA2D_FORMAT_RGB888;

    _dma2d.cfg.dst.address = (uint32_t)palm_input;
    _dma2d.cfg.dst.line_offset = 0;
    _dma2d.cfg.dst.format = DMA2D_FORMAT_RGB888;

    _dma2d.cfg.width_pixels = PALM_INPUT_WIDTH;
    _dma2d.cfg.height_lines = PALM_INPUT_HEIGHT;
    _dma2d.cfg.mode = DMA2D_MODE_MEM_TO_MEM;

    return (DMA2D_Transfer(&_dma2d) == DMA2D_OK);
}

static bool _aiRunPalmDetection(uint8_t completed_idx)
{
    uint32_t now;

    if (!_aiCopyPalmInputFromCamera(completed_idx)) {
        return false;
    }

    SCHEDULER_Tick_get(&_palm_start_tick);

    if (PALM_Run(&palm_output) != AI_STATUS_OK) {
        return false;
    }

    SCHEDULER_Tick_get(&now);
    _palm_duration_ms = now - _palm_start_tick;

    const AI_Status_TypeDef palm_status = PALM_Postprocess(&palm_output, &palm_detection);
    const bool palm_valid = (palm_status == AI_STATUS_OK);

    PALM_UpdateDetectionFilter(&palm_filter, palm_valid);

    if ((!palm_valid) || (!palm_filter.detected)) {
        if (!palm_filter.detected) {
            _clearPredictedOverlay();
        }

        return false;
    }

    if (PALM_CreateLandmarkROI(&palm_detection, LTDC_Layer1Config.width,
                               LTDC_Layer1Config.height, &landmark_roi) != AI_STATUS_OK) {
        _resetTracking();
        return false;
    }

    return true;
}

static void _aiDrawPalmOnly(void)
{
    LTDC_Layer_Config_TypeDef draw_cfg = LTDC_Layer1Config;
    draw_cfg.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[ltdc_layer_bg_buffer_draw_idx];

    LTDC_Layer_Draw_ROIDirect(&draw_cfg, &landmark_roi, LTDC_LAYER_COLOR_BLUE);
    CACHE_CLEAN(&ltdc_layer_bg_buffer[ltdc_layer_bg_buffer_draw_idx], sizeof(ltdc_layer_bg_buffer[0]));
}

static void _aiDrawTrackingOverlay(const AIPipelineUi_TypeDef *ui)
{
    LTDC_Layer_Config_TypeDef draw_cfg = LTDC_Layer1Config;

    if (ui == NULL) {
        return;
    }

    draw_cfg.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[ltdc_layer_bg_buffer_draw_idx];

    if (ui->hand_vis) {
        LTDC_Layer_Draw_LandmarksDirect(&draw_cfg, predicted_landmark_points);
    }

    if (ui->palm_vis) {
        LTDC_Layer_Draw_ROIDirect(&draw_cfg, &landmark_roi, LTDC_LAYER_COLOR_BLUE);
    }
    CACHE_CLEAN(&ltdc_layer_bg_buffer[ltdc_layer_bg_buffer_draw_idx], sizeof(ltdc_layer_bg_buffer[0]));
}

static bool _aiRunLandmarkPass(bool from_palm, const AIPipelineUi_TypeDef *ui)
{
    uint32_t now;

    if (!_aiRunLandmarkBlocking(_saved_camera_buffer_idx)) {
        _resetTracking();
        return false;
    }

    SCHEDULER_Tick_get(&now);
    _landmark_duration_ms = now - _landmark_start_tick;

    if (!_isLandmarkValid(&landmark_output)) {
        _clearPredictedOverlay();

        if (!from_palm) {
            _handleInvalidLandmark();
        }

        return false;
    }

    landmark_lost_count = 0U;

    HandROI_TypeDef current_roi = landmark_roi;
    last_landmark_rotation = current_roi.rotation;

    if (LANDMARK_MapToFrame(&landmark_output, &current_roi, LTDC_Layer1Config.width,
                            LTDC_Layer1Config.height, landmark_points) != AI_STATUS_OK) {
        DEBUG_PRINTF("Landmark postprocessing failed!\r\n");
        return false;
    }

    HandROI_TypeDef next_roi;

    if (LANDMARK_UpdateROIFromNetworkOutput(&landmark_output, &current_roi,  &next_roi) != AI_STATUS_OK) {
        _resetTracking();
        return false;
    }

    memcpy(last_current_landmark_points, landmark_points, sizeof(last_current_landmark_points));

    SCHEDULER_Tick_get(&last_valid_landmark_output_tick);

    landmark_roi = current_roi;

    _predictLandmarksTimed(landmark_points, predicted_landmark_points);
    _aiDrawTrackingOverlay(ui);

    landmark_roi = next_roi;

    memcpy(last_current_landmark_points, landmark_points, sizeof(last_current_landmark_points));

    SCHEDULER_Tick_get(&last_valid_landmark_output_tick);

    _predictLandmarksTimed(landmark_points, predicted_landmark_points);

    _aiDrawTrackingOverlay(ui);

    if (from_palm) {
        previous_landmarks_valid = 0U;
        hand_state = HAND_STATE_LANDMARK_TRACKING;
        last_landmark_tick = now;

        DEBUG_PRINTF("AI state: LANDMARK_TRACKING\r\n");
        return true;
    }

    if ((ui != NULL) && (ui->ai_mode >= 2U)) {
        _aiRunFingeralphabetIfDue(ui);
    }

    return true;
}

static void _aiStatePalmSearch(const AIPipelineUi_TypeDef *ui)
{
    uint32_t now;

    if (ui == NULL) {
        return;
    }

    if (nn_frame_ready == 0U) {
        return;
    }

    /*
     * if ((now - last_palm_search_tick) < PALM_SEARCH_INTERVAL_MS) {
     *     return;
     * }
     */

    SCHEDULER_Tick_get(&now);

    last_palm_search_tick = now;
    nn_frame_ready = 0U;

    _saved_camera_buffer_idx = (uint8_t)ltdc_layer_bg_buffer_ai_idx;

    const uint8_t completed_idx = nn_completed_buffer_idx;

    if (!_aiRunPalmDetection(completed_idx)) {
        return;
    }

    if (ui->ai_mode == 0U) {
        if (ui->palm_vis) {
            _aiDrawPalmOnly();
        }

        return;
    }

    (void)_aiRunLandmarkPass(true, ui);
}

static void _aiStateLandmarkTracking(const AIPipelineUi_TypeDef *ui)
{
    uint32_t now;

    if (ui == NULL) {
        return;
    }

    if (ui->ai_mode < 1U) {
        _resetTracking();
        return;
    }

    if (nn_frame_ready == 0U) {
        return;
    }

    SCHEDULER_Tick_get(&now);

    last_landmark_tick = now;
    nn_frame_ready = 0U;

    _saved_camera_buffer_idx = (uint8_t)ltdc_layer_bg_buffer_ai_idx;

    (void)_aiRunLandmarkPass(false, ui);
}

void vAIPipelineTask(void)
{
    AIPipelineUi_TypeDef ui;
    uint32_t now;

    _aiUpdateUiContext(&ui);

    if (nn_frame_ready) {
        SCHEDULER_Tick_get(&_pipeline_start_tick);
    }

    switch (hand_state) {
        case HAND_STATE_PALM_SEARCH:
            _aiStatePalmSearch(&ui);
            break;

        case HAND_STATE_LANDMARK_TRACKING:
            _aiStateLandmarkTracking(&ui);
            break;

        default:
            _resetTracking();
            break;
    }

    if (_pipeline_start_tick) {
        SCHEDULER_Tick_get(&now);
        _pipeline_duration_ms = now - _pipeline_start_tick;
        _pipeline_start_tick = 0U;
    }

    if (hand_state == HAND_STATE_PALM_SEARCH) {
        /* No valid hand: reset hand & sign timings */
        _landmark_duration_ms = 0U;
        _fingeralphabet_duration_ms = 0U;
    }

    if (ui.ai_mode >= 2U) {
        isr_sign_vis = 1U;
        if (hand_state == HAND_STATE_PALM_SEARCH) {
            snprintf(_sign_str, sizeof(_sign_str), "Searching Hand");
        } else {
            snprintf(_sign_str, sizeof(_sign_str), "%s", fingeralphabet_result.label);
        }
    } else {
        isr_sign_vis = 0U;
    }
}

static void _printStackUsage(void)
{
    static uint32_t last_print = 0;
    uint32_t now;
    SCHEDULER_Tick_get(&now);
    if (now - last_print < 3000) return;
    last_print = now;

    DEBUG_PRINTF("\r\n--- Stack Usage (words) ---\r\n");
    for (int i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        const char *name;
        if (SCHEDULER_GetTaskName((uint8_t)i, &name) != SCHEDULER_OK) name = "?";
        uint32_t used = SCHEDULER_GetTaskStackUsed((uint8_t)i);
        uint32_t size = SCHEDULER_GetTaskStackSize((uint8_t)i);
        DEBUG_PRINTF("  [%d] %-16s %4u / %u\r\n",
            i, name, used, size);
    }
    DEBUG_PRINTF("--------------------------\r\n");
}

void vSystemInfoTask(void) {
    snprintf(_sysinfo_p, sizeof(_sysinfo_p), "P:%lums", (unsigned long)_palm_duration_ms);
    snprintf(_sysinfo_h, sizeof(_sysinfo_h), "H:%lums", (unsigned long)_landmark_duration_ms);
    snprintf(_sysinfo_s, sizeof(_sysinfo_s), "S:%lums", (unsigned long)_fingeralphabet_duration_ms);
    snprintf(_sysinfo_t, sizeof(_sysinfo_t), "T:%lums", (unsigned long)_pipeline_duration_ms);

    static uint32_t last_print = 0U;
    uint32_t now;
    SCHEDULER_Tick_get(&now);
    if (now - last_print >= 3000U) {
        last_print = now;
        uint32_t total = _pipeline_duration_ms;
        uint32_t palm = _palm_duration_ms;
        uint32_t landmark = _landmark_duration_ms;
        uint32_t finger = _fingeralphabet_duration_ms;
        uint32_t overhead = (total > palm + landmark + finger)
                          ? total - (palm + landmark + finger) : 0U;
        DEBUG_PRINTF("\r\n--- AI Pipeline Timings ---\r\n"
                     "  Palm         : %lums\r\n"
                     "  Landmark     : %lums\r\n"
                     "  Fingeralphabet: %lums\r\n"
                     "  Overhead     : %lums\r\n"
                     "  Total        : %lums\r\n"
                     "---------------------------\r\n",
                     (unsigned long)palm, (unsigned long)landmark,
                     (unsigned long)finger, (unsigned long)overhead,
                     (unsigned long)total);
    }

    isr_sysinfo_vis = 1U;
    _printStackUsage();
}
