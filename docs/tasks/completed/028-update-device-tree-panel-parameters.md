# Task 028: Update Device Tree Panel Parameters

**Status:** completed  
**Priority:** high  
**Phase:** VIII - VM Integration & Android Analysis Integration  
**Assigned:** AI Agent  
**Created:** 2025-10-11  
**Context:** `ai/contexts/android-calibration-integration.md`

## Objective

Update `sun50i-h713-hy300.dts` device tree with correct panel parameters from Android analysis: native 1280x720 resolution (NOT 1080p), PWM frequencies (1.2MHz LED, 40kHz backlight), and factory calibration values from tvpq.db.

## Prerequisites

- [ ] Task 032 completed (Android firmware analysis)
- [ ] Task 028-1 completed (tvpq.db extraction)
- [ ] Task 028-4 completed (context document)
- [ ] Panel resolution confirmed: 1280x720 native
- [ ] PWM frequencies confirmed: 1.2MHz (LED), 40kHz (backlight)
- [ ] Factory calibration defaults extracted (brightness=50, contrast=50, etc.)

## Acceptance Criteria

- [ ] Panel resolution changed from 1920x1080 to 1280x720 in lcd0/panel node
- [ ] PWM channel 0 (LED) set to 1.2MHz (period = 833ns)
- [ ] PWM channel 5 (backlight) set to 40kHz (period = 25µs)
- [ ] Factory calibration properties added to panel node (brightness, contrast, saturation, etc.)
- [ ] Device tree compiles to valid DTB without errors
- [ ] DTB size reasonable (~10-11KB expected)
- [ ] Git commit with proper task reference and explanation of 1080p→720p correction

## Implementation Steps

### 1. Read Current Device Tree
Read `sun50i-h713-hy300.dts` sections:
- lcd0/panel node (around lines 450-480)
- pwm@300a000 node (around lines 240-260)
- Any existing resolution or timing parameters

### 2. Update Panel Resolution
Change panel node properties:
```dts
panel@0 {
    compatible = "hy300,lcd-panel";
    /* Native panel resolution (corrected from Android analysis) */
    width-mm = <160>;  /* Physical width */
    height-mm = <90>;  /* Physical height (16:9 aspect) */
    
    display-timings {
        native-mode = <&timing0>;
        timing0: 1280x720 {
            clock-frequency = <74250000>;  /* 1280x720@60Hz */
            hactive = <1280>;
            vactive = <720>;
            hfront-porch = <110>;
            hsync-len = <40>;
            hback-porch = <220>;
            vfront-porch = <5>;
            vsync-len = <5>;
            vback-porch = <20>;
        };
    };
};
```

### 3. Add Factory Calibration Properties
Add tvpq.db standard mode defaults:
```dts
    /* Factory calibration from tvpq.db (standard mode) */
    brightness = <50>;        /* 0x32 */
    contrast = <50>;          /* 0x32 */
    saturation = <50>;        /* 0x32 */
    hue = <50>;               /* 0x32 */
    sharpness = <100>;        /* 0x64 */
    tnr = <8>;                /* Temporal noise reduction */
    snr = <8>;                /* Spatial noise reduction */
    backlight-level = <29>;   /* 0x1d */
```

### 4. Update PWM Frequencies
Update pwm@300a000 node:
```dts
pwm@300a000 {
    compatible = "allwinner,sun50i-h616-pwm";
    reg = <0x0300a000 0x400>;
    clocks = <&ccu CLK_BUS_PWM>, <&ccu CLK_PWM>;
    clock-names = "bus", "mod";
    resets = <&ccu RST_BUS_PWM>;
    #pwm-cells = <3>;
    status = "okay";
    
    /* LED PWM - Channel 0 */
    pwm0 {
        period-ns = <833>;     /* 1.2 MHz = 833ns period */
        duty-cycle-ns = <416>; /* 50% default */
    };
    
    /* Backlight PWM - Channel 5 */
    pwm5 {
        period-ns = <25000>;   /* 40 kHz = 25µs period */
        duty-cycle-ns = <7250>; /* ~29% per backlight-level */
    };
};
```

### 5. Add HDMI Downscaling Note
Add comment documenting MIPS role:
```dts
/* 
 * Note: HDMI input supports 1920x1080, but MIPS co-processor
 * downscales to native 1280x720 panel resolution.
 * See drivers/media/platform/sunxi/hy300-hdmi-input.c
 */
```

### 6. Compile and Validate
```bash
# Compile device tree
dtc -I dts -O dtb -o sun50i-h713-hy300.dtb sun50i-h713-hy300.dts

# Check for errors
echo $?  # Should be 0

# Verify panel configuration
dtc -I dtb -O dts sun50i-h713-hy300.dtb | grep -A 20 "panel"

# Check file size
ls -lh sun50i-h713-hy300.dtb  # Should be ~10-11KB
```

### 7. Cross-Reference Updates
Update documentation:
- `docs/HY300_SPECIFIC_HARDWARE.md` - correct display resolution
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` - update display status
- `docs/ANDROID_FIRMWARE_ANALYSIS_COMPLETE_SUMMARY.md` - add DTS update reference

## Quality Validation

- [ ] DTS compiles without errors or warnings
- [ ] Panel resolution is 1280x720 (not 1080p)
- [ ] PWM frequencies match Android analysis (1.2MHz, 40kHz)
- [ ] Factory calibration values present in panel node
- [ ] DTB size is reasonable (~10-11KB)
- [ ] Git commit explains 1080p→720p correction with Android evidence
- [ ] Cross-reference documentation updated

## Next Task Dependencies

- Task 028-2: Update MIPS Driver (verify reserved-memory consistency)
- NixOS VM builds: Will use updated DTB
- Phase IX: Hardware testing (validates display output with correct resolution)

## Notes

- **CRITICAL CORRECTION:** Previous assumption of 1080p was incorrect
- Android analysis proves native panel is 1280x720
- HDMI input at 1080p is downscaled by MIPS co-processor
- This explains MIPS framebuffer size (26MB) - enough for multiple 720p buffers
- PWM frequencies must match factory settings for proper brightness control
- Calibration values enable factory-equivalent picture quality
- Display timings (porches, sync) may need hardware validation/adjustment
- Consider adding multiple display-timings for different modes if needed later
