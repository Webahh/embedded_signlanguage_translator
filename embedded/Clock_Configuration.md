# Clock Configuration Overview

## 1. Oscillator Sources

| Oscillator | Frequency | Usage |
|-----------|-----------|-------|
| **HSI** | 64 MHz | Primary source for all PLLs (PLL1–PLL4) |
| **HSE** | 48 MHz | Used by Audio BSP (`PLL2` reconfig for SAI1) |
| **MSI** | 4 MHz | Default after reset |
| **LSI** | 32 kHz | Low-speed internal |
| **LSE** | 32.768 kHz | RTC (not explicitly used in project) |

**Defined in:** `Inc/stm32n6xx_hal_conf.h:91-141`

---

## 2. PLL Configurations (all from HSI = 64 MHz)

### PLL1 — CPU Core Clock
```
VCO Input  = HSI / PLLM = 64 / 2  = 32 MHz
VCO Output = 32 × PLLN   = 32 × 25 = 800 MHz
PLL1 Out   = 800 / (PLLP1 × PLLP2) = 800 / (1 × 1) = 800 MHz
```
| Param | Value |
|-------|-------|
| PLLM | 2 |
| PLLN | 25 |
| PLLP1 | 1 |
| PLLP2 | 1 |
| **Output** | **800 MHz** |

### PLL2 — NPU & DCMIPP Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 125 = 1000 MHz
PLL2 Out   = 1000 / (1 × 1) = 1000 MHz
```
| Param | Value |
|-------|-------|
| PLLM | 8 |
| PLLN | 125 |
| PLLP1 | 1 |
| PLLP2 | 1 |
| **Output** | **1000 MHz** |

### PLL3 — AXISRAM3/4/5/6 Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 225 = 1800 MHz
PLL3 Out   = 1800 / (1 × 2) = 900 MHz
```
| Param | Value |
|-------|-------|
| PLLM | 8 |
| PLLN | 225 |
| PLLP1 | 1 |
| PLLP2 | 2 |
| **Output** | **900 MHz** |

### PLL4 — Peripheral / LTDC Base Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 225 = 1800 MHz
PLL4 Out   = 1800 / (6 × 6) = 50 MHz
```
| Param | Value |
|-------|-------|
| PLLM | 8 |
| PLLN | 225 |
| PLLP1 | 6 |
| PLLP2 | 6 |
| **Output** | **50 MHz** |

---

## 3. Internal Connection Clocks (ICK / ICC)

| ICK | Source | Divider | Derived From | Frequency |
|-----|--------|---------|-------------|-----------|
| **IC1** | PLL1 | 1 | PLL1 output | **800 MHz** |
| **IC2** | PLL1 | 2 | PLL1 output | **400 MHz** |
| **IC6** | PLL2 | 1 | PLL2 output | **1000 MHz** |
| **IC7** | PLL2 | 1 (or 17) | PLL2 output (SAI1, Audio BSP) | variable |
| **IC11** | PLL3 | 1 | PLL3 output | **900 MHz** |
| **IC16** | PLL4 | 2 | PLL4 output (LTDC) | **25 MHz** |
| **IC17** | PLL2 | 3 | PLL2 output (DCMIPP) | **333 MHz** |
| **IC18** | PLL1 | 40 | PLL1 output (CSI) | **20 MHz** |

---

## 4. System Bus Clocks

| Clock | Source Divider | Formula | Frequency |
|-------|---------------|---------|-----------|
| **CPUCLK** (sysa_ck) | IC1 = PLL1/1 | 800/1 | **800 MHz** |
| **AXI** (sysb_ck) | IC2 = PLL1/2 | 800/2 | **400 MHz** |
| **NPU** (sysc_ck) | IC6 = PLL2/1 | 1000/1 | **1000 MHz** |
| **AXISRAM3-6** (sysd_ck) | IC11 = PLL3/1 | 900/1 | **900 MHz** |
| **HCLK** | AXI / 2 | 400/2 | **200 MHz** |
| **PCLK1** (APB1) | HCLK / 1 | 200/1 | **200 MHz** |
| **PCLK2** (APB2) | HCLK / 1 | 200/1 | **200 MHz** |
| **PCLK4** (APB4) | HCLK / 1 | 200/1 | **200 MHz** |
| **PCLK5** (APB5) | HCLK / 1 | 200/1 | **200 MHz** |

### Clock Tree Diagram
```
HSI (64 MHz)
  │
  ├── PLL1 (M=2, N=25) ── 800 MHz ── IC1/÷1 ── CPUCLK    800 MHz
  │                                ├── IC2/÷2 ── AXI       400 MHz
  │                                │              ├── HCLK/÷2 ── 200 MHz
  │                                │              │    ├── PCLK1 (APB1)  200 MHz
  │                                │              │    ├── PCLK2 (APB2)  200 MHz
  │                                │              │    ├── PCLK4 (APB4)  200 MHz
  │                                │              │    └── PCLK5 (APB5)  200 MHz
  │                                │              └── GPU2D, DMA2D, GFXMMU  400 MHz
  │                                └── IC18/÷40 ─ CSI                    20 MHz
  │
  ├── PLL2 (M=8, N=125) ─ 1000 MHz ── IC6/÷1 ─── NPU        1000 MHz
  │                                ├── IC17/÷3 ── DCMIPP     333 MHz
  │                                └── IC7 (SAI1 BSP)        variable
  │
  ├── PLL3 (M=8, N=225) ─ 900 MHz ── IC11/÷1 ── AXISRAM3-6  900 MHz
  │                                    (NPU RAMs)
  │
  └── PLL4 (M=8, N=225) ─ 50 MHz ── IC16/÷2 ── LTDC         25 MHz
```

---

## 5. Peripheral Clock Assignments

### XSPI1
- **Source:** HCLK (AHB bus)
- **Frequency:** 200 MHz
- **Config:** `RCC_XSPI1CLKSOURCE_HCLK` (`main.c:285`)

### XSPI2
- **Source:** HCLK (AHB bus)
- **Frequency:** 200 MHz
- **Config:** `RCC_XSPI2CLKSOURCE_HCLK` (`main.c:289`)

### DCMIPP (Camera Interface)
- **Source:** IC17 = PLL2 / 3
- **Frequency:** 333 MHz
- **Config:** `RCC_DCMIPPCLKSOURCE_IC17` (`main.c:432`)
- **Note:** Application overrides BSP weak default (which uses PLL1/4 = 300 MHz)

### CSI (Camera Serial Interface)
- **Source:** IC18 = PLL1 / 40
- **Frequency:** 20 MHz
- **Config:** `RCC_ICCLKSOURCE_PLL1` + divider 40 (`main.c:440-441`)
- **Note:** Application overrides BSP weak default (divider 60 → 13.3 MHz)

### LTDC (Display Controller)
- **Source:** IC16 = PLL4 / 2
- **Frequency:** 25 MHz
- **Config:** `RCC_LTDCCLKSOURCE_IC16` (`stm32n6570_discovery_lcd.c:433-435`)
- **BSP file:** `STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_lcd.c:411`

### SAI1 (Audio — BSP only, not used in main app)
- **Source:** IC7 = PLL2 (with optional HSE reconfiguration)
- **BSP file:** `stm32n6570_discovery_audio.c:1315`
- **Note:** Audio BSP reconfigures PLL2 with HSE (48 MHz) as source, different M/N/P dividers depending on sample rate

### USB1 (OTG_HS)
- **Clock enabled:** `__HAL_RCC_USB1_OTG_HS_CLK_ENABLE()` in `Lib/screenl/Src/scrl_usb.c`
- **PHY clock:** PWR clock enabled (`__HAL_RCC_PWR_CLK_ENABLE()`)

### SPI5
- **Clock enabled:** `__HAL_RCC_SPI5_CLK_ENABLE()` in `Lib/screenl/Src/scrl_spi.c`
- Also enables `__HAL_RCC_HPDMA1_CLK_ENABLE()` for DMA

### TIM4
- **Clock enabled:** `__HAL_RCC_TIM4_CLK_ENABLE()` in `Src/freertos_bsp.c`
- **Timed from:** PCLK1 = 200 MHz

### I2C1/I2C2
- **Not explicitly enabled in project source code**
- Handled internally by BSP drivers (`BSP_I2C1_Init`, `BSP_I2C2_Init`)
- Derive clock from APB domain (200 MHz PCLK) via I2C kernel clock mux

---

## 6. GPU & Graphics Clocks

### GPU2D
- **Clock enable:** `__HAL_RCC_GPU2D_CLK_ENABLE()` (`main.c:496`)
- **Reset:** Force + Release (`main.c:494-495`)
- **Domain:** AXI bus clock domain (400 MHz)
- **IRQs:** GPU2D_IRQn (priority 6), GPU2D_ER_IRQn (priority 5)

### GFXMMU
- **Clock enable:** `__HAL_RCC_GFXMMU_CLK_ENABLE()` (`main.c:517`)
- **Domain:** AXI bus clock domain (400 MHz)
- **IRQ:** GFXMMU_IRQn (priority 5)

### DMA2D
- **Clock enable:** `__HAL_RCC_DMA2D_CLK_ENABLE()` (LCD BSP `stm32n6570_discovery_lcd.c:1500`)
- **Domain:** AXI bus clock domain (400 MHz)

### LTDC
- **Clock enable:** `__HAL_RCC_LTDC_CLK_ENABLE()` (LCD BSP `stm32n6570_discovery_lcd.c:1344`)
- **Pixel clock:** 25 MHz (via IC16 = PLL4/2)

---

## 7. NPU & NPU Memory Clocks

### NPU Core
- **Domain:** sysc_ck = IC6 = PLL2 / 1 = **1000 MHz**
- **Clock enable:** `__HAL_RCC_NPU_CLK_ENABLE()` (`main.c:97`)
- **Reset:** Force + Release (`main.c:98-99`)

### NPU RAMs (AXISRAM3/4/5/6)
- **Domain:** sysd_ck = IC11 = PLL3 / 1 = **900 MHz**
- **Clock enables:** `__HAL_RCC_AXISRAM3_MEM_CLK_ENABLE()` etc. (`main.c:102-105`)
- **RAMCFG enable:** `__HAL_RCC_RAMCFG_CLK_ENABLE()` + `HAL_RAMCFG_EnableAXISRAM()` for each SRAM instance (`main.c:106-115`)

### NPU Cache AXI
- **Clock enables:**
  - `__HAL_RCC_CACHEAXIRAM_MEM_CLK_ENABLE()` (`main.c:451`)
  - `__HAL_RCC_CACHEAXI_CLK_ENABLE()` (`main.c:452`)
- **Reset:** Force + Release (`main.c:453-454`)

---

## 8. Summary Table

| Clock Signal | Source | Divider Chain | Frequency | Used By |
|-------------|--------|---------------|-----------|---------|
| CPUCLK | PLL1 (HSI) | /1 | **800 MHz** | CPU core |
| AXI (sysb_ck) | PLL1 | /2 | **400 MHz** | GPU2D, DMA2D, GFXMMU, AXI interconnect |
| HCLK | AXI | /2 | **200 MHz** | AHB peripherals, XSPI1/2 |
| PCLK1 | HCLK | /1 | **200 MHz** | APB1: TIM4, I2C, etc. |
| PCLK2 | HCLK | /1 | **200 MHz** | APB2: SPI5, etc. |
| PCLK4 | HCLK | /1 | **200 MHz** | APB4: LPUART, etc. |
| PCLK5 | HCLK | /1 | **200 MHz** | APB5 |
| NPU (sysc_ck) | PLL2 | /1 | **1000 MHz** | NPU core |
| AXISRAM3-6 (sysd_ck) | PLL3 | /1 | **900 MHz** | NPU SRAM memory |
| DCMIPP | PLL2 (IC17) | /3 | **333 MHz** | Camera interface |
| CSI | PLL1 (IC18) | /40 | **20 MHz** | CSI-2 receiver |
| LTDC | PLL4 (IC16) | /2 | **25 MHz** | LCD display controller |
| SAI1 | PLL2 (IC7) | variable | variable | Audio (BSP only) |
| XSPI1 | HCLK | — | **200 MHz** | External PSRAM / Flash |
| XSPI2 | HCLK | — | **200 MHz** | External PSRAM / Flash |

---

## 9. Voltage & Power

- **SMPS:** Set to `SMPS_VOLTAGE_OVERDRIVE` before clock configuration (`main.c:197`)
- This enables the higher voltage rail needed for 800 MHz CPU and 1000 MHz NPU operation

---

## 10. Source File Locations

| Component | File | Line(s) |
|-----------|------|---------|
| SystemClock_Config | `Src/main.c` | 191–295 |
| MX_DCMIPP_ClockConfig | `Src/main.c` | 426–447 |
| NPURam_enable | `Src/main.c` | 95–116 |
| npu_cache_enable_clocks_and_reset | `Src/main.c` | 449–455 |
| HAL_GPU2D_MspInit | `Src/main.c` | 490–503 |
| HAL_GPU2D_MspDeInit | `Src/main.c` | 506–511 |
| HAL_GFXMMU_MspInit | `Src/main.c` | 514–522 |
| HAL_GFXMMU_MspDeInit | `Src/main.c` | 524–531 |
| MX_LTDC_ClockConfig (BSP weak) | `STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_lcd.c` | 411–442 |
| MX_SAI1_ClockConfig (BSP weak) | `STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_audio.c` | 1315–1368 |
| MX_DCMIPP_ClockConfig (BSP weak) | `STM32Cube_FW_N6/Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_camera.c` | 534–564 |
| Oscillator value defines | `Inc/stm32n6xx_hal_conf.h` | 91–141 |
| SystemCoreClockUpdate (secure) | `STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Source/Templates/system_stm32n6xx_s.c` | 260–402 |
| SystemCoreClockUpdate (non-secure) | `STM32Cube_FW_N6/Drivers/CMSIS/Device/ST/STM32N6xx/Source/Templates/system_stm32n6xx_ns.c` | 233 |
