# STM32N6570-DK — Full Memory Map (RM0486)

## CPU Address Space (Cortex-M55, 32-bit)

```
 0xE0000000 ┬─ ARM Private Peripheral Bus (NVIC, SysTick, MPU, SCB, ITM, ETM)
            └─ CMSIS system registers

 0x90000000 ┬─ XSPI1 Memory-Mapped Region (256 MB max)
            │   ┌───────────────────────────────────────────────────┐
            │   │ 0x90000000  Hexadeca-SPI PSRAM (256-Mbit)         │
            │   │             32 MB on-board AP Memory              │
            │   │             LCD frame buffers (800×480×2)         │
            │   └───────────────────────────────────────────────────┘
            │   Peripheral base: 0x58025000

 0x80000000 ┬─ XSPI3 Memory-Mapped Region (256 MB max)
            │   [Not populated on STM32N6570-DK]
            │   Peripheral base: 0x5802D000

 0x70000000 ┬─ XSPI2 Memory-Mapped Region (256 MB max)
            │   ┌───────────────────────────────────────────────────┐
            │   │ 0x70000000  Firmware / FSBL (loaded at boot)      │
            │   ├───────────────────────────────────────────────────┤
            │   │ 0x71000000  Finger Alphabet model  (+2 MB)        │
            │   ├───────────────────────────────────────────────────┤
            │   │ 0x71200000  Palm Detection model   (+4 MB)        │
            │   ├───────────────────────────────────────────────────┤
            │   │ 0x71600000  Hand Landmark model    (+10 MB)       │
            │   ├───────────────────────────────────────────────────┤
            │   │ 0x72000000  EC Blobs               (Remainder)    │
            │   │             ↓ copied to AXISRAM1 at init          │
            │   └───────────────────────────────────────────────────┘
            │   External: Macronix MX66UW1G45G (128 MB NOR)
            │   Peripheral base: 0x5802A000

 0x58000000 ┬─ AHB5 Peripheral Region
            │   XSPI1      0x58025000   XSPI I/O manager  0x5802B400
            │   XSPI2      0x5802A000   XSPI3             0x5802D000
            │   LTDC       0x58001000   DCMIPP (camera)   0x58002000
            │   NPU        0x580E0000   NPU Cache         0x580DFC00
            │   VENC       0x58005000   JPEG              0x58023000
            │   DMA2D      0x58021000   Ethernet          0x58036000
            │   USB OTG1   0x58040000   USB OTG2          0x58080000
            │   SDMMC1     0x58027000   SDMMC2            0x58026800

 0x56000000 ┬─ AHB4 / APB4 Peripheral Region
            │   Pinctrl    0x56020000   RCC               0x56028000
            │   BSEC/OTP   0x56009000   RTC               0x56004000
            │   IWDG       0x56004800   CRC               0x56024C00
            │   EXTI       0x56025000   I2C4              0x56001C00

 0x52000000 ┬─ APB2 Peripheral Region
            │   TIM1       0x52000000   TIM8              0x52000400
            │   USART1     0x52001000   USART6            0x52001400
            │   SPI1/I2S1  0x52003000   SPI4              0x52003400
            │   SAI1       0x52005800   SAI2              0x52005C00

 0x50000000 ┬─ APB1 Peripheral Region
            │   TIM2-7     0x50000000   TIM12-14          0x50001800
            │   USART2-3   0x50004400   UART4-5           0x50004C00
            │   I2C1-3     0x50005400   SPI2-3            0x50003800
            │   FDCAN1-3   0x5000A000   WWDG              0x50002C00
            │   ADC1       0x50022000   ADC2              0x50022100
            │   DMA (GPDMA1) 0x50021000

 0x34000000 ┬─ On-Chip SRAM (4.2 MB contiguous, AXI-bus)
            │   ┌───────────────────────────────────────────────────┐
            │   │ FLEXMEM / Retention (80 KB)                       │
            │   │ 0x34000000 ─ 0x34013FFF                           │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM1 (~1.5 MB)                                │
            │   │ 0x34000000 ─ 0x3417FFFF                           │
            │   │   ├─ 0x34080000  EC_RUNTIME (512 KB)              │
            │   │   └─ 0x34100000  EC_CONST   (512 KB)              │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM2 (256 KB)                                 │
            │   │ 0x34180000 ─ 0x341BFFFF                           │
            │   │   ├─ 0x34180400  ROM: boot code + vectors         │
            │   │   └─ 0x341C0000  RAM: .data, .bss, stack          │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM3 / npuRAM3 (448 KB)                       │
            │   │ 0x34200000 ─ 0x3426FFFF  NPU scratch              │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM4 / npuRAM4 (448 KB)                       │
            │   │ 0x34270000 ─ 0x342DFFFF  NPU input (nn_in)        │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM5 / npuRAM5 (448 KB)                       │
            │   │ 0x342E0000 ─ 0x3434FFFF  NPU output (nn_out)      │
            │   ├───────────────────────────────────────────────────┤
            │   │ AXISRAM6 / npuRAM6 (448 KB)                       │
            │   │ 0x34350000 ─ 0x343BFFFF  NPU / general            │
            │   └───────────────────────────────────────────────────┘

 0x30000000 ┬─ Internal flash (not present on STM32N657x0)
            │   [STM32N657x0 has no internal flash]

 0x00000000   Boot ROM / Flash alias (depending on BOOT pin config)
```

---

## Data Flow — Model Inference Pipeline

```
  OctoSPI Flash (XSPI2)                    Internal SRAM (AXISRAM1)
  ┌─────────────────────┐                   ┌─────────────────────┐
  │ 0x72000000 EC Blobs │ ──── copy ──────→ │ 0x34080000 EC_RT    │
  │                     │                   │ 0x34100000 EC_CONST │
  └─────────────────────┘                   └─────────────────────┘
           │                                          │
           │ NPU AXI Cache                            │ NPU reads
           │ streams weights                          │ EC instructions
           ▼                                          ▼
  ┌─────────────────────────────────────────────────────────────┐
  │                    Neural-ART NPU                           │
  │  STRENG (stream) → CONVACC (convolution) → POOL → ACTIV     │
  └─────────────────────────────────────────────────────────────┘
           │                                     ▲
           │ nn_in                               │ nn_out
           ▼                                     │
  ┌──────────────────┐              ┌──────────────────┐
  │ npuRAM4 0x3427.. │  camera in   │ npuRAM5 0x342E.. │  heatmaps
  │ (448 KB)         │ ←── DCMIPP   │ (448 KB)         │ ──→ CPU
  └──────────────────┘              └──────────────────┘

  OctoSPI PSRAM (XSPI1)
  ┌─────────────────────────────────────────┐
  │ 0x90000000  LCD frame buffers (32 MB)   │ ←── LTDC reads for display
  │             800×480×2 bytes × 2 layers  │
  └─────────────────────────────────────────┘
```

---

## Linker Regions (`STM32N657X0HXQ_AXISRAM2_fsbl.ld`)

| Region     | Address      | Size   | Description                              |
| ---------- | ------------ | ------ | ---------------------------------------- |
| ROM        | `0x34180400` | 255 KB | `.isr_vector`, `.text`, `.rodata`        |
| RAM        | `0x341C0000` | 256 KB | `.data`, `.bss`, stack                   |
| EC_RUNTIME | `0x34080000` | 512 KB | `.ecblobs_runtime` (NPU working state)   |
| EC_CONST   | `0x34100000` | 512 KB | `.ecblobs_const` (NPU microinstructions) |
| PSRAM      | `0x91000000` | 16 MB  | `.psram_section` (LCD buffers)           |
| ECBLOBS    | `0x72000000` | 8 MB   | External flash (raw EC blob binary)      |

**Symbols exported by linker:**

```c
_mem_pool_xSPI2_fingeralphabet_model_v3 = 0x71000000;
_mem_pool_xSPI2_palm_detection_model_v3 = 0x71200000;
_mem_pool_xSPI2_hand_landmark_model_v3  = 0x71600000;
```

---

## Flash Layout — `flash_models.sh`

| Address      | Binary                                         | Size      |
| ------------ | ---------------------------------------------- | --------- |
| `0x70000000` | *(FSBL firmware — not flashed by this script)* | —         |
| `0x71000000` | `fingeralphabet_model_v3_atonbuf.xSPI2.bin`    | +2 MB     |
| `0x71200000` | `palm_detection_model_v3_atonbuf.xSPI2.bin`    | +4 MB     |
| `0x71600000` | `hand_landmark_model_v3_atonbuf.xSPI2.bin`     | +10 MB    |
| `0x72000000` | `ecblobs.bin`                                  | Remainder |
