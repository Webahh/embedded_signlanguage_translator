#!/bin/bash
# Flash inference model binaries to STM32N6570-DK OctoSPI flash
# Memory map (from STM32N657X0HXQ_AXISRAM2_fsbl.ld):
#   fingeralphabet   0x71000000
#   palm_detection   0x71200000
#   hand_landmark    0x71600000
#   ecblobs          0x72000000
#
# Usage:
#   ./flash_models.sh [--programmer <path>] [--loader <path>]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODEL_DIR="$SCRIPT_DIR/Assets/AI"

FLASH_ORDER=(
  "fingeralphabet_model_v3_atonbuf.xSPI2.bin:0x71000000:Finger Alphabet"
  "palm_detection_model_v3_atonbuf.xSPI2.bin:0x71200000:Palm Detection"
  "hand_landmark_model_v3_atonbuf.xSPI2.bin:0x71600000:Hand Landmark"
  "ecblobs.bin:0x72000000:EC Blobs"
)

# --- parse args ---
CUSTOM_PROGRAMMER=""
CUSTOM_LOADER=""
while [ $# -gt 0 ]; do
  case "$1" in
    --programmer) CUSTOM_PROGRAMMER="$2"; shift 2 ;;
    --loader)     CUSTOM_LOADER="$2";     shift 2 ;;
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
    "/home/oliver/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI" \
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
    "/home/oliver/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr" \
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
echo ""

# --- flash each binary sequentially ---
TOTAL=${#FLASH_ORDER[@]}
INDEX=0
FAILED=0

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

echo ""
echo "====================================================================="
if [ $FAILED -eq 0 ]; then
  echo "  All models flashed and verified."
else
  echo "  $FAILED model(s) failed."
fi
echo "====================================================================="
