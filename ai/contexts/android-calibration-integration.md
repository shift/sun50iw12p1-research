# Android Calibration Data Integration Context

**Created:** 2025-10-11  
**Phase:** Phase VIII - VM Integration & Android Analysis Integration  
**Related Tasks:** 028-series (calibration extraction and integration)

## Overview

This context document captures the integration of Android firmware calibration data into the mainline Linux device tree and drivers for the HY300 projector. The Android analysis (Task 032) revealed critical factory calibration data that must be replicated in the Linux system for proper hardware operation.

## Source Data Location

**Primary Source:** `firmware/extractions/super.img.extracted/33128670/rootfs/etc/tvconfig/tvpq.db`
- **Type:** SQLite3 database
- **Purpose:** Factory display calibration parameters
- **Size:** ~16KB

**Analysis Documents:**
- `docs/ANDROID_FIRMWARE_ANALYSIS_COMPLETE_SUMMARY.md` - Complete Android analysis
- `docs/ANDROID_CONFIG_CALIBRATION_PHASE3.md` - Configuration extraction details
- `docs/ANDROID_SYSTEM_COMPREHENSIVE_ANALYSIS.md` - System integration patterns

## Critical Calibration Data Extracted

### 1. MIPS Co-Processor Memory Layout

**From:** `libmips.so` analysis (Phase 2)
**Total Memory:** 40MB @ physical address `0x4b100000`

```
Memory Region Breakdown:
┌─────────────────────────────────────┐
│ Boot Region          4KB @ 0x00000  │ ← MIPS bootloader
├─────────────────────────────────────┤
│ Firmware Region     12MB @ 0x01000  │ ← MIPS main firmware (display.bin)
├─────────────────────────────────────┤
│ TSE Region           1MB @ 0xC01000 │ ← Transport Stream Engine
├─────────────────────────────────────┤
│ Framebuffer Region  26MB @ 0xD01000 │ ← Display framebuffer
└─────────────────────────────────────┘
```

**Integration Target:** `drivers/misc/sunxi-mipsloader.c`
- Update memory allocation logic
- Add region validation
- Document memory map in driver comments

### 2. Display Panel Parameters

**From:** tvpq.db `Picture_Mode` table
**Actual Resolution:** 1280x720 (NOT 1080p as previously assumed)

**Panel Timing Parameters:**
```
Native Resolution: 1280x720
HDMI Input: 1920x1080 (downscaled by MIPS processor)
Refresh Rate: 60Hz
Color Depth: 24-bit (8 bits per channel)
```

**Factory Calibration Values (standard mode, tvin=8, mode=1):**
```c
// Picture Mode Parameters
brightness:       50 (0x32)
contrast:         50 (0x32)
saturation:       50 (0x32)
hue:              50 (0x32)
sharpness:       100 (0x64)

// Noise Reduction
tnr:               8 (temporal noise reduction)
snr:               8 (spatial noise reduction)

// Backlight Control
backlight:        29 (0x1d)
dynamic_backlight: 3

// Color Management
colortemperature:  8
gamma:            varies (see Gamma_Point table)
dci:              varies
blackextension:   varies
```

**Integration Target:** `sun50i-h713-hy300.dts`
- Update panel node with correct 1280x720 resolution
- Add factory calibration as device tree properties
- Reference: lines 450-480 (lcd0 panel configuration)

### 3. White Balance Calibration

**From:** tvpq.db `White_Balance_Mode` table
**Per-Input Calibration:** Different values for HDMI (tvin=8) and internal (tvin=1)

**RGB Gain/Offset Structure:**
```c
struct white_balance {
    u8 tvin;        // Input source (1=internal, 8=HDMI, 9=other)
    u8 mode;        // Color temperature mode (1-4)
    u8 RGain;       // Red gain adjustment
    u8 GGain;       // Green gain adjustment
    u8 BGain;       // Blue gain adjustment
    s8 ROffset;     // Red offset adjustment
    s8 GOffset;     // Green offset adjustment
    s8 BOffset;     // Blue offset adjustment
};
```

**Integration Target:** Device tree or sysfs interface
- Add white balance properties to panel node
- Consider runtime calibration via sysfs

### 4. Gamma Correction Table

**From:** tvpq.db `Gamma_Point` table
**Type:** Lookup table (LUT) for gamma correction curve

**Structure:**
```c
// Gamma correction points
struct gamma_point {
    u32 id;      // Point index
    u32 value;   // Correction value
};
```

**Integration Target:** Display driver or MIPS firmware
- May be handled by MIPS co-processor
- Document for future display driver work

### 5. PWM Configuration

**From:** Android kernel analysis
**PWM Channels:**

```
LED PWM (Channel 0):
  - Frequency: 1.2 MHz
  - Period: 833 ns
  - Duty cycle: Variable (brightness control)
  - Device tree path: pwm@300a000 (pwm0)

Backlight PWM (Channel 5):
  - Frequency: 40 kHz
  - Period: 25 µs
  - Duty cycle: Variable (backlight brightness)
  - Device tree path: pwm@300a000 (pwm5)
```

**Integration Target:** `sun50i-h713-hy300.dts` PWM nodes
- Verify PWM frequency settings
- Add factory default duty cycles
- Reference: lines 240-260 (pwm controller)

## Database Schema Reference

### Picture_Mode Table
```sql
CREATE TABLE "Picture_Mode" (
  "tvin" integer NOT NULL,           -- Input source
  "mode" integer NOT NULL,           -- Picture mode preset
  "brightness" integer,              -- 0-100 range
  "contrast" integer,                -- 0-100 range
  "saturation" integer,              -- 0-100 range
  "hue" integer,                     -- 0-100 range
  "sharpness" integer,               -- 0-100 range
  "tnr" integer,                     -- Temporal noise reduction
  "snr" integer,                     -- Spatial noise reduction
  "backlight" integer,               -- Backlight level
  "colortemperature" integer,        -- Color temp mode
  "gamma" integer,                   -- Gamma mode reference
  "dci" integer,                     -- Dynamic contrast
  "blackextenstion" integer,         -- Black level extension
  "name" TEXT,                       -- Mode name
  "dynamic_backlight" integer,       -- Dynamic backlight mode
  PRIMARY KEY ("tvin", "mode")
);
```

### Picture Mode Names
- `standard` - Default balanced mode
- `cinema` - Movie optimized
- `vivid` - Enhanced colors
- `game` - Low latency gaming
- `hdr` - HDR content
- `computer` - Text/productivity
- `custom` - User customized

## Integration Priority

### High Priority (Must Have)
1. **MIPS Memory Layout** - Required for proper firmware loading
2. **Panel Resolution** - Critical for display output (1280x720 not 1080p!)
3. **PWM Configuration** - Required for LED and backlight control

### Medium Priority (Should Have)
4. **Picture Mode Defaults** - Standard mode calibration values
5. **White Balance** - Factory color calibration

### Low Priority (Nice to Have)
6. **Gamma Table** - May be handled by MIPS firmware
7. **All Picture Modes** - Advanced modes (cinema, vivid, etc.)

## Cross-Reference Updates Required

### Documentation Updates
1. `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
   - Update display section with 1280x720 resolution
   - Add calibration data status
   - Update MIPS memory layout documentation

2. `docs/ANDROID_FIRMWARE_ANALYSIS_COMPLETE_SUMMARY.md`
   - Add cross-references to device tree updates
   - Link to integration tasks

3. `docs/HY300_SPECIFIC_HARDWARE.md`
   - Update display specifications section
   - Add calibration data references

### Code Updates
1. `sun50i-h713-hy300.dts`
   - Panel resolution: 1280x720
   - PWM frequencies: 1.2MHz (LED), 40kHz (backlight)
   - Add calibration properties

2. `drivers/misc/sunxi-mipsloader.c`
   - Update memory map comments/documentation
   - Verify 40MB allocation at 0x4b100000
   - Add region size constants

3. `drivers/media/platform/sunxi/hy300-hdmi-input.c`
   - Update for 1920x1080 → 1280x720 downscaling
   - Document MIPS processor role in scaling

## Validation Methodology

### Device Tree Validation
```bash
# Compile and verify DTS changes
dtc -I dts -O dtb -o sun50i-h713-hy300.dtb sun50i-h713-hy300.dts

# Check for compilation errors
dtc -I dtb -O dts sun50i-h713-hy300.dtb | grep -A 10 "panel"

# Verify memory reservation
dtc -I dtb -O dts sun50i-h713-hy300.dtb | grep -A 5 "reserved-memory"
```

### Driver Validation
```bash
# Check MIPS loader documentation
grep -n "0x4b100000" drivers/misc/sunxi-mipsloader.c

# Verify memory region sizes
grep -n "mips_mem" drivers/misc/sunxi-mipsloader.c
```

### VM Testing Validation
```bash
# Build NixOS image with updated DTS
nix build .#hy300-image

# Test in VM with updated device tree
nix build .#checks.x86_64-linux.hy300-vm-test
```

## External References

### Android System References
- Android Panel HAL: `/vendor/lib64/hw/hwcomposer.sun50iw12p1.so`
- Display Manager Service: `/system/bin/displayd`
- MIPS Communication: `/vendor/bin/mips_service`

### Linux Mainline References
- Simple Panel Driver: `drivers/gpu/drm/panel/panel-simple.c`
- PWM Backlight Driver: `drivers/video/backlight/pwm_bl.c`
- Reserved Memory: `Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt`

## Known Limitations

1. **Gamma Table**: May require MIPS firmware support, not directly accessible from ARM
2. **Dynamic Backlight**: Algorithm runs in MIPS processor, not in ARM Linux
3. **Picture Mode Switching**: Requires MIPS communication protocol implementation
4. **HDR Mode**: May have hardware dependencies not yet understood

## Success Criteria

### Task 028-1: Database Extraction ✅ COMPLETED
- [x] tvpq.db analyzed and documented
- [x] All three tables extracted and understood
- [x] Factory calibration values documented
- [x] Integration priorities defined

### Task 028-2: MIPS Driver Update
- [ ] Memory layout documented in driver comments
- [ ] 40MB @ 0x4b100000 allocation verified
- [ ] Region sizes match Android analysis
- [ ] Driver compiles without errors

### Task 028-3: Device Tree Update
- [ ] Panel resolution changed to 1280x720
- [ ] PWM frequencies set correctly (1.2MHz, 40kHz)
- [ ] Factory calibration values added as properties
- [ ] DTS compiles to valid DTB

### Task 028-4: Context Document
- [x] Comprehensive integration guide created
- [x] All calibration data documented
- [x] Cross-references complete
- [x] Validation procedures defined

## Timeline

- **Task 028-1:** Completed (2025-10-11)
- **Task 028-2:** 1-2 hours (documentation updates)
- **Task 028-3:** 2-3 hours (DTS modifications and testing)
- **Task 028-4:** Completed (2025-10-11)

**Total Estimated Effort:** 3-5 hours for remaining integration work

## Notes

- This integration validates and corrects assumptions made in Phase IV device tree creation
- The 1280x720 native resolution is a critical correction from previous 1080p assumption
- MIPS memory layout provides exact addresses for firmware loading system (Task 018)
- Calibration data enables future picture quality optimization work
- All values are factory-calibrated and should be preserved exactly
