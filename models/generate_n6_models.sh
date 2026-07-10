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
echo "=== Copying files to embedded project ==="
COPY_SCRIPT="$SCRIPT_DIR/copy_n6_models.sh"
if [ -f "$COPY_SCRIPT" ]; then
    bash "$COPY_SCRIPT" --stai --weights
    echo ""
else
    echo "WARNING: $COPY_SCRIPT not found — skipping copy step."
    echo "Run manually:  ./models/copy_n6_models.sh --stai --weights"
fi

FW_DIR="$SCRIPT_DIR/../embedded/STM32N6570DK/FSBL"
ASSETS_DIR="$FW_DIR/Assets/AI"

if command -v arm-none-eabi-gcc &> /dev/null; then
    echo "=== Rebuilding firmware (regenerates ecblobs.bin) ==="
    make -C "$FW_DIR/Debug" -j"$(nproc)" 2>&1 | tail -5
    echo ""

    if [ -f "$FW_DIR/Debug/neural_art_ecblobs.bin" ]; then
        cp "$FW_DIR/Debug/neural_art_ecblobs.bin" "$ASSETS_DIR/ecblobs.bin"
        echo "  copied: ecblobs.bin ($(stat -c%s "$ASSETS_DIR/ecblobs.bin") bytes)"
    fi

    echo "=== Flashing all binaries to board ==="
    if [ -f "$FW_DIR/flash_models.sh" ]; then
        bash "$FW_DIR/flash_models.sh"
    else
        echo "WARNING: flash_models.sh not found — flash manually."
    fi
else
    echo "=== Build toolchain not found ==="
    echo "To complete the update:"
    echo "  1. Rebuild firmware in STM32CubeIDE"
    echo "  2. Run:  cd $FW_DIR && ./flash_models.sh"
fi

echo ""
echo "=== Done ==="
