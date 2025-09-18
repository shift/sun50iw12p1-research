# Task 017: Device Tree MIPS Integration

## Task Metadata
- **Task ID**: 017
- **Type**: Device Tree Integration
- **Priority**: High
- **Phase**: VII
- **Status**: pending
- **Created**: 2025-09-18
- **Dependencies**: Task 015 (MIPS driver development)

## Objective
Integrate the MIPS co-processor driver into the mainline device tree and ensure proper memory reservations and register mappings.

## Context
The MIPS driver (`drivers/misc/sunxi-mipsloader.c`) is compiled and ready, but needs proper device tree integration to bind with hardware and reserve memory regions.

## Implementation Plan

### 1. Device Tree Node Creation
Add MIPS loader node to `sun50i-h713-hy300.dts`:
```dts
mipsloader: mipsloader@3061000 {
    compatible = "allwinner,sunxi-mipsloader";
    reg = <0x0 0x03061000 0x0 0x1000>;
    memory-region = <&mips_reserved>;
    status = "okay";
};
```

### 2. Memory Reservation
Update reserved memory section:
```dts
mips_reserved: mips@4b100000 {
    reg = <0x0 0x4b100000 0x0 0x2800000>; /* 40MB */
    no-map;
};
```

### 3. Device Tree Binding Documentation
Create `Documentation/devicetree/bindings/misc/allwinner,sunxi-mipsloader.yaml`

### 4. Kernel Configuration
Add CONFIG options for MIPS loader in kernel config

## Acceptance Criteria
- [ ] MIPS node properly defined in device tree
- [ ] Memory regions correctly reserved
- [ ] Device tree binding documentation created
- [ ] Device tree compiles without errors
- [ ] Driver binds to device tree node
- [ ] Memory mapping functional

## Files to Modify
- `sun50i-h713-hy300.dts` - Add MIPS node and memory reservation
- `Documentation/devicetree/bindings/misc/allwinner,sunxi-mipsloader.yaml` - DT bindings
- `arch/arm64/configs/defconfig` - Kernel configuration

## Testing Plan
1. Device tree compilation verification
2. Memory reservation validation
3. Driver binding confirmation
4. Register access testing

## Notes
- Must maintain compatibility with existing hardware configuration
- Memory regions must not conflict with other reservations
- Register mapping must match factory analysis (0x3061000)