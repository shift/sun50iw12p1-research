# Factory DTB Analysis - HY300 Projector Hardware Configuration

**Status:** REVISED - Technical claims verified against actual DTB files  
**Date:** 2025-09-18  
**Source Files:** Four device tree files from `firmware/update.img.extracted/` (binary DTB format)

## Factory DTB File Locations

The factory device tree blob files are located at:
```
firmware/update.img.extracted/FC00/system.dtb      (3064 lines when decompiled)
firmware/update.img.extracted/1EAC00/system.dtb    (3064 lines when decompiled)  
firmware/update.img.extracted/16815C/system.dtb    (921 lines when decompiled)
firmware/update.img.extracted/C755C/system.dtb     (921 lines when decompiled)
```

**Note:** These are binary DTB files. To analyze their contents, use:
```bash
dtc -I dtb -O dts <dtb_file> | wc -l    # Count decompiled lines
dtc -I dtb -O dts <dtb_file>            # View full decompiled content
```

---

## Executive Summary

Analysis of four Device Tree Blob (DTB) files extracted from the HY300 projector's factory firmware reveals a definitive hardware blueprint based on the **Allwinner H713 System-on-Chip (SoC)**. The DTB files fall into two distinct categories: simplified configurations (921 lines) and full configurations (3064 lines) that include advanced display processing capabilities.

**Key Technical Findings:**
- **Confirmed SoC:** Allwinner H713 (sun50iw12p1) with tv303 compatibility  
- **GPU Architecture:** ARM Mali-Midgard family (NOT Mali-G31 as previously claimed)  
- **Display Processing:** MIPS co-processor subsystem for advanced video functionality  
- **Hardware Variants:** Two DTB configurations suggest different feature sets or production runs

The analysis provides concrete memory mappings, GPIO assignments, and peripheral configurations essential for mainline Linux porting efforts.

---

## 1. DTB File Structure Analysis

### File Classification

| DTB File Path | Decompiled Size | MIPS Loader | GPU Support | Classification |
|---------------|-----------------|-------------|-------------|---------------|
| firmware/update.img.extracted/16815C/system.dtb | 921 lines | NO | NO | Basic Configuration |
| firmware/update.img.extracted/C755C/system.dtb  | 921 lines | NO | NO | Basic Configuration |
| firmware/update.img.extracted/1EAC00/system.dtb | 3064 lines | YES | YES | Full Configuration |
| firmware/update.img.extracted/FC00/system.dtb   | 3064 lines | YES | YES | Full Configuration |

**Source:** Direct analysis of DTB file line counts and component presence using `dtc -I dtb -O dts`

### Core System Identification

All four DTB files consistently identify:
```dts
model = "sun50iw12";
compatible = "allwinner,tv303", "arm,sun50iw12p1";
```

**Source:** Lines 7-8 in `firmware/update.img.extracted/FC00/system.dtb` (decompiled), identical across all four files

This confirms:
- **SoC Model:** Allwinner H713 (internal designation: sun50iw12p1)
- **Platform Code:** tv303 (Allwinner's internal TV/projector platform identifier)
- **Architecture:** 64-bit ARM with dual address/size cells (`#address-cells = <0x02>; #size-cells = <0x02>`)

---

## 2. GPU Configuration Analysis

### Actual GPU Hardware (Full Configurations Only)

```dts
gpu@0x01800000 {
    device_type = "gpu";
    compatible = "arm,mali-midgard";
    reg = <0x00 0x1800000 0x00 0x10000>;
    interrupts = <0x00 0x75 0x04 0x00 0x76 0x04 0x00 0x4c 0x04>;
    interrupt-names = "JOB", "MMU", "GPU";
    clocks = <0x03 0x07 0x03 0x1c 0x03 0x1d>;
    clock-names = "clk_parent", "clk_mali", "clk_bus";
}
```

**Source:** `firmware/update.img.extracted/FC00/system.dtb` lines 1854-1866 (decompiled)

**Technical Corrections:**
- **GPU Family:** ARM Mali-Midgard architecture family
- **Memory Base:** 0x01800000 (25MB base address)
- **Memory Size:** 64KB register space (0x10000)
- **Interrupts:** Three interrupt lines (JOB, MMU, GPU)

**Note:** Previous claims of "Mali-G31" were incorrect. The `mali-midgard` compatible string indicates the architecture family, not a specific GPU model.

---

## 3. MIPS Co-Processor Display Subsystem

### MIPS Loader Configuration (Full Configurations Only)

```dts
mipsloader@3061000 {
    #address-cells = <0x02>;
    #size-cells = <0x02>;
    compatible = "allwinner,sunxi-mipsloader";
    reg = <0x00 0x3061000 0x00 0x100>;
    resets = <0x03 0x01 0x03 0x02 0x03 0x03>;
    reset-names = "bus-reset", "cold-reset", "soft-reset";
    clocks = <0x03 0x1b 0x03 0x1a>;
    clock-names = "bus-clk", "mips-clk";
    memory-region = <0x5d>;
    status = "okay";
}
```

**Source:** `firmware/update.img.extracted/FC00/system.dtb` lines 2859-2870 (decompiled)

### Reserved Memory Regions

```dts
mipsloader {
    reg = <0x00 0x4b100000 0x00 0x2841000>;
    phandle = <0x5d>;
};

decd {
    reg = <0x00 0x4d941000 0x00 0x20000>;
    phandle = <0x5c>;
};
```

**Source:** `firmware/update.img.extracted/FC00/system.dtb` lines 1065-1072 (decompiled)

**Memory Layout Analysis:**
- **MIPS Loader Base:** 0x4b100000 (1.254GB)
- **MIPS Loader Size:** 0x2841000 (~40.3MB)
- **Decoder Buffer:** 0x4d941000 (128KB)
- **Register Base:** 0x3061000 (256 bytes)

### TV Display Components

Present in full configurations only:
- **tvdisp@5000000** - Main display controller
- **tvcap@6800000** - TV capture interface  
- **tvtop@5700000** - TV top-level controller
- **dtmb@6600000** - Digital TV broadcast support

**Source:** `firmware/update.img.extracted/FC00/system.dtb` alias definitions lines 3021-3024 (decompiled)

---

## 4. Memory Architecture Analysis

### Address Space Layout

| Component | Base Address | Size | Purpose |
|-----------|--------------|------|---------|
| GPU | 0x01800000 | 64KB | Mali-Midgard registers |
| MIPS Registers | 0x03061000 | 256B | MIPS co-processor control |
| MIPS Firmware | 0x4b100000 | 40.3MB | MIPS loader and firmware |
| Decoder Buffer | 0x4d941000 | 128KB | Video decode buffer |
| TV Display | 0x05000000 | TBD | Display controller |
| TV Capture | 0x06800000 | TBD | Video capture |

**Source:** Register addresses extracted from `firmware/update.img.extracted/FC00/system.dtb` (decompiled)

---

## 5. Configuration Variants Analysis

### Basic vs Full Configuration Implications

**Basic Configurations (16815C, C755C):**
- Minimal 921-line DTB files
- No MIPS co-processor support
- No GPU acceleration  
- No advanced TV/video processing
- Likely used for basic firmware or recovery modes

**Full Configurations (1EAC00, FC00):**
- Complete 3064-line DTB files
- MIPS co-processor enabled
- GPU acceleration available
- Full TV/video processing pipeline
- Production firmware configurations

### GPIO and Pin Multiplexing

All configurations include comprehensive pin control definitions with specific GPIO assignments for:
- **UART Debug:** PH6, PH7 for mipsloader debug interface
- **MIPS JTAG:** PD8, PD9, PH8, PH9 for hardware debugging
- **Power Control:** Various GPIO pins for component power management

**Source:** `firmware/update.img.extracted/FC00/system.dtb` pinctrl definitions lines 1600-1700 (decompiled)

---

## 6. Technical Implications for Linux Porting

### Phase III Kernel Development Considerations

1. **Display Driver Complexity:** The MIPS co-processor architecture requires:
   - Firmware loading mechanism for MIPS subsystem
   - Inter-processor communication protocol
   - Coordination between ARM and MIPS for display output

2. **GPU Driver Requirements:**
   - Mali-Midgard driver stack (not Mali-G31)
   - Power domain management integration
   - Thermal zone configuration

3. **Hardware Variant Support:**
   - Basic configuration for minimal systems
   - Full configuration for production features
   - Runtime detection of available capabilities

---

## 7. Corrections to Previous Analysis

### Technical Claim Verification

| Previous Claim | Verification Result | Actual Evidence |
|----------------|-------------------|-----------------|
| "Mali-G31 GPU" | ❌ INCORRECT | `compatible = "arm,mali-midgard"` |
| "H713/tv303 relationship" | ✅ VERIFIED | `compatible = "allwinner,tv303", "arm,sun50iw12p1"` |
| "MIPS co-processor" | ✅ VERIFIED | `compatible = "allwinner,sunxi-mipsloader"` |
| "Four identical DTB files" | ❌ INCORRECT | Two distinct configurations (921 vs 3064 lines) |

### Methodological Improvements

- **Direct DTB Analysis:** All claims now backed by specific file references and line numbers
- **Comprehensive Comparison:** Analysis covers all four DTB files with documented differences  
- **Concrete Evidence:** Memory addresses, register mappings, and GPIO assignments provided
- **Proper Citations:** References include specific files and content locations

---

## 8. Next Phase Priorities

Based on concrete DTB analysis:

1. **MIPS Firmware Analysis:** Extract and analyze display.bin firmware for MIPS subsystem
2. **Driver Architecture Planning:** Design ARM-MIPS communication layer for display
3. **Hardware Testing Strategy:** Validate GPIO assignments and memory mappings
4. **Kernel Configuration:** Prepare device tree for mainline kernel integration

---

## References

**Primary Sources:**
- `firmware/update.img.extracted/FC00/system.dtb` - Full configuration DTB (3064 lines when decompiled)
- `firmware/update.img.extracted/1EAC00/system.dtb` - Full configuration DTB (3064 lines when decompiled)  
- `firmware/update.img.extracted/16815C/system.dtb` - Basic configuration DTB (921 lines when decompiled)
- `firmware/update.img.extracted/C755C/system.dtb` - Basic configuration DTB (921 lines when decompiled)

**File Format:** Binary Device Tree Blob (DTB) files, analyzed using `dtc -I dtb -O dts` for decompilation

**Analysis Tools:**
- Device Tree Compiler (`dtc -I dtb -O dts` for binary-to-source conversion)
- grep-based component extraction from decompiled source
- Line-by-line configuration comparison
- Memory mapping verification against decompiled DTB content

**Verification Method:** All technical claims verified against actual DTB file content with specific line references provided.