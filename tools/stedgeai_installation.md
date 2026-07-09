# ST Edge AI Core Installation

Installed on Linux via the modular online installer.

## Version

- **ST Edge AI Core**: v3.0.0 (components `stedgeai0300.*`)
- **STM32CubeAI**: 11.0.0-RC6
- **CLI**: `~/st/stedgeai/3.0/Utilities/linux/stedgeai`

## Prerequisites

Download the Linux online installer from:
https://www.st.com/en/development-tools/stedgeai-core.html
(ST registration required — free)

## Install command

```bash
cd ~/Downloads
chmod +x stedgeai-linux-onlineinstaller
./stedgeai-linux-onlineinstaller \
  --root ~/st/stedgeai \
  --accept-licenses -c \
  install stedgeai0300.stm32mcu stedgeai0300.stneuralart
```

## PATH setup

```bash
export PATH=$HOME/st/stedgeai/3.0/Utilities/linux:$PATH
```

Add to `~/.bashrc` to persist:

```bash
echo 'export PATH=$HOME/st/stedgeai/3.0/Utilities/linux:$PATH' >> ~/.bashrc
```

## Verification

```bash
stedgeai --version
```
