# H713 FEL USB Timeout Investigation and Fix

## Problem Description

H713 BROM has **extremely slow USB bulk transfers** in FEL mode, causing timeouts with the default sunxi-tools configuration.

### Symptoms
- Small reads (status checks) work fine with H713-specific workaround
- **Writes fail with timeout** even for modest sizes (32KB)
- Error: `usb_bulk_send() ERROR -7: Operation timed out`
- Default 20-second timeout is insufficient

### Testing Results

**With sunxi-fel-h713-v5 (20s timeout, 512KB chunks):**
```bash
$ sudo ./sunxi-fel-h713-v5 write 0x104000 /tmp/test-32768.bin
usb_bulk_send() ERROR -7: Operation timed out
❌ FAILED
```

**Test File:** 32KB transfer
**Timeout:** 20 seconds (insufficient for H713's slow USB)

## Root Cause Analysis

1. **H713 BROM USB transfer speed** is much slower than other Allwinner SoCs
2. **Original sunxi-tools assumptions:**
   - USB_TIMEOUT: 20 seconds (comment incorrectly says "10 seconds")
   - Expected transfer speed: ~64 KiB/sec
   - Max chunk size: 512 KiB
   
3. **H713 reality:**
   - 32KB transfer exceeds 20-second timeout
   - Estimated speed: < 1.6 KB/sec (32KB / 20s)
   - **~40x slower** than expected

## Solution: Version 6 Fix

### Changes Applied

**File:** `build/sunxi-tools/fel_lib.c`

1. **Increased timeout:** 20s → 120s
   ```c
   #define USB_TIMEOUT 120000 /* 120 seconds */
   ```

2. **Reduced max chunk size:** 512KB → 16KB
   ```c
   static const int AW_USB_MAX_BULK_SEND = 16 * 1024;
   ```

3. **Reduced progress chunk:** 128KB → 4KB
   ```c
   static const int AW_USB_PROGRESS_CHUNK = 4 * 1024;
   ```

### Rationale

- **120s timeout:** Provides 6x safety margin for slow transfers
- **16KB chunks:** Breaks large transfers into manageable pieces
  - Each 16KB chunk has 120s to complete
  - Better progress reporting
  - Reduces risk of single large timeout
- **4KB progress chunks:** Gives granular feedback during writes

### Binary Versions

| Version | Timeout | Max Chunk | Status |
|---------|---------|-----------|--------|
| v5 | 20s | 512KB | ❌ Times out on 32KB writes |
| v6-slow-usb | 120s | 16KB | ✅ Should handle slow H713 USB |

## Testing Instructions

### Test Script
```bash
./test-v6-slow-usb.sh [size_in_bytes]
```

**Default:** Tests 32KB write (boot0.bin size)

### Expected Behavior

With the new timeout and chunk size:
- Each 16KB chunk has up to 120s to complete
- 32KB file = 2 chunks × 120s = up to 240s maximum
- Progress updates every 4KB when using `-p` flag

### Verification Commands

```bash
# Put device in FEL mode first

# Test 32KB (boot0.bin size)
sudo ./test-v6-slow-usb.sh 32768

# Test with progress reporting
sudo ./sunxi-fel-h713-v6-slow-usb -p write 0x104000 firmware/boot0.bin

# Time the transfer to measure actual speed
time sudo ./sunxi-fel-h713-v6-slow-usb write 0x104000 firmware/boot0.bin
```

## H713-Specific USB Quirks Summary

### Receive (Already Fixed in v5)
- **Issue:** H713 BROM sends more data than requested for status reads
- **Fix:** Allocate 64-byte buffer for reads ≤8 bytes
- **Status:** ✅ Working

### Send (Fixed in v6)
- **Issue:** Extremely slow bulk transfer speed causes timeouts
- **Fix:** Increase timeout 6x, reduce chunk size 32x
- **Status:** 🧪 Ready for testing

## Memory Map Validation

The timeout investigation also validated H713 memory addresses:

```c
// From build/sunxi-tools/soc_info.c - H713 configuration
.spl_addr = 0x104000,      // ✅ Correct (SRAM A2 + 0x4000)
.scratch_addr = 0x121000,  // ✅ Correct
.thunk_addr = 0x123a00,    // ✅ Correct
.swap_buffers = h713_sram_swap_buffers  // ✅ Correct
```

These addresses were extracted from factory `boot0.bin` analysis and are verified correct.

## Next Steps

1. **Test v6 binary** with device in FEL mode
2. **Measure actual transfer time** to understand real USB speed
3. **If still failing:** May need even longer timeout or smaller chunks
4. **If working:** Can optimize chunk size for best performance

## Files Modified

- `build/sunxi-tools/fel_lib.c` - USB timeout and chunk size changes
- `sunxi-fel-h713-v6-slow-usb` - New binary with fixes
- `test-v6-slow-usb.sh` - Test script for v6 binary

## Related Documentation

- `FEL_USB_TIMEOUT_INVESTIGATION.md` - Initial timeout analysis
- `H713_FEL_PROTOCOL_ANALYSIS.md` - Protocol-level investigation
- `USING_H713_FEL_MODE.md` - General FEL mode usage
