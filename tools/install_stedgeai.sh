#!/usr/bin/env bash
set -euo pipefail

STEDGEAI_ROOT="$HOME/st/stedgeai"
INSTALLER="stedgeai-linux-onlineinstaller"

echo "=== ST Edge AI Core Installer ==="
echo ""

# Check if already installed
if [ -f "$STEDGEAI_ROOT/3.0/Utilities/linux/stedgeai" ]; then
    echo "ST Edge AI Core already installed at $STEDGEAI_ROOT/3.0"
    stedgeai --version
    exit 0
fi

# Check for installer
if [ ! -f "$INSTALLER" ]; then
    echo "ERROR: '$INSTALLER' not found in current directory."
    echo ""
    echo "Download it from: https://www.st.com/en/development-tools/stedgeai-core.html"
    echo "(ST registration required — free)"
    exit 1
fi

chmod +x "$INSTALLER"

echo "Installing ST Edge AI Core (STM32CubeAI 11.0.0-RC6) ..."
./"$INSTALLER" \
    --root "$STEDGEAI_ROOT" \
    --accept-licenses -c \
    install stedgeai0300.stm32mcu stedgeai0300.stneuralart

echo ""
echo "=== Done ==="

# Add to PATH for current session
export PATH="$STEDGEAI_ROOT/3.0/Utilities/linux:$PATH"

echo "ST Edge AI Core installed at: $STEDGEAI_ROOT/3.0"
echo ""
stedgeai --version
echo ""
echo "Add to ~/.bashrc to persist PATH:"
echo "  echo 'export PATH=\$HOME/st/stedgeai/3.0/Utilities/linux:\$PATH' >> ~/.bashrc"
