# Task 019: AV1 Hardware Decoding Research for H713 SoC

## Task Information
- **Task ID**: 019-av1-hardware-decoding-research  
- **Created**: 2025-01-22
- **Status**: in_progress
- **Priority**: medium
- **Phase**: VIII (VM Testing and Service Integration)

## Objective
Research AV1 hardware decoding capabilities for the Allwinner H713 SoC to determine video acceleration support for the HY300 projector media center functionality.

## Key Findings Summary

### ❌ **No Confirmed H713 AV1 Hardware Support**
Based on comprehensive analysis of mainline kernel sources and factory firmware, **H713 does not have verified AV1 hardware decoding support**.

### ✅ **Confirmed Video Decoding Capabilities**
H713 supports these hardware-accelerated codecs through Allwinner CedarX/Cedrus VPU:
- **H.264/AVC** - Full hardware acceleration
- **H.265/HEVC** - Full hardware acceleration (up to 10-bit on H6+ variants)
- **MPEG-2** - Full hardware acceleration  
- **VP8** - Full hardware acceleration
- **VP9** - Software decode only (no HW acceleration)
- **AV1** - Software decode only (no HW acceleration)

## Technical Analysis

### Mainline Kernel Video Engine Support

**Cedrus Driver Analysis** (`linux-6.16.7/drivers/staging/media/sunxi/cedrus/`):
```c
// H6 capabilities (H713 inherits from H6)
static const struct cedrus_variant sun50i_h6_cedrus_variant = {
    .capabilities = CEDRUS_CAPABILITY_UNTILED |
                   CEDRUS_CAPABILITY_MPEG2_DEC |
                   CEDRUS_CAPABILITY_H264_DEC |
                   CEDRUS_CAPABILITY_H265_DEC |
                   CEDRUS_CAPABILITY_H265_10_DEC |
                   CEDRUS_CAPABILITY_VP8_DEC,
    .mod_rate = 600000000,
};
```

**Notable Omissions:**
- No `CEDRUS_CAPABILITY_VP9_DEC` 
- No `CEDRUS_CAPABILITY_AV1_DEC`
- No AV1-related code in cedrus driver

### Device Tree Video Engine Configuration

**Current H713 Device Tree** (`sun50i-h713-hy300.dts`):
```dts
ve_sram: sram-section@0 {
    compatible = "allwinner,sun50i-h6-sram-c1",
                 "allwinner,sun4i-a10-sram-c1";
    reg = <0x000000 0x200000>;
};

decd_reserved: decoder@4d941000 {
    reg = <0x0 0x4d941000 0x0 0x20000>;
    no-map;
};
```

**Missing Video Engine Node:**
Our current device tree lacks the video engine node that would enable hardware video decoding. Required configuration:
```dts
video-codec@1c0e000 {
    compatible = "allwinner,sun50i-h6-video-engine";
    reg = <0x01c0e000 0x2000>;
    clocks = <&ccu CLK_BUS_VE>, <&ccu CLK_VE>, <&ccu CLK_MBUS_VE>;
    clock-names = "ahb", "mod", "ram";
    resets = <&ccu RST_BUS_VE>;
    interrupts = <GIC_SPI 89 IRQ_TYPE_LEVEL_HIGH>;
    allwinner,sram = <&ve_sram 1>;
};
```

## Implications for HY300 Media Center

### Content Strategy Recommendations
1. **Prioritize H.265/HEVC content** - Full hardware acceleration available
2. **Accept H.264/AVC content** - Excellent hardware acceleration  
3. **Avoid AV1 content** - or accept CPU-limited performance
4. **Transcode AV1 to H.265** - For optimal playback performance

### Device Tree Integration Requirements
To enable hardware video decoding, add to device tree:

```dts
&ve {
    compatible = "allwinner,sun50i-h6-video-engine";
    reg = <0x01c0e000 0x2000>;
    clocks = <&ccu CLK_BUS_VE>, <&ccu CLK_VE>, <&ccu CLK_MBUS_VE>;
    clock-names = "ahb", "mod", "ram";
    resets = <&ccu RST_BUS_VE>;
    interrupts = <GIC_SPI 89 IRQ_TYPE_LEVEL_HIGH>;
    allwinner,sram = <&ve_sram 1>;
    status = "okay";
};
```

## Quality Validation

### Success Criteria Met
- ✅ Comprehensive analysis of H713 video capabilities
- ✅ Clear documentation of AV1 hardware decode absence  
- ✅ Concrete recommendations for optimal video configuration
- ✅ Device tree integration requirements documented

## Next Actions

1. **Complete device tree video engine integration** 
2. **Test hardware video acceleration** with H.264/H.265 samples
3. **Configure Kodi media center** for optimal hardware acceleration
4. **Update hardware status matrix** with video acceleration capabilities
