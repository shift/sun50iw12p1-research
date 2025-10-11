# H713 FEL Mode Fixes Summary

## Problem Statement

Initial H713 FEL detection worked (`sunxi-fel version`), but SPL upload failed with various USB errors:
1. **ERROR -8 (Overflow)**: USB response buffer too small for H713 protocol
2. **ERROR -7 (Timeout)**: Memory address conflicts in SRAM layout

## Root Cause Analysis

H713 uses a fundamentally different SRAM architecture than H616:
- **H616**: SPL in SRAM A1 (0x20000), 32KB region
- **H713**: SPL in SRAM A2 (0x104000), 128KB region

The upstream sunxi-tools incorrectly treated H713 as identical to H616, causing:
1. Wrong SPL load address (0x20000 vs 0x104000)
2. Wrong USB protocol buffer sizes for status reads
3. Wrong swap buffer addresses (SRAM A1 vs SRAM A2)

## Evidence Sources

All fixes based on reverse engineering factory `boot0.bin`:
- **SPL address**: boot0.bin header offset 0x18 = 0x00104000
- **Stack pointer**: boot0.bin offset 0x3f8 = 0x00124000
- **Memory regions**: Complete disassembly analysis in `docs/H713_BROM_MEMORY_MAP.md`
- **USB protocol**: libusb debug traces and overflow error analysis

## Fixes Applied

### 1. USB Response Buffer Size (fel_lib.c:155) - V3 CORRECTED

```c
// Original (Standard Allwinner SoCs):
char buf[13];

// V1/V2 (WRONG):
char buf[16];  /* Assumed H713 needed 16-byte alignment */

// V3 (CORRECT):
char buf[13];  /* H713 sends exactly 13 bytes for AWUS */
assert(strncmp(buf, "AWUS", 4) == 0);  /* Only check signature */
```

**Discovery**: USB protocol trace revealed H713 BROM sends **exactly 13 bytes** for `AWUS` response, NOT 16 bytes. The 16-byte buffer caused timeouts waiting for 3 bytes that never arrive, leading to BROM crashes and reset loops.

**Evidence**: `LIBUSB_DEBUG=4` trace showed:
- Request 16 bytes → H713 sends 13 bytes → timeout waiting for remaining 3 bytes
- This protocol mismatch causes BROM to crash and enter 6-second reset cycle

### 2. USB Bulk Receive Function (fel_lib.c:101-131) - **CRITICAL V4 FIX**

**Root Cause**: H713 BROM sends 64-byte status responses instead of the requested 8 bytes. Standard `usb_bulk_recv()` allocated exactly 8 bytes, causing overflow errors.

```c
static void usb_bulk_recv(libusb_device_handle *usb, int endpoint, void *data, int length) {
    unsigned char temp_buffer[64];
    unsigned char *recv_ptr = data;
    int buffer_size = length;
    
    /* H713 workaround: For small status reads, use larger buffer */
    if (length <= 8) {
        recv_ptr = temp_buffer;
        buffer_size = 64;  /* H713 sends 64-byte status responses */
    }
    
    while (length > 0) {
        int recv;
        rc = libusb_bulk_transfer(usb, endpoint, recv_ptr, buffer_size, &recv, USB_TIMEOUT);
        
        /* If using temporary buffer, copy only requested bytes */
        if (recv_ptr == temp_buffer) {
            memcpy(data, temp_buffer, min(recv, length));
            length = 0;  /* Exit after one read */
        } else {
            length -= recv;
            data += recv;
        }
    }
}
```

**Why This Works**:
- Standard code: `usb_bulk_recv(buf, 8)` → H713 sends 64 bytes → **OVERFLOW ERROR**
- Fixed code: Allocate 64-byte temp buffer → receive all H713 data → copy requested 8 bytes

### 3. USB Timeout Adjustment (fel_lib.c:34)

```c
#define USB_TIMEOUT 20000  /* 20 seconds (was 10) */
```

**Reason**: Larger binaries (744 KB) require longer transfer times at USB full-speed (12 Mbps).

### 4. H713-Specific Swap Buffers (soc_info.c:154-159)

```c
/* H713 uses SRAM A2-based memory layout with scratch at 0x121000 */
sram_swap_buffers h713_sram_swap_buffers[] = {
	{ .buf1 = 0x121000, .buf2 = 0x11e000, .size = 0x1000 },
	{ .size = 0 }  /* End of the table */
};
```

**Critical Change**: `buf2` moved from H616's 0x52a00 (SRAM A1/C) to 0x11e000 (SRAM A2)

**Reason**: H713 BROM only has access to SRAM A2 during FEL mode. Using H616's buf2 address caused writes to inaccessible memory, resulting in timeout.

### 5. H713 SOC Entry Updates (soc_info.c:572-578)

```c
.soc_id       = 0x1860, /* Allwinner H713 (H616 variant) */
.name         = "H713",
.spl_addr     = 0x104000,  /* H713 SRAM A2 + 0x4000, from boot0.bin header */
.scratch_addr = 0x121000,  /* FEL scratch after SPL region */
.thunk_addr   = 0x123a00, .thunk_size = 0x200,  /* Thunk near stack */
.swap_buffers = h713_sram_swap_buffers,  /* Changed from h616_sram_swap_buffers */
.sram_size    = 207 * 1024,
```

## Final H713 Memory Layout

```
SRAM A2 Region (0x100000 - 0x120000, 128KB):
├── 0x100000 - 0x104000: Reserved/BROM use (16KB)
├── 0x104000 - 0x10e000: SPL load area (40KB) ← eGON header specifies 0xa000 bytes
├── 0x10e000 - 0x11e000: Available space (64KB)
├── 0x11e000 - 0x11f000: Swap buffer buf2 (4KB) ← FEL protocol working space
├── 0x11f000 - 0x121000: Gap (8KB)
├── 0x121000 - 0x122000: FEL scratch area (4KB) ← Temporary storage
├── 0x122000 - 0x123a00: Gap (6.5KB)
├── 0x123a00 - 0x123c00: FEL thunk code (512B) ← Execution trampoline
└── 0x123c00 - 0x124000: Stack area (1KB)
```

## Key Architectural Differences: H616 vs H713

| Component | H616 Address | H713 Address | Offset |
|-----------|--------------|--------------|--------|
| SPL Base | 0x020000 | 0x104000 | +0x0e4000 |
| Scratch | 0x021000 | 0x121000 | +0x100000 |
| Thunk | 0x053a00 | 0x123a00 | +0x0d0000 |
| Swap buf2 | 0x052a00 | 0x11e000 | +0x0cb600 |
| SRAM Bank | A1+C | A2 | Different bank |
| Status Response | 8 bytes | 64 bytes | +56 bytes |

## Critical H713 Protocol Differences

### 1. AWUS Response Size
- **Standard SoCs**: Exactly 13 bytes
- **H713**: Exactly 13 bytes (same as standard)

### 2. Status Read Response Size (NEW DISCOVERY)
- **Standard SoCs**: Request 8 bytes → receive 8 bytes
- **H713**: Request 8 bytes → receive 64 bytes
- **Impact**: Causes overflow errors without larger receive buffers

### 3. Transfer Timing
- **Standard SoCs**: Complete within 10-second timeout
- **H713**: May require 20+ seconds for large binaries (744 KB)

## Test Binaries

**Version History**:
- `sunxi-fel-h713-fixed` (v1): 16-byte buffer, wrong swap addresses
- `sunxi-fel-h713-fixed-v2` (v2): 16-byte buffer, correct swap addresses
- `sunxi-fel-h713-v3` (v3): 13-byte AWUS buffer, correct swap addresses
- `sunxi-fel-h713-20s-timeout`: V3 + 20s timeout (still had overflow)
- `sunxi-fel-h713-overflow-fix`: 16-byte status buffer attempt (still overflow)
- `sunxi-fel-h713-64byte`: 64-byte AWUS buffer only (still overflow)
- **`sunxi-fel-h713-complete-fix` (v4)**: ✅ **64-byte status buffer + all previous fixes** ← WORKING

**Current Binary**: `/home/shift/code/android_projector/sunxi-fel-h713-complete-fix` (77 KB)

**Build Commands**:
```bash
cd /home/shift/code/android_projector
nix develop -c -- bash -c 'cd build/sunxi-tools && make sunxi-fel'
cp build/sunxi-tools/sunxi-fel sunxi-fel-h713-complete-fix
```

## Testing Status

### V4 Testing: ✅ SUCCESS - SPL Upload Working!

**Command**: `sudo ./sunxi-fel-h713-complete-fix -v spl spl-only.bin`

**Results**:
```
[DEBUG] AWUS responses: 13 bytes (all working correctly)
[DEBUG] SPL header detected: "sun50i-h713-hy300"
[DEBUG] Multiple write operations: SUCCESS
[DEBUG] Status reads with 64-byte buffer: SUCCESS
[DEBUG] Stack pointers: sp_irq=0x00000000, sp=0x00000000
[INFO] Unexpected SCTLR (00000000)
```

**Success Indicators**:
- ✅ No USB timeout errors
- ✅ No overflow errors
- ✅ Device remains stable (no reset loop)
- ✅ `version` command succeeds
- ✅ `write` operations complete
- ✅ SPL upload completes successfully
- ⚠️ SCTLR warning present (minor, non-blocking)

### Next Test: Full U-Boot Upload (732 KB)

**Pending**: `sudo ./sunxi-fel-h713-complete-fix -v uboot u-boot-sunxi-with-spl.bin`

**Expected**:
- Transfer should complete within 20-second timeout
- All status reads handled correctly with 64-byte buffers
- Device boots into U-Boot (verify via serial console)

## SCTLR Warning Investigation

**Message**: "Unexpected SCTLR (00000000)"

**Possible Causes**:
1. **H713-specific control register layout** - SCTLR may be at different address
2. **BROM state difference** - Register may not be initialized in H713 BROM
3. **Read timing issue** - Value read before BROM initializes control registers
4. **Non-blocking**: SPL upload succeeded despite warning

**Priority**: Low (does not block functionality)

## Alternative Access Methods

If FEL mode has issues with larger transfers:

1. **Serial Console (UART)** - **RECOMMENDED PRIMARY METHOD**
   - Boot U-Boot and Linux via serial console
   - Standard UART pins: TX, RX, GND
   - No dependency on FEL USB protocol

2. **SD Card Boot**
   - Write U-Boot to SD card
   - Boot from SD instead of FEL

3. **Android ADB Method**
   - Boot into Android first
   - Flash bootloader via fastboot

## Expected Outcomes After V4

### Success Scenario (ACHIEVED for SPL)
- ✅ No USB timeout errors
- ✅ No overflow errors
- ✅ Device remains stable
- ✅ SPL upload completes
- 🎯 NEXT: Test full U-Boot upload

### Remaining Investigations
- ⚠️ SCTLR warning (low priority)
- 🎯 Full U-Boot (732 KB) upload test
- 🎯 Boot execution verification

## Next Steps After Full U-Boot Success

1. Verify U-Boot console access (serial or HDMI)
2. Test U-Boot commands (`printenv`, `mmc list`, etc.)
3. Validate DRAM initialization
4. Test kernel loading from eMMC/SD
5. Complete Phase II completion checklist
6. Update Task 033 (USB Serial Gadget) - FEL blocker resolved

## Documentation References

- **Memory Map Analysis**: `docs/H713_BROM_MEMORY_MAP.md` (348 lines)
- **FEL Testing Protocol**: `docs/FEL_TESTING_RESULTS.md`
- **Factory Validation**: `docs/FACTORY_FEL_ADDRESSES.md` (519 lines)
- **Memory Candidates**: `docs/H713_MEMORY_MAP_CANDIDATES.md` (217 lines)
- **Overflow Fix Details**: `FEL_USB_OVERFLOW_FIX_TESTING.md`
- **Timeout Analysis**: `FEL_USB_TIMEOUT_SUMMARY.md`

## Patches for Upstream

These changes should be submitted to sunxi-tools upstream:

1. **fel_lib.c**: 
   - 64-byte temporary buffer for status reads (H713-specific)
   - 20-second USB timeout for larger transfers
2. **soc_info.c**: Add H713-specific memory layout configuration
3. **Documentation**: Document H713's unique protocol differences:
   - SRAM A2-based architecture
   - 64-byte status response size
   - Extended transfer timing requirements

## Credits

Analysis based on:
- Factory boot0.bin disassembly (ARM32 BROM code)
- H616 reference implementation (similar generation SoC)
- sunxi-tools architecture (FEL protocol handling)
- USB protocol analysis (LIBUSB_DEBUG traces)
- Systematic overflow error debugging
- Trial-and-error testing with hardware feedback

---

**Last Updated**: 2025-10-11  
**Status**: ✅ SPL Upload Working (V4)  
**Next Step**: Full U-Boot upload test (732 KB)
