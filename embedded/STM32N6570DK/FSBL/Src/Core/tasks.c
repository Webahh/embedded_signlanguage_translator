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
#include "hand_landmark_preprocessing.h"
#include "hand_landmark_postprocessing.h"
#include "fingeralphabet_preprocessing.h"
#include "simple_text.h"
#include "simple_dma2d.h"
#include "ui.h"

static volatile int ltdc_fg_disp_idx = 1;
static volatile uint8_t nn_frame_ready = 0U;
static volatile uint8_t nn_completed_buffer_idx = 0U;
static volatile uint8_t camera_frame_ready = 0U;

void DCMIPP_PIPE_FrameEventCallback(uint32_t pipe)
{
    if (pipe == DCMIPP_PIPE1) {
        AE_OnFrameStats();

        int next_disp_idx = (ltdc_layer_bg_buffer_disp_idx + 1) % LTDC_LAYER_DISPLAY_BUFFER_NB;
        int next_capt_idx = (ltdc_layer_bg_buffer_capt_idx + 1) % LTDC_LAYER_DISPLAY_BUFFER_NB;

        DCMIPP_Pipe_UpdateBufAddr(
            CAM_PIPE_DISPLAY,
            (uint32_t)&ltdc_layer_bg_buffer[next_capt_idx]
        );

        LTDC_Layer1Config.fb = (volatile uint8_t *)&ltdc_layer_bg_buffer[next_disp_idx];
        LTDC_Layer_Address_Set(&LTDC_Layer1Config);

        int ai_idx =
            (next_disp_idx + LTDC_LAYER_AI_LOOKAHEAD_FRAMES) %
            LTDC_LAYER_DISPLAY_BUFFER_NB;

        ltdc_layer_bg_buffer_ai_idx   = ai_idx;
        ltdc_layer_bg_buffer_disp_idx = next_disp_idx;
        ltdc_layer_bg_buffer_capt_idx = next_capt_idx;

        camera_frame_ready = 1U;

    } else if (pipe == DCMIPP_PIPE2) {
        nn_completed_buffer_idx = (DCMIPP->P2SR & DCMIPP_P2SR_LSTFRM) ? 1U : 0U;
        nn_frame_ready = 1U;
    }
}


static const uint8_t bg_colors[BG_NUM_COLORS][3] = {
    {255, 0, 0},    /* Red   */
    {0, 255, 0},    /* Green */
    {0, 0, 255}     /* Blue  */
};

static uint8_t  bg_seg_idx    = 0;
static uint32_t bg_blend_start = 0;

__attribute__((noinline, optimize("O0"))) // No optimizations for better testing
static int recursion(int n)
{
    volatile uint32_t marker = 0xDEADBEEF;
    volatile uint32_t padding[8];

    padding[0] = marker;

    if (n == 0)
        return padding[0];

    return recursion(n - 1) + 1;
}

void vRecursionTestTask(void) {
	recursion(20);
}

void vSystemTimeTask(void) {
    uint32_t now;
    SCHEDULER_Tick_get(&now); // MAX:     4294967296
	char str[11]; // + '\0'
	snprintf(str, sizeof(str), "%lu", (unsigned long)now);
	TEXT_StringBg_draw(&LTDC_Layer1Config, str, 720, 0, LTDC_LAYER_COLOR_WHITE, 0x00000000U);
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
    if (now - last_ms < 30) return;

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

        // Paint touch feedbac
        if (data.pressed)
        {
        	int next_idx = ltdc_layer_bg_buffer_disp_idx;
            LTDC_Layer_Config_TypeDef tmp = LTDC_Layer1Config;
            tmp.fb = (void *)&ltdc_layer_bg_buffer[next_idx];
            LTDC_Layer_Draw_Circle(&tmp, data.x, data.y, 5, LTDC_LAYER_COLOR_BLUE);
        }
    }
}


static PalmNetworkOutput_TypeDef palm_output;
static PalmDetection_TypeDef palm_detection;
static PalmDetectionFilter_TypeDef palm_filter;
static HandROI_TypeDef landmark_roi;

static LandmarkNetworkOutput_TypeDef landmark_output;
static uint8_t landmark_preprocessed_input[LANDMARK_INPUT_SIZE] __attribute__((aligned(32)));
static LandmarkPoint_TypeDef landmark_points[LANDMARK_POINT_COUNT];

static uint8_t fingeralphabet_input[FINGERALPHABET_INPUT_SIZE];
static uint8_t fingeralphabet_output[FINGERALPHABET_OUTPUT_SIZE];
static FingeralphabetResult_TypeDef fingeralphabet_result;

static DMA2D_Handle_TypeDef _dma2d;
static int _dma2d_initialized = 0;

static void DMA2D_EnsureInit(void){
    if (!_dma2d_initialized) {
        DMA2D_Init(&_dma2d);
        _dma2d_initialized = 1;
    }
}

typedef enum {
    HAND_STATE_PALM_SEARCH = 0,
    HAND_STATE_LANDMARK_TRACKING
} HandTrackingState_TypeDef;

typedef enum {
    AI_STAGE_IDLE = 0,
    AI_STAGE_WAIT_PALM,
    AI_STAGE_WAIT_LANDMARK_FROM_PALM,
    AI_STAGE_WAIT_LANDMARK_TRACKING,
    AI_STAGE_WAIT_FINGERALPHABET
} AIPipelineStage_TypeDef;

#define LANDMARK_PRESENCE_THRESHOLD  0.5f
#define LANDMARK_LOST_FRAME_COUNT    3U
#define PALM_SEARCH_INTERVAL_MS      200U
#define FINGERALPHABET_INTERVAL_MS   1500U
#define LANDMARK_TRACK_INTERVAL_MS   0U
#define LANDMARK_OVERLAY_TIMEOUT_MS  300U

static uint32_t last_valid_landmark_output_tick = 0U;

static HandTrackingState_TypeDef hand_state = HAND_STATE_PALM_SEARCH;
static AIPipelineStage_TypeDef ai_stage = AI_STAGE_IDLE;

static uint8_t landmark_lost_count = 0U;
static uint32_t last_palm_search_tick = 0U;
static uint32_t last_landmark_tick = 0U;
static uint32_t last_fingeralphabet_tick = 0U;

static uint8_t _saved_camera_buffer_idx;

static uint32_t _palm_duration_ms;
static uint32_t _landmark_duration_ms;
static uint32_t _fingeralphabet_duration_ms;
static uint32_t _palm_start_tick;
static uint32_t _landmark_start_tick;
static uint32_t _fingeralphabet_start_tick;

#define LANDMARK_PREDICTION_TIME_MS     45.0f
#define LANDMARK_PREDICTION_MAX_DELTA   0.08f
#define LANDMARK_SMOOTHING_ALPHA        0.75f

static LandmarkPoint_TypeDef previous_landmark_points[LANDMARK_POINT_COUNT];
static LandmarkPoint_TypeDef smoothed_landmark_points[LANDMARK_POINT_COUNT];
static LandmarkPoint_TypeDef predicted_landmark_points[LANDMARK_POINT_COUNT];

static uint8_t previous_landmarks_valid = 0U;
static uint32_t previous_landmark_tick = 0U;

static uint8_t predicted_overlay_valid = 0U;
static LandmarkPoint_TypeDef last_current_landmark_points[LANDMARK_POINT_COUNT];

#define LANDMARK_OVERLAY_REDRAW_INTERVAL_MS  30U
static uint32_t last_overlay_redraw_tick = 0U;

static float _clampf(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void _predictLandmarksTimed(
    const LandmarkPoint_TypeDef current[LANDMARK_POINT_COUNT],
    LandmarkPoint_TypeDef predicted[LANDMARK_POINT_COUNT]
)
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

        smoothed_landmark_points[i].x =
            LANDMARK_SMOOTHING_ALPHA * px +
            (1.0f - LANDMARK_SMOOTHING_ALPHA) * smoothed_landmark_points[i].x;

        smoothed_landmark_points[i].y =
            LANDMARK_SMOOTHING_ALPHA * py +
            (1.0f - LANDMARK_SMOOTHING_ALPHA) * smoothed_landmark_points[i].y;

        smoothed_landmark_points[i].z = current[i].z;

        predicted[i] = smoothed_landmark_points[i];
    }

    memcpy(previous_landmark_points, current, sizeof(previous_landmark_points));
}

static void _clearPredictedOverlay(void)
{
    predicted_overlay_valid = 0U;
    previous_landmarks_valid = 0U;
    last_overlay_redraw_tick = 0U;

    LTDC_Layer_Draw_LandmarksClearPrevious();
}

static void _redrawPredictedOverlayFromLastMeasurement(void)
{
    uint32_t now;
    SCHEDULER_Tick_get(&now);

    if (!predicted_overlay_valid) {
        return;
    }

    if ((now - last_valid_landmark_output_tick) > LANDMARK_OVERLAY_TIMEOUT_MS) {
        _clearPredictedOverlay();
        return;
    }

    if ((now - last_overlay_redraw_tick) < LANDMARK_OVERLAY_REDRAW_INTERVAL_MS) {
        return;
    }

    last_overlay_redraw_tick = now;

    _predictLandmarksTimed(last_current_landmark_points, predicted_landmark_points);

    LTDC_Layer_Draw_LandmarksClearPrevious();
    LTDC_Layer_Draw_Landmarks(predicted_landmark_points);
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
    ai_stage = AI_STAGE_IDLE;

    landmark_lost_count = 0U;

    _clearPredictedOverlay();
}

static bool _startLandmark(uint8_t camera_buffer_idx, AIPipelineStage_TypeDef next_stage)
{
    const bool preprocessing_ok = LANDMARK_PreprocessROI((const uint8_t *)ltdc_layer_bg_buffer[camera_buffer_idx],
    													 LTDC_Layer1Config.width,
														 LTDC_Layer1Config.height,
														 LTDC_Layer1Config.buf_width * LANDMARK_INPUT_CHANNELS,
														 &landmark_roi,
														 landmark_preprocessed_input);

    if (!preprocessing_ok) {
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

    if (DMA2D_Transfer(&_dma2d) != DMA2D_OK) {
        return false;
    }

    if (!LANDMARK_Start(NULL)) {
        return false;
    }

    SCHEDULER_Tick_get(&_landmark_start_tick);
    ai_stage = next_stage;

    return true;
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

static void _runFingeralphabetIfDue(void)
{
    uint32_t now;
    SCHEDULER_Tick_get(&now);

    if ((now - last_fingeralphabet_tick) < FINGERALPHABET_INTERVAL_MS) {
        return;
    }

    last_fingeralphabet_tick = now;

    if (!FINGERALPHABET_Preprocess(landmark_points, landmark_output.handedness, fingeralphabet_input)) {
        DEBUG_PRINTF("Fingeralphabet preprocessing failed\r\n");
        return;
    }

    if (!FINGERALPHABET_Start(fingeralphabet_input)) {
        DEBUG_PRINTF("Fingeralphabet start failed\r\n");
        return;
    }

    SCHEDULER_Tick_get(&_fingeralphabet_start_tick);
    ai_stage = AI_STAGE_WAIT_FINGERALPHABET;
}

void vAIPipelineTask(void)
{
    uint32_t now;
    SCHEDULER_Tick_get(&now);

    _redrawPredictedOverlayFromLastMeasurement();

    if (ai_stage == AI_STAGE_WAIT_PALM) {
        AI_RunStepStatus_TypeDef palm_status = PALM_RunStep(&palm_output);

        if (palm_status == AI_RUN_BUSY) {
            return;
        }

        ai_stage = AI_STAGE_IDLE;

        if (palm_status == AI_RUN_ERROR) {
            return;
        }

        {
            uint32_t now;
            SCHEDULER_Tick_get(&now);
            _palm_duration_ms = now - _palm_start_tick;
        }

        const bool palm_valid = PALM_Postprocess(&palm_output, &palm_detection);
        PALM_UpdateDetectionFilter(&palm_filter, palm_valid);

        if (!palm_valid) {
            if (!palm_filter.detected) {
            	_clearPredictedOverlay();
            }
            return;
        }

        if (!palm_filter.detected) {
        	_clearPredictedOverlay();
            return;
        }

        if (!PALM_CreateLandmarkROI(&palm_detection, LTDC_Layer1Config.width,
                                    LTDC_Layer1Config.height, &landmark_roi)) {
            _resetTracking();
            return;
        }

        if (!_startLandmark(_saved_camera_buffer_idx, AI_STAGE_WAIT_LANDMARK_FROM_PALM)) {
            _resetTracking();
        }
        return;
    }

    if ((ai_stage == AI_STAGE_WAIT_LANDMARK_FROM_PALM) ||
        (ai_stage == AI_STAGE_WAIT_LANDMARK_TRACKING)) {

        const AIPipelineStage_TypeDef finished_stage = ai_stage;
        LandmarkRunStatus_TypeDef landmark_status = LANDMARK_RunStep(&landmark_output);

        if (landmark_status == LANDMARK_RUN_BUSY) {
            return;
        }

        ai_stage = AI_STAGE_IDLE;

        if (landmark_status == LANDMARK_RUN_ERROR) {
            _resetTracking();
            return;
        }

        {
            uint32_t now;
            SCHEDULER_Tick_get(&now);
            _landmark_duration_ms = now - _landmark_start_tick;
        }

        if (!_isLandmarkValid(&landmark_output)) {
        	_clearPredictedOverlay();
            if (finished_stage == AI_STAGE_WAIT_LANDMARK_TRACKING) {
                _handleInvalidLandmark();
            }

            return;
        }

        landmark_lost_count = 0U;
        LANDMARK_MapToFrame(&landmark_output, &landmark_roi, landmark_points);

        memcpy(last_current_landmark_points, landmark_points, sizeof(last_current_landmark_points));
        predicted_overlay_valid = 1U;
        SCHEDULER_Tick_get(&last_valid_landmark_output_tick);

        _predictLandmarksTimed(landmark_points, predicted_landmark_points);

        LTDC_Layer_Draw_LandmarksClearPrevious();
        LTDC_Layer_Draw_Landmarks(predicted_landmark_points);

        if (!LANDMARK_UpdateROI(landmark_points, LTDC_Layer1Config.width,
        						LTDC_Layer1Config.height, &landmark_roi)) {
        	_resetTracking();
            return;
        }

        LTDC_Layer_Draw_ROIClearPrevious();
        //LTDC_Layer_Draw_ROILandmark(&landmark_roi, LTDC_LAYER_COLOR_GREEN);

        if (finished_stage == AI_STAGE_WAIT_LANDMARK_FROM_PALM) {
        	previous_landmarks_valid = 0U;
            hand_state = HAND_STATE_LANDMARK_TRACKING;
            last_landmark_tick = now;

            DEBUG_PRINTF("AI state: LANDMARK_TRACKING\r\n");
            return;
        }

        _runFingeralphabetIfDue();

        return;
    }

    if (ai_stage == AI_STAGE_WAIT_FINGERALPHABET) {
        AI_RunStepStatus_TypeDef fa_status = FINGERALPHABET_RunStep(fingeralphabet_output);

        if (fa_status == AI_RUN_BUSY) {
            return;
        }

        ai_stage = AI_STAGE_IDLE;

        if (fa_status == AI_RUN_ERROR) {
            DEBUG_PRINTF("Fingeralphabet inference failed\r\n");
            return;
        }

        {
            uint32_t now;
            SCHEDULER_Tick_get(&now);
            _fingeralphabet_duration_ms = now - _fingeralphabet_start_tick;
        }

        fingeralphabet_result = FINGERALPHABET_GetResult(fingeralphabet_output);
        DEBUG_PRINTF("Index: %d Result: %s\r\n", fingeralphabet_result.class_index, fingeralphabet_result.label);
        return;
    }

    if (hand_state == HAND_STATE_PALM_SEARCH) {
        if (nn_frame_ready == 0U) {
            return;
        }

        if ((now - last_palm_search_tick) < PALM_SEARCH_INTERVAL_MS) {
            return;
        }

        last_palm_search_tick = now;
        nn_frame_ready = 0U;
    }
    else {
        if (camera_frame_ready == 0U) {
            return;
        }

        if ((now - last_landmark_tick) < LANDMARK_TRACK_INTERVAL_MS) {
            return;
        }

        last_landmark_tick = now;
        camera_frame_ready = 0U;
    }

    _saved_camera_buffer_idx = (uint8_t)ltdc_layer_bg_buffer_ai_idx;
    const uint8_t completed_idx = nn_completed_buffer_idx;

    switch (hand_state) {
    case HAND_STATE_PALM_SEARCH:
    {
        uint8_t *palm_input = PALM_GetInputBuffer();

        if (palm_input == NULL) {
            return;
        }

        DMA2D_EnsureInit();

        _dma2d.cfg.src.address = (uint32_t)ltdc_layer_nn_raw_buffer[completed_idx];
        _dma2d.cfg.src.line_offset = 0;
        _dma2d.cfg.src.format = DMA2D_FORMAT_RGB888;
        _dma2d.cfg.dst.address = (uint32_t)palm_input;

        _dma2d.cfg.dst.line_offset = 0;
        _dma2d.cfg.dst.format = DMA2D_FORMAT_RGB888;

        _dma2d.cfg.width_pixels = PALM_INPUT_WIDTH;
        _dma2d.cfg.height_lines = PALM_INPUT_HEIGHT;
        _dma2d.cfg.mode = DMA2D_MODE_MEM_TO_MEM;

        if (DMA2D_Transfer(&_dma2d) != DMA2D_OK) {
            return;
        }

        SCHEDULER_Tick_get(&_palm_start_tick);
        if (!PALM_Start()) {
            return;
        }

        ai_stage = AI_STAGE_WAIT_PALM;
        return;
    }

    case HAND_STATE_LANDMARK_TRACKING:
    {
        if (!_startLandmark(_saved_camera_buffer_idx, AI_STAGE_WAIT_LANDMARK_TRACKING)) {
            _handleInvalidLandmark();
            return;
        }
        return;
    }

    default:
        _resetTracking();
        return;
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

void vSystemInfoTask(void)
{
    char buf[12];

    LTDC_Layer_Draw_Rect(&LTDC_Layer1Config, 720, 16, 80, 48, 0x00000000U);

    snprintf(buf, sizeof(buf), "P:%lums", (unsigned long)_palm_duration_ms);
    TEXT_StringBg_draw(&LTDC_Layer1Config, buf, 720, 16, LTDC_LAYER_COLOR_WHITE, 0x00000000U);

    snprintf(buf, sizeof(buf), "H:%lums", (unsigned long)_landmark_duration_ms);
    TEXT_StringBg_draw(&LTDC_Layer1Config, buf, 720, 32, LTDC_LAYER_COLOR_WHITE, 0x00000000U);

    snprintf(buf, sizeof(buf), "S:%lums", (unsigned long)_fingeralphabet_duration_ms);
    TEXT_StringBg_draw(&LTDC_Layer1Config, buf, 720, 48, LTDC_LAYER_COLOR_WHITE, 0x00000000U);

    _printStackUsage();
}
