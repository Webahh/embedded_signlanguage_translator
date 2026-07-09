#!/bin/bash
# SPDX-License-Identifier: MIT
#
# Copy generated NPU model files from models/generated/ into the
# embedded STM32N6 project tree.
#
# Usage:
#   ./copy_n6_models.sh                   # copy core files
#   ./copy_n6_models.sh --stai            # also copy stai_* wrappers
#   ./copy_n6_models.sh --weights         # also copy model weight .bin files
#   ./copy_n6_models.sh --stai --weights  # all files
#   ./copy_n6_models.sh --dry-run         # preview only
#   ./copy_n6_models.sh --help            # show full help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
AI_DIR="$PROJECT_ROOT/embedded/STM32N6570DK/FSBL/Src/AI"
ASSETS_DIR="$PROJECT_ROOT/embedded/STM32N6570DK/FSBL/Assets/AI"
ST_AI_OUTPUT="$SCRIPT_DIR/st_ai_output"

COPY_STAI=false
COPY_WEIGHTS=false
DRY_RUN=false

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Copy generated model files into the embedded project tree.

Options:
  --dry-run    Print what would be copied without copying.
  --stai       Also copy the stai_* wrapper files.
  --weights    Also copy model weight .bin files to Assets/AI/.
  --help       Show this help and exit.
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) DRY_RUN=true; shift ;;
        --stai)    COPY_STAI=true; shift ;;
        --weights) COPY_WEIGHTS=true; shift ;;
        --help)    usage ;;
        *)         echo "Unknown option: $1"; usage ;;
    esac
done

# ---------------------------------------------------------------------------
# Model mapping: generated_dir -> AI_subdir
# ---------------------------------------------------------------------------
declare -A MODELS=(
    [palm]=Palm_detection
    [landmark]=Hand_landmark
    [finger]=Fingeralphabet
)

do_copy() {
    local src="$1"
    local dst="$2"
    mkdir -p "$(dirname "$dst")"
    if $DRY_RUN; then
        echo "  cp $src"
        echo "      -> $dst"
    else
        cp "$src" "$dst"
        echo "  copied: $(basename "$src")"
    fi
}

# ---------------------------------------------------------------------------
# Table header
# ---------------------------------------------------------------------------
echo "=== Copy NPU Models to Embedded Project ==="
$DRY_RUN && echo "  [DRY RUN - no files will be written]"
echo "  Source: $SCRIPT_DIR/generated/"
echo "  Dest:   $AI_DIR/"
echo ""

total=0

for gen_dir in "${!MODELS[@]}"; do
    ai_subdir="${MODELS[$gen_dir]}"
    src_dir="$SCRIPT_DIR/generated/$gen_dir"
    base_inc_dir="$AI_DIR/$ai_subdir/Inc"
    base_src_dir="$AI_DIR/$ai_subdir/Src"

    # Determine file prefix based on model directory
    case "$gen_dir" in
        palm)   prefix="palm_detection_model_v3" ;;
        landmark) prefix="hand_landmark_model_v3" ;;
        finger) prefix="fingeralphabet_model_v3" ;;
    esac

    # Core files (always copied)
    echo "[$ai_subdir]"
    do_copy "$src_dir/${prefix}.c"          "$base_src_dir/${prefix}.c"
    do_copy "$src_dir/${prefix}_ecblobs.h"  "$base_inc_dir/${prefix}_ecblobs.h"
    total=$((total + 2))

    # Patch: inject ecblob_sections.h include so const arrays land in ECBLOBS
    # region (0x72000000, 8 MB) instead of overflowing the 255 KB ROM.
    patched="$base_inc_dir/${prefix}_ecblobs.h"
    if ! $DRY_RUN && [ -f "$patched" ]; then
        if ! grep -q 'ecblob_sections.h' "$patched"; then
            sed -i '/^#include <string.h>/a #include "ecblob_sections.h"' "$patched"
            echo "  patched: $(basename "$patched")"
        fi
    fi

    # Optional stai wrapper files
    if $COPY_STAI; then
        do_copy "$src_dir/stai_${prefix}.c" "$base_src_dir/stai_${prefix}.c"
        do_copy "$src_dir/stai_${prefix}.h" "$base_inc_dir/stai_${prefix}.h"
        total=$((total + 2))
    fi
    echo ""
done

# ---------------------------------------------------------------------------
# Weight .bin files (optional)
# ---------------------------------------------------------------------------
if $COPY_WEIGHTS; then
    echo "=== Copying weight .bin files to Assets/AI/ ==="
    $DRY_RUN && echo "  [DRY RUN]"

    declare -A WEIGHTS=(
        [palm_detection_model_v3_atonbuf.xSPI2.raw]=palm_detection_model_v3_atonbuf.xSPI2.bin
        [hand_landmark_model_v3_atonbuf.xSPI2.raw]=hand_landmark_model_v3_atonbuf.xSPI2.bin
        [fingeralphabet_model_v3_atonbuf.xSPI2.raw]=fingeralphabet_model_v3_atonbuf.xSPI2.bin
    )

    for raw_name in "${!WEIGHTS[@]}"; do
        bin_name="${WEIGHTS[$raw_name]}"
        src="$ST_AI_OUTPUT/$raw_name"
        dst="$ASSETS_DIR/$bin_name"

        if [ ! -f "$src" ]; then
            echo "  SKIPPED: $raw_name not found (run generate_n6_models.sh first)"
            continue
        fi

        mkdir -p "$ASSETS_DIR"
        if $DRY_RUN; then
            echo "  cp $src"
            echo "      -> $dst"
        else
            cp "$src" "$dst"
            echo "  copied: $bin_name"
        fi
        total=$((total + 1))
    done
    echo ""
fi

echo "=== Done ($total files copied) ==="
