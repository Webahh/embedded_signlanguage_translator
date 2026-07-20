#!/bin/bash
# Flash FSBL binary and inference model binaries to STM32N6570-DK
#
# Standalone memory map (from STM32N657X0HXQ_AXISRAM2_fsbl.ld):
#   Internal flash:  0x34000400  (FSBL binary — vector table + code)
#   External xSPI2 NOR (OctoSPI):
#     fingeralphabet   0x71000000
#     palm_detection   0x71200000
#     hand_landmark    0x71600000
#     ecblobs          0x72000000
#
# Usage:
#   ./flash_models.sh                        # flash models only (default)
#   ./flash_models.sh --with-fsbl            # flash FSBL + models
#   ./flash_models.sh --only-fsbl            # flash FSBL only
#   ./flash_models.sh --programmer <path>    # custom STM32_Programmer_CLI
#   ./flash_models.sh --loader <path>        # custom external loader .stldr

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODEL_DIR="$SCRIPT_DIR/Assets/AI"

# FSBL build output directories (Debug checked first)
FSBL_HEX=""
for d in "$SCRIPT_DIR/Debug" "$SCRIPT_DIR/Release"; do
  for f in "$d"/Sign_Language_Translator.hex; do
    [ -f "$f" ] && FSBL_HEX="$f" && break 2
  done
done

FLASH_ORDER=(
  "fingeralphabet_model_v3_atonbuf.xSPI2.bin:0x71000000:Finger Alphabet"
  "palm_detection_model_v3_atonbuf.xSPI2.bin:0x71200000:Palm Detection"
  "hand_landmark_model_v3_atonbuf.xSPI2.bin:0x71600000:Hand Landmark"
  "ecblobs.bin:0x72000000:EC Blobs"
)

# --- parse args ---
CUSTOM_PROGRAMMER=""
CUSTOM_LOADER=""
MODE="models"            # models | fsbl | all
while [ $# -gt 0 ]; do
  case "$1" in
    --with-fsbl)   MODE="all";    shift ;;
    --only-fsbl)   MODE="fsbl";   shift ;;
    --programmer)  CUSTOM_PROGRAMMER="$2"; shift 2 ;;
    --loader)      CUSTOM_LOADER="$2";     shift 2 ;;
    *) echo "Unknown: $1"; exit 1 ;;
  esac
done

# --- find STM32_Programmer_CLI ---
PROGRAMMER=""
if [ -n "$CUSTOM_PROGRAMMER" ]; then
  PROGRAMMER="$CUSTOM_PROGRAMMER"
elif [ -n "${STM32_PROGRAMMER_PATH:-}" ]; then
  PROGRAMMER="$STM32_PROGRAMMER_PATH"
else
  for c in \
    "STM32_Programmer_CLI" \
    "/home/${USER}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI" \
    "/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI" \
    "/opt/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI" \
    "/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe" \
    "/c/Program Files (x86)/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
  do
    if command -v "$c" &>/dev/null; then
      PROGRAMMER="$c"
      break
    fi
  done
fi

if [ -z "$PROGRAMMER" ]; then
  echo "STM32_Programmer_CLI not found."
  echo "Set path: export STM32_PROGRAMMER_PATH=/path/to/STM32_Programmer_CLI"
  echo "Or pass: ./flash_models.sh --programmer /path/to/STM32_Programmer_CLI"
  exit 1
fi

# --- find external OctoSPI flash loader ---
LOADER=""
if [ -n "$CUSTOM_LOADER" ]; then
  LOADER="$CUSTOM_LOADER"
else
  for l in \
    "/home/${USER}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr" \
    "/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr" \
    "/opt/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr"
  do
    if [ -f "$l" ]; then
      LOADER="$l"
      break
    fi
  done
fi

LOADER_OPT=""
[ -n "$LOADER" ] && LOADER_OPT="-el \"$LOADER\""

# --- summary ---
echo "Programmer: $PROGRAMMER"
echo "Loader:     ${LOADER:-none}"
echo "Mode:       $MODE"
echo ""

FAILED=0

# --- flash FSBL to internal flash (no external loader needed) ---
if [ "$MODE" = "all" ] || [ "$MODE" = "fsbl" ]; then
  if [ -z "$FSBL_HEX" ]; then
    echo "====================================================================="
    echo "  FSBL binary -- SKIPPED (no .hex found in Debug/ or Release/)"
    echo "====================================================================="
    if [ "$MODE" = "fsbl" ]; then
      echo ""
      echo "  Build the project first, then re-run this script."
      exit 1
    fi
  else
    SIZE=$(stat -c%s "$FSBL_HEX" 2>/dev/null || stat -f%z "$FSBL_HEX" 2>/dev/null)
    echo ""
    echo "====================================================================="
    echo "  FSBL Binary"
    echo "  File:   $(basename $FSBL_HEX) ($SIZE bytes)"
    echo "  Target: 0x34000000 (internal flash)"
    echo "====================================================================="
    echo ""

    CMD="$PROGRAMMER -c port=SWD mode=NORMAL -d \"$FSBL_HEX\" 0x34000000 -v"
    echo "  $CMD"
    echo ""

    if eval "$CMD"; then
      echo ""
      echo "  [OK] FSBL"
    else
      echo ""
      echo "  [FAIL] FSBL"
      FAILED=$((FAILED + 1))
    fi
  fi
fi

# --- flash model binaries to external xSPI2 NOR flash ---
if [ "$MODE" = "all" ] || [ "$MODE" = "models" ]; then
  TOTAL=${#FLASH_ORDER[@]}
  INDEX=0

  for ENTRY in "${FLASH_ORDER[@]}"; do
    FILE="$MODEL_DIR/${ENTRY%%:*}"
    REST="${ENTRY#*:}"
    ADDR="${REST%%:*}"
    LABEL="${REST##*:}"
    INDEX=$((INDEX + 1))

    if [ ! -f "$FILE" ]; then
      echo "[$INDEX/$TOTAL] $LABEL -- SKIPPED (missing: $(basename $FILE))"
      continue
    fi

    SIZE=$(stat -c%s "$FILE" 2>/dev/null || stat -f%z "$FILE" 2>/dev/null)
    echo ""
    echo "====================================================================="
    echo "  [$INDEX/$TOTAL] $LABEL"
    echo "  File:   $(basename $FILE) ($SIZE bytes)"
    echo "  Target: $ADDR"
    echo "====================================================================="
    echo ""

    CMD="$PROGRAMMER -c port=SWD mode=NORMAL $LOADER_OPT -d \"$FILE\" $ADDR -v"
    echo "  $CMD"
    echo ""

    if eval "$CMD"; then
      echo ""
      echo "  [OK] $LABEL"
    else
      echo ""
      echo "  [FAIL] $LABEL"
      FAILED=$((FAILED + 1))
    fi
  done
fi

echo ""
echo "====================================================================="
if [ $FAILED -eq 0 ]; then
  echo "  All targets flashed and verified."
else
  echo "  $FAILED target(s) failed."
fi
echo "====================================================================="
