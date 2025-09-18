# Task 018: Firmware Loading System Implementation

## Task Metadata
- **Task ID**: 018
- **Type**: Firmware Loading System
- **Priority**: High
- **Phase**: VII
- **Status**: pending
- **Created**: 2025-09-18
- **Dependencies**: Task 017 (Device tree integration)

## Objective
Implement complete firmware loading system for the MIPS co-processor including display.bin loading, validation, and memory management.

## Context
We have the `display.bin` firmware extracted from factory Android system (1.25MB, CRC32: 0xe0376895). Need to implement loading system in the MIPS driver.

## Implementation Plan

### 1. Firmware Loading Framework Integration
Integrate with Linux firmware loading framework:
```c
// In sunxi-mipsloader.c
static int mips_load_firmware(struct mips_device *mips)
{
    const struct firmware *fw;
    int ret;
    
    ret = request_firmware(&fw, "display.bin", mips->dev);
    if (ret) {
        dev_err(mips->dev, "Failed to load firmware: %d\n", ret);
        return ret;
    }
    
    // Validate firmware
    if (fw->size != EXPECTED_FIRMWARE_SIZE) {
        dev_err(mips->dev, "Invalid firmware size: %zu\n", fw->size);
        release_firmware(fw);
        return -EINVAL;
    }
    
    // Copy to MIPS memory region
    memcpy(mips->firmware_addr, fw->data, fw->size);
    
    release_firmware(fw);
    return 0;
}
```

### 2. Memory Layout Implementation
Implement factory-analyzed memory layout:
```c
#define MIPS_BOOT_CODE_ADDR    0x4b100000  // 4KB
#define MIPS_FIRMWARE_ADDR     0x4b101000  // 12MB  
#define MIPS_DEBUG_ADDR        0x4bd01000  // 1MB
#define MIPS_CONFIG_ADDR       0x4be01000  // 256KB
#define MIPS_DATABASE_ADDR     0x4be41000  // 1MB
#define MIPS_FRAMEBUFFER_ADDR  0x4bf41000  // 26MB
```

### 3. Firmware Validation
- CRC32 validation against factory checksum (0xe0376895)
- Size validation (1.25MB expected)
- Header validation for format verification

### 4. IOCTL Interface Implementation
```c
#define MIPS_IOC_LOAD_FIRMWARE    _IO('M', 1)
#define MIPS_IOC_START_MIPS       _IO('M', 2)
#define MIPS_IOC_STOP_MIPS        _IO('M', 3)
#define MIPS_IOC_RESET_MIPS       _IO('M', 4)
```

### 5. Firmware Installation
Create firmware installation in `/lib/firmware/`:
- Copy `display.bin` to `/lib/firmware/display.bin`
- Set proper permissions and ownership

## Acceptance Criteria
- [ ] Firmware loading framework integrated
- [ ] Memory layout properly implemented
- [ ] CRC32 validation working
- [ ] IOCTL interface functional
- [ ] Firmware file properly installed
- [ ] Error handling comprehensive
- [ ] Memory management safe

## Files to Modify
- `drivers/misc/sunxi-mipsloader.c` - Add firmware loading functions
- `firmware/Makefile` - Add display.bin installation
- `include/linux/sunxi-mipsloader.h` - Add IOCTL definitions

## Testing Plan
1. Firmware loading validation
2. Memory mapping verification
3. CRC32 checksum validation
4. IOCTL interface testing
5. Error condition handling

## Implementation Steps
1. **Framework Integration**: Add request_firmware() calls
2. **Memory Management**: Implement safe memory copying
3. **Validation System**: Add CRC32 and size checks
4. **IOCTL Interface**: Create device control interface
5. **Error Handling**: Comprehensive error management
6. **Documentation**: User space interface documentation

## Notes
- Must use Linux firmware loading framework for security
- Memory operations must be safe and validated
- Error handling must prevent system crashes
- Factory CRC32: 0xe0376895 for validation
- Firmware size: 1.25MB (1,310,720 bytes)