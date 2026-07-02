/**
 * @file simple_ltdc_layer.h
 * @author Groß
 * @date 02.07.2026
 * @brief Contains logic for LTDC layers
 *
 * Usage
 * -----
 * 1. LTDC_ConfigLayer
 */

// ==========================================================
// Defines
// ==========================================================

#define LTDC_LAYER_BG_WIDTH  800
#define LTDC_LAYER_BG_HEIGHT 480

#define LTDC_LAYER_FG_WIDTH  800
#define LTDC_LAYER_FG_HEIGHT 480

#define LTDC_LAYER_DISPLAY_BUFFER_NB    2
#define LTDC_LAYER_DISPLAY_BPP          3

#define LTDC_LAYER_NN_BUFFER_NB    2
#define LTDC_LAYER_NN_BPP          2

#define LTDC_LAYER_NN_RAW_WIDTH   192
#define LTDC_LAYER_NN_RAW_HEIGHT  192
#define LTDC_LAYER_NN_RAW_BPP       3
#define LTDC_LAYER_NN_RAW_SIZE    (LTDC_LAYER_NN_RAW_WIDTH * LTDC_LAYER_NN_RAW_HEIGHT * LTDC_LAYER_NN_RAW_BPP)

#define LTDC_LAYER_FPF_ARGB4444_INIT { .bytes_per_pixel = 2, .alpha_len = 4, .alpha_pos = 12, .red_len = 4, .red_pos = 8, .green_len = 4, .green_pos = 4, .blue_len = 4, .blue_pos = 0 }
#define LTDC_LAYER_FPF_ARGB1555_INIT { .bytes_per_pixel = 2, .alpha_len = 1, .alpha_pos = 15, .red_len = 5, .red_pos = 10, .green_len = 5, .green_pos = 5, .blue_len = 5, .blue_pos = 0 }

// ==========================================================
// Typedefs
// ==========================================================

typedef enum {
    LTDC_PF_ARGB8888 = 0b000,
    LTDC_PF_ABGR8888 = 0b001,
    LTDC_PF_RGBA8888 = 0b010,
    LTDC_PF_BGRA8888 = 0b011,
    LTDC_PF_RGB565   = 0b100,
    LTDC_PF_BGR565   = 0b101,
    LTDC_PF_RGB888   = 0b110,
    LTDC_PF_Flexible = 0b111,
} LTDC_Layer_PixelFormat_TypeDef;

/*
 * Flexible pixel format descriptor for custom layer color types.
 * Configures the LTDC FPF0R / FPF1R registers when PF = 0b111.
 */
typedef struct {
    uint8_t bytes_per_pixel;
    uint8_t alpha_len;
    uint8_t alpha_pos;
    uint8_t red_len;
    uint8_t red_pos;
    uint8_t green_len;
    uint8_t green_pos;
    uint8_t blue_len;
    uint8_t blue_pos;
} LTDC_Layer_FlexiblePixelFormat_TypeDef;

typedef struct LTDC_LayerConfig {
    LTDC_Layer_TypeDef                              *regs;
    volatile void                                   *fb;
    uint16_t                                         x;
    uint16_t                                         y;
    uint16_t                                         width;
    uint16_t                                         height;
    uint16_t                                         buf_width;
    LTDC_Layer_PixelFormat_TypeDef                         pixel_format;
    const LTDC_Layer_FlexiblePixelFormat_TypeDef    *flexible_fmt;
    uint8_t                                          const_alpha;
    uint8_t                                          per_pixel_alpha;
    uint32_t                                         default_color;
    uint8_t                                          blending_order;
} LTDC_Layer_Config_TypeDef;

// ==========================================================
// API
// ==========================================================

/**
 * @brief Configure an LTDC layer from a configuration struct
 *
 * @param [in] cfg | Layer configuration
 */
void LTDC_Layer_Config(const LTDC_Layer_Config_TypeDef *cfg);

