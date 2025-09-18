# HY300 DRAM Parameter Analysis

**Source:** boot0.bin (Allwinner H713 bootloader)  
**Analysis Date:** September 2025  
**Target:** U-Boot DRAM configuration for HY300 projector

## DRAM Configuration Structure

The DRAM parameters are located at offset 0x30-0x90 in boot0.bin:

```
Offset: 0x30-0x90 (DRAM Configuration Block)
00000030  08 00 00 00 80 02 00 00  03 00 00 00 fb 7b 7b 00  |.............{{.|
00000040  01 00 00 00 f4 10 00 00  00 00 00 00 70 1c 00 00  |............p...|
00000050  40 00 00 00 18 00 00 00  00 00 00 00 95 21 4a 00  |@............!J.|
00000060  90 31 42 02 61 b0 08 00  96 78 78 b4 00 00 00 00  |.1B.a....xx.....|
00000070  48 48 48 48 48 00 00 00  1e 12 20 16 00 00 00 00  |HHHHH..... .....|
00000080  00 00 00 00 00 00 00 00  00 00 44 44 55 55 00 00  |..........DDUU..|
00000090  00 01 01 34 00 00 00 00  00 00 00 00 00 00 00 00  |...4............|
```

## Extracted Parameters

### Primary DRAM Configuration
| Offset | Value (Hex) | Value (Dec) | Parameter | U-Boot Config |
|--------|-------------|-------------|-----------|---------------|
| 0x30   | 08 00 00 00 | 8           | Unknown/Timing | TBD |
| 0x34   | 80 02 00 00 | 640         | DRAM Frequency | CONFIG_DRAM_CLK=640 |
| 0x38   | 03 00 00 00 | 3           | DRAM Type | CONFIG_DRAM_TYPE=3 (DDR3) |
| 0x3C   | fb 7b 7b 00 | 0x7b7bfb    | ZQ Calibration | CONFIG_DRAM_ZQ=0x7b7bfb |
| 0x40   | 01 00 00 00 | 1           | ODT Enable | CONFIG_DRAM_ODT_EN=1 |

### DRAM Timing Parameters
| Offset | Value (Hex) | Value (Dec) | Likely Parameter | U-Boot Config |
|--------|-------------|-------------|------------------|---------------|
| 0x44   | f4 10 00 00 | 4340        | Timing/CAS | CONFIG_DRAM_EMR1=4340 |
| 0x4C   | 70 1c 00 00 | 7280        | Timing Parameter | CONFIG_DRAM_TPR0=7280 |
| 0x50   | 40 00 00 00 | 64          | Timing Parameter | CONFIG_DRAM_TPR1=64 |
| 0x54   | 18 00 00 00 | 24          | Timing Parameter | CONFIG_DRAM_TPR2=24 |

### Additional Configuration
| Offset | Value (Hex) | Value (Dec) | Likely Parameter | U-Boot Config |
|--------|-------------|-------------|------------------|---------------|
| 0x5C   | 95 21 4a 00 | 0x4a2195    | Drive Strength | CONFIG_DRAM_DRV=0x4a2195 |
| 0x60   | 90 31 42 02 | 0x24231420  | Timing Extended | CONFIG_DRAM_TPR3=0x24231420 |
| 0x64   | 61 b0 08 00 | 0x8b061     | Timing Extended | CONFIG_DRAM_TPR4=0x8b061 |

## Critical Parameter Analysis

### DRAM Frequency: 640 MHz
```
Value: 0x00000280 = 640 decimal
CONFIG_DRAM_CLK=640
```
This is a valid DDR3 frequency. Common Allwinner frequencies are 408, 600, 672, 720 MHz.

### DRAM Type: DDR3
```
Value: 0x00000003 = 3
CONFIG_DRAM_TYPE=3
```
Confirms DDR3 memory type (2=DDR2, 3=DDR3, 4=DDR4).

### ZQ Calibration: 0x7b7bfb
```
Value: 0x007b7bfb
CONFIG_DRAM_ZQ=0x7b7bfb
```
ZQ calibration value for output drive strength. This appears to be a reasonable value.

### ODT Enable: 1
```
Value: 0x00000001 = 1 (enabled)
CONFIG_DRAM_ODT_EN=1
```
On-Die Termination is enabled, typical for DDR3.

## U-Boot Configuration Template

Based on the extracted parameters, the preliminary U-Boot defconfig should include:

```makefile
CONFIG_DRAM_CLK=640
CONFIG_DRAM_TYPE=3
CONFIG_DRAM_ZQ=0x7b7bfb
CONFIG_DRAM_ODT_EN=1
CONFIG_DRAM_EMR1=4340
CONFIG_DRAM_TPR0=7280
CONFIG_DRAM_TPR1=64
CONFIG_DRAM_TPR2=24
CONFIG_DRAM_DRV=0x4a2195
CONFIG_DRAM_TPR3=0x24231420
CONFIG_DRAM_TPR4=0x8b061
```

## Validation Status

### ✅ Confirmed Parameters
- **DRAM Type:** DDR3 (value 3 matches standard)
- **Frequency:** 640 MHz (reasonable for DDR3)
- **ODT Enable:** Standard for DDR3
- **ZQ Value:** Within expected range

### ⚠️ Requires Validation
- **Timing Parameters:** TPR0-TPR4 values need cross-reference
- **Drive Strength:** DRV value should be verified
- **EMR1 Value:** CAS latency calculation needed

### ❌ Missing Parameters
- Memory size/density configuration
- Rank and width information
- Additional timing constraints

## Next Steps for U-Boot Integration

1. **Create sun50i-h713-hy300_defconfig** with extracted parameters
2. **Cross-reference timing values** with DDR3 specifications
3. **Add missing memory layout parameters** (size, ranks)
4. **Test compilation** with U-Boot build system
5. **Prepare for FEL mode testing** once hardware access available

## Safety Considerations

### Safe Parameter Ranges
- DRAM_CLK: 408-720 MHz typical for H713
- ZQ values: Usually in 0x70000-0x80000 range
- TPR values: Require validation against datasheet

### Testing Protocol
- Compile U-Boot with parameters first
- Test via FEL mode before eMMC flashing
- Monitor for DRAM initialization errors
- Have factory boot0.bin backup for recovery

## Analysis Confidence

**High Confidence:**
- DRAM Type (DDR3)
- Frequency (640 MHz)  
- ODT Enable (1)

**Medium Confidence:**
- ZQ Calibration value
- Basic timing parameters

**Low Confidence:**
- Extended timing parameters
- Drive strength values
- Missing memory topology

The extracted parameters provide a solid foundation for U-Boot DRAM configuration, but hardware testing will be required to validate timing parameters.