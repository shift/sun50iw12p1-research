# Task 021: Hardware Testing and Validation

## Task Metadata
- **Task ID**: 021
- **Type**: Hardware Testing
- **Priority**: Low
- **Phase**: VIII
- **Status**: pending
- **Created**: 2025-09-18
- **Dependencies**: Task 020 (Motor driver integration)

## Objective
Perform comprehensive hardware testing and validation of the complete keystone correction system using FEL mode and safe testing procedures.

## Context
All software components are ready for hardware testing:
- ✅ MIPS co-processor driver (Task 015-018)
- ✅ Keystone sysfs interface (Task 019) 
- ✅ Motor driver integration (Task 020)
- ✅ Device tree configuration complete
- ✅ Safe testing methodology established

## Testing Strategy

### Phase 1: Safe Environment Setup
**Objective**: Establish safe testing environment with recovery capabilities

**Prerequisites**:
- FEL mode recovery confirmed working
- Serial console access established (UART0: 115200n8)
- Factory firmware backup verified
- U-Boot with device tree support ready

**Safety Protocols**:
1. **FEL Recovery Ready**: Sunxi-fel tools tested and working
2. **Serial Console**: Debug access for kernel messages
3. **Non-destructive**: No modification to factory partitions
4. **Incremental**: Test each component independently

### Phase 2: Basic Boot and Driver Loading
**Objective**: Validate basic system boot with custom kernel and drivers

**Test Sequence**:
1. **Kernel Boot via FEL**
   ```bash
   # Load custom kernel with device tree
   sunxi-fel -p uboot u-boot-sunxi-with-spl.bin
   sunxi-fel -p write 0x40080000 sun50i-h713-hy300.dtb
   sunxi-fel -p write 0x40200000 Image
   sunxi-fel -p exe 0x40008000
   ```

2. **Driver Loading Verification**
   ```bash
   # Check kernel modules loaded
   lsmod | grep -E "(sunxi_mipsloader|hy300_keystone)"
   
   # Verify device nodes created
   ls -la /dev/mipsloader
   ls -la /sys/class/platform/keystone-motor/
   ls -la /sys/class/mips/mipsloader/
   ```

3. **Memory Mapping Validation**
   ```bash
   # Check memory reservations
   cat /proc/iomem | grep -i mips
   dmesg | grep -i "mips\|keystone"
   ```

**Success Criteria**:
- [ ] Kernel boots without panic
- [ ] All drivers load successfully
- [ ] Device nodes created properly
- [ ] Memory regions reserved correctly
- [ ] No critical errors in dmesg

### Phase 3: Motor Control System Testing
**Objective**: Validate physical motor control functionality

**Test Sequence**:
1. **Motor Initialization**
   ```bash
   # Check motor driver status
   cat /sys/class/platform/keystone-motor/homed
   cat /sys/class/platform/keystone-motor/position
   cat /sys/class/platform/keystone-motor/max_position
   ```

2. **Homing Operation**
   ```bash
   # Initiate homing sequence
   echo 1 > /sys/class/platform/keystone-motor/home
   
   # Monitor progress
   watch cat /sys/class/platform/keystone-motor/position
   ```

3. **Position Control Testing**
   ```bash
   # Test incremental movements
   echo 10 > /sys/class/platform/keystone-motor/position
   echo 50 > /sys/class/platform/keystone-motor/position
   echo 100 > /sys/class/platform/keystone-motor/position
   echo 0 > /sys/class/platform/keystone-motor/position
   ```

4. **Limit Switch Validation**
   ```bash
   # Test limit switch detection
   echo -500 > /sys/class/platform/keystone-motor/position  # Force limit trigger
   dmesg | tail -10  # Check limit switch interrupt
   ```

**Success Criteria**:
- [ ] Motor responds to position commands
- [ ] Homing operation completes successfully
- [ ] Position feedback accurate
- [ ] Limit switch interrupt functional
- [ ] No mechanical binding or errors

### Phase 4: MIPS Co-processor Testing
**Objective**: Validate MIPS firmware loading and communication

**Test Sequence**:
1. **Firmware Loading**
   ```bash
   # Load MIPS firmware
   echo 1 > /sys/class/mips/mipsloader/mips_enable
   
   # Check firmware status
   cat /sys/class/mips/mipsloader/mips_status
   cat /sys/class/mips/mipsloader/firmware_version
   ```

2. **Register Communication**
   ```bash
   # Test register access (via kernel debug interface)
   echo "0x3061000" > /sys/kernel/debug/mips/register_dump
   cat /sys/kernel/debug/mips/register_dump
   ```

3. **Memory Region Validation**
   ```bash
   # Check memory layout
   cat /proc/iomem | grep mips
   dmesg | grep "MIPS memory"
   ```

**Success Criteria**:
- [ ] Firmware loads without errors
- [ ] MIPS processor starts successfully
- [ ] Register communication functional
- [ ] Memory regions properly mapped
- [ ] No memory corruption detected

### Phase 5: Keystone Correction Integration
**Objective**: Test complete 4-corner keystone correction system

**Test Sequence**:
1. **Basic Parameter Setting**
   ```bash
   # Test parameter interface
   echo "tl_x=0,tl_y=0,tr_x=0,tr_y=0,bl_x=0,bl_y=0,br_x=0,br_y=0" > /sys/class/mips/mipsloader/panelparam
   cat /sys/class/mips/mipsloader/panelparam
   ```

2. **User Correction Testing**
   ```bash
   # Apply user's specific correction
   echo "tl_x=-1,tl_y=-5,tr_x=-6,tr_y=-14,bl_x=-7,bl_y=-13,br_x=0,br_y=-5" > /sys/class/mips/mipsloader/panelparam
   
   # Verify parameters applied
   cat /sys/class/mips/mipsloader/panelparam
   dmesg | tail -5
   ```

3. **Motor + Digital Correction Integration**
   ```bash
   # Set motor baseline position
   echo 300 > /sys/class/platform/keystone-motor/position
   
   # Apply digital correction
   echo "tl_x=-1,tl_y=-5,tr_x=-6,tr_y=-14,bl_x=-7,bl_y=-13,br_x=0,br_y=-5" > /sys/class/mips/mipsloader/panelparam
   ```

**Success Criteria**:
- [ ] Keystone parameters accept user input
- [ ] Parameter validation working
- [ ] MIPS processor receives parameters
- [ ] Visual correction observable (if display active)
- [ ] Motor and digital correction coordinated

### Phase 6: Display Output Validation
**Objective**: Validate display pipeline and visual output

**Test Sequence**:
1. **Display Pipeline Status**
   ```bash
   # Check display subsystem
   cat /sys/class/drm/card0/device/status
   dmesg | grep -i "display\|gpu\|drm"
   ```

2. **Frame Buffer Testing**
   ```bash
   # Test basic frame buffer
   cat /dev/urandom > /dev/fb0  # Fill with test pattern
   ```

3. **Visual Keystone Verification**
   - Project test pattern
   - Verify keystone correction applied
   - Compare with factory correction

**Success Criteria**:
- [ ] Display pipeline functional
- [ ] Frame buffer accessible
- [ ] Visual keystone correction observable
- [ ] Correction matches expected geometry

## Test Documentation

### Test Log Format
```
Test ID: [Phase].[Test].[Step]
Timestamp: [ISO 8601]
Description: [Test description]
Expected: [Expected result]
Actual: [Actual result]
Status: [PASS/FAIL/SKIP]
Notes: [Additional observations]
```

### Failure Analysis Protocol
1. **Capture State**: dmesg, /proc/iomem, lsmod output
2. **Identify Root Cause**: Hardware vs software issue
3. **Document Workaround**: If applicable
4. **Plan Fix**: Implementation strategy
5. **Safety Check**: Ensure device not damaged

## Hardware Requirements

### Minimum Test Setup
- HY300 device with FEL mode access
- USB cable for FEL communication
- Serial console adapter (optional but recommended)
- Host computer with sunxi-tools

### Optional Advanced Setup
- Logic analyzer for GPIO signal verification
- Oscilloscope for timing validation
- External display for visual verification
- Temperature monitoring for thermal testing

## Risk Mitigation

### Hardware Protection
- FEL mode recovery always available
- No irreversible modifications
- Factory firmware backup maintained
- Power cycle recovery option

### Software Protection
- Comprehensive error handling
- Safe register access patterns
- Memory corruption detection
- Graceful failure modes

## Success Metrics

### System Level
- Complete boot cycle successful
- All drivers loaded and functional
- Hardware control operational
- Recovery mechanisms working

### Feature Level
- Motor positioning accurate (±1 step)
- Keystone parameters applied correctly
- Display output functional
- User interface responsive

### Quality Level
- No kernel panics or crashes
- No memory leaks detected
- Error handling robust
- Performance acceptable

## Acceptance Criteria
- [ ] Safe testing environment established
- [ ] Basic system boot successful with custom kernel
- [ ] Motor control system fully functional
- [ ] MIPS co-processor operational
- [ ] Keystone correction interface working
- [ ] Complete integration validated
- [ ] Visual output confirmed (if possible)
- [ ] Recovery procedures verified
- [ ] Test documentation complete

## Notes
- Testing must be incremental and safe
- Each phase builds on previous success
- Failure at any phase requires investigation before proceeding
- Visual confirmation of keystone correction is ultimate validation
- All testing must maintain device recovery capability