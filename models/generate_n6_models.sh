#!/bin/bash
# SPDX-License-Identifier: MIT
#
# Generate STM32N6 NPU C code from .tflite models using ST Edge AI Core.
# Uses custom mpool files that keep all activations in on-chip AXISRAM.
#
# Prerequisites:
#   - stedgeai CLI in PATH (ST Edge AI Core v3.0.0+)
#   - arm-none-eabi-objcopy for .hex conversion (optional)
#
# Structure:
#   source/*.tflite          input models
#   my_mpools/*.mpool        memory pool definitions
#   user_neuralart.json      per-model config (flags + mpool refs)
#   generated/{palm,landmark,finger}/   output C code + .hex
#   st_ai_output/            intermediate build output (gitignored)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NEURALART_CFG="$SCRIPT_DIR/user_neuralart.json"
ST_AI_OUTPUT="$SCRIPT_DIR/st_ai_output"

# Check prerequisites
if ! command -v stedgeai &> /dev/null; then
    echo "ERROR: stedgeai not found in PATH."
    echo "Set it with:  export PATH=\$HOME/st/stedgeai/3.0/Utilities/linux:\$PATH"
    exit 1
fi

echo "=== ST Edge AI Core ==="
stedgeai --version
echo ""

# Clean previous intermediate output
rm -rf "$ST_AI_OUTPUT"

# ---------------------------------------------------------------------------
# 1. Palm Detection  (192x192x3, uint8 input)
# ---------------------------------------------------------------------------
echo ">>> Generating palm_detection_model_v3 ..."
stedgeai generate \
    --no-inputs-allocation \
    --name palm_detection_model_v3 \
    --model "$SCRIPT_DIR/source/033_palm_detection_full_quant_pc_ff_od.tflite" \
    --target stm32n6 \
    --st-neural-art "palm_detection_model_v3@$NEURALART_CFG" \
    --input-data-type uint8 \
    --output "$ST_AI_OUTPUT"

mkdir -p "$SCRIPT_DIR/generated/palm"
cp "$ST_AI_OUTPUT/palm_detection_model_v3_ecblobs.h"   "$SCRIPT_DIR/generated/palm/"
cp "$ST_AI_OUTPUT/palm_detection_model_v3.c"            "$SCRIPT_DIR/generated/palm/"
cp "$ST_AI_OUTPUT/stai_palm_detection_model_v3.c"       "$SCRIPT_DIR/generated/palm/"
cp "$ST_AI_OUTPUT/stai_palm_detection_model_v3.h"       "$SCRIPT_DIR/generated/palm/"

if command -v arm-none-eabi-objcopy &> /dev/null; then
    RAW="$ST_AI_OUTPUT/palm_detection_model_v3_atonbuf.xSPI2.raw"
    if [ -f "$RAW" ]; then
        cp "$RAW" "$ST_AI_OUTPUT/palm_detection_data.xSPI2.bin"
        arm-none-eabi-objcopy -I binary "$ST_AI_OUTPUT/palm_detection_data.xSPI2.bin" \
            --change-addresses 0x71200000 -O ihex "$SCRIPT_DIR/generated/palm/palm_detection_data.hex"
    fi
fi

# ---------------------------------------------------------------------------
# 2. Hand Landmark  (224x224x3)
# ---------------------------------------------------------------------------
echo ">>> Generating hand_landmark_model_v3 ..."
stedgeai generate \
    --name hand_landmark_model_v3 \
    --model "$SCRIPT_DIR/source/033_hand_landmark_full_quant_pc_uf_handl.tflite" \
    --target stm32n6 \
    --st-neural-art "hand_landmark_model_v3@$NEURALART_CFG" \
    --output "$ST_AI_OUTPUT"

mkdir -p "$SCRIPT_DIR/generated/landmark"
cp "$ST_AI_OUTPUT/hand_landmark_model_v3_ecblobs.h"    "$SCRIPT_DIR/generated/landmark/"
cp "$ST_AI_OUTPUT/hand_landmark_model_v3.c"             "$SCRIPT_DIR/generated/landmark/"
cp "$ST_AI_OUTPUT/stai_hand_landmark_model_v3.c"        "$SCRIPT_DIR/generated/landmark/"
cp "$ST_AI_OUTPUT/stai_hand_landmark_model_v3.h"        "$SCRIPT_DIR/generated/landmark/"

if command -v arm-none-eabi-objcopy &> /dev/null; then
    RAW="$ST_AI_OUTPUT/hand_landmark_model_v3_atonbuf.xSPI2.raw"
    if [ -f "$RAW" ]; then
        cp "$RAW" "$ST_AI_OUTPUT/hand_landmark_data.xSPI2.bin"
        arm-none-eabi-objcopy -I binary "$ST_AI_OUTPUT/hand_landmark_data.xSPI2.bin" \
            --change-addresses 0x71600000 -O ihex "$SCRIPT_DIR/generated/landmark/hand_landmark_data.hex"
    fi
fi

# ---------------------------------------------------------------------------
# 3. Finger Alphabet  (custom model)
# ---------------------------------------------------------------------------
echo ">>> Generating fingeralphabet_model_v3 ..."
stedgeai generate \
    --name fingeralphabet_model_v3 \
    --model "$SCRIPT_DIR/source/fingeralphabet_model_int8.tflite" \
    --target stm32n6 \
    --st-neural-art "fingeralphabet_model_v3@$NEURALART_CFG" \
    --input-data-type uint8 \
    --output "$ST_AI_OUTPUT"

mkdir -p "$SCRIPT_DIR/generated/finger"
cp "$ST_AI_OUTPUT/fingeralphabet_model_v3_ecblobs.h"    "$SCRIPT_DIR/generated/finger/"
cp "$ST_AI_OUTPUT/fingeralphabet_model_v3.c"            "$SCRIPT_DIR/generated/finger/"
cp "$ST_AI_OUTPUT/stai_fingeralphabet_model_v3.c"       "$SCRIPT_DIR/generated/finger/"
cp "$ST_AI_OUTPUT/stai_fingeralphabet_model_v3.h"       "$SCRIPT_DIR/generated/finger/"

if command -v arm-none-eabi-objcopy &> /dev/null; then
    RAW="$ST_AI_OUTPUT/fingeralphabet_model_v3_atonbuf.xSPI2.raw"
    if [ -f "$RAW" ]; then
        cp "$RAW" "$ST_AI_OUTPUT/fingeralphabet_data.xSPI2.bin"
        arm-none-eabi-objcopy -I binary "$ST_AI_OUTPUT/fingeralphabet_data.xSPI2.bin" \
            --change-addresses 0x71000000 -O ihex "$SCRIPT_DIR/generated/finger/fingeralphabet_data.hex"
    fi
fi

echo ""
echo "=== Done ==="
echo "Output: $SCRIPT_DIR/generated/{palm,landmark,finger}/"
