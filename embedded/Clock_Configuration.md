# Clock Configuration Overview

## 1. Oscillator Sources

| Oscillator | Frequency | Usage |
|-----------|-----------|-------|
| **HSI** | 64 MHz | Primary source for all PLLs (PLL1–PLL4) |
| **HSE** | 48 MHz | Used by Audio BSP (`PLL2` reconfig for SAI1) |
| **MSI** | 4 MHz | Default after reset |
| **LSI** | 32 kHz | Low-speed internal |
| **LSE** | 32.768 kHz | RTC (not explicitly used in project) |

---

## 2. PLL Configurations (all from HSI = 64 MHz)

### PLL1 — CPU Core Clock
```
VCO Input  = HSI / PLLM = 64 / 2  = 32 MHz
VCO Output = 32 × PLLN   = 32 × 25 = 800 MHz
PLL1 Out   = 800 / (PLLP1 × PLLP2) = 800 / (1 × 1) = 800 MHz
```

| Param      | Value       |
| ---------- | ----------- |
| PLLM       | 2           |
| PLLN       | 25          |
| PLLP1      | 1           |
| PLLP2      | 1           |
| **Output** | **800 MHz** |

### PLL2 — NPU & DCMIPP Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 125 = 1000 MHz
PLL2 Out   = 1000 / (1 × 1) = 1000 MHz
```

| Param      | Value        |
| ---------- | ------------ |
| PLLM       | 8            |
| PLLN       | 125          |
| PLLP1      | 1            |
| PLLP2      | 1            |
| **Output** | **1000 MHz** |

### PLL3 — AXISRAM3/4/5/6 Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 225 = 1800 MHz
PLL3 Out   = 1800 / (1 × 2) = 900 MHz
```

| Param      | Value       |
| ---------- | ----------- |
| PLLM       | 8           |
| PLLN       | 225         |
| PLLP1      | 1           |
| PLLP2      | 2           |
| **Output** | **900 MHz** |

### PLL4 — Peripheral / LTDC Base Clock
```
VCO Input  = HSI / PLLM = 64 / 8  = 8 MHz
VCO Output = 8 × PLLN   = 8 × 225 = 1800 MHz
PLL4 Out   = 1800 / (6 × 6) = 50 MHz
```

| Param      | Value      |
| ---------- | ---------- |
| PLLM       | 8          |
| PLLN       | 225        |
| PLLP1      | 6          |
| PLLP2      | 6          |
| **Output** | **50 MHz** |

---

## 3. Internal Connection Clocks (ICC)

| ICK  | Source | Divider   | Derived From                  | Frequency    |
| ---- | ------ | --------- | ----------------------------- | ------------ |
| IC1  | PLL1   | 1         | PLL1 output                   | **800 MHz**  |
| IC2  | PLL1   | 2         | PLL1 output                   | **400 MHz**  |
| IC6  | PLL2   | 1         | PLL2 output                   | **1000 MHz** |
| IC7  | PLL2   | 1 (or 17) | PLL2 output (SAI1, Audio BSP) | variable     |
| IC11 | PLL3   | 1         | PLL3 output                   | **900 MHz**  |
| IC16 | PLL4   | 2         | PLL4 output (LTDC)            | **25 MHz**   |
| IC17 | PLL2   | 3         | PLL2 output (DCMIPP)          | **333 MHz**  |
| IC18 | PLL1   | 40        | PLL1 output (CSI)             | **20 MHz**   |

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
  │                                │              └── GPU2D,DMA2D,GFXMMU 400 MHz
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

## 9. Summary Table

| Clock Signal         | Source      | Divider Chain | Frequency    | Used By                                |
| -------------------- | ----------- | ------------- | ------------ | -------------------------------------- |
| CPUCLK               | PLL1 (HSI)  | /1            | **800 MHz**  | CPU core                               |
| AXI (sysb_ck)        | PLL1        | /2            | **400 MHz**  | GPU2D, DMA2D, GFXMMU, AXI interconnect |
| HCLK                 | AXI         | /2            | **200 MHz**  | AHB peripherals, XSPI1/2               |
| PCLK1                | HCLK        | /1            | **200 MHz**  | APB1: TIM4, I2C, etc.                  |
| PCLK2                | HCLK        | /1            | **200 MHz**  | APB2: SPI5, etc.                       |
| PCLK4                | HCLK        | /1            | **200 MHz**  | APB4: LPUART, etc.                     |
| PCLK5                | HCLK        | /1            | **200 MHz**  | APB5                                   |
| NPU (sysc_ck)        | PLL2        | /1            | **1000 MHz** | NPU core                               |
| AXISRAM3-6 (sysd_ck) | PLL3        | /1            | **900 MHz**  | NPU SRAM memory                        |
| DCMIPP               | PLL2 (IC17) | /3            | **333 MHz**  | Camera interface                       |
| CSI                  | PLL1 (IC18) | /40           | **20 MHz**   | CSI-2 receiver                         |
| LTDC                 | PLL4 (IC16) | /2            | **25 MHz**   | LCD display controller                 |
| SAI1                 | PLL2 (IC7)  | variable      | variable     | Audio (BSP only)                       |
| XSPI1                | HCLK        | —             | **200 MHz**  | External PSRAM / Flash                 |
| XSPI2                | HCLK        | —             | **200 MHz**  | External PSRAM / Flash                 |

---

## 9. Voltage & Power

- **SMPS:** Set to `SMPS_VOLTAGE_OVERDRIVE` before clock configuration
- This enables the higher voltage rail needed for 800 MHz CPU and 1000 MHz NPU operation