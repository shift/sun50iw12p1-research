# H713 FEL USB Timeout Fix - Summary

## Issue Discovered
H713 BROM has **extremely slow USB bulk transfers** (~40x slower than expected), causing timeout errors with default sunxi-tools configuration.

## Versions

### sunxi-fel-h713-v5 (Previous)
- ❌ USB_TIMEOUT: 20 seconds
- ❌ Max chunk: 512KB
- ❌ **Result:** Times out on 32KB writes

### sunxi-fel-h713-v6-slow-usb (Current)
- ✅ USB_TIMEOUT: 120 seconds (6x increase)
- ✅ Max chunk: 16KB (32x reduction)
- ✅ Progress chunk: 4KB
- ✅ **Result:** Should handle H713's slow USB

## Testing

**Put device in FEL mode, then run:**
```bash
sudo ./test-v6-slow-usb.sh 32768
```

Or test boot0.bin directly:
```bash
time sudo ./sunxi-fel-h713-v6-slow-usb write 0x104000 firmware/boot0.bin
```

## Technical Details

See complete analysis in: `FEL_USB_TIMEOUT_FIX_TESTING.md`

**Key findings:**
- H713 USB speed: < 1.6 KB/sec (vs expected 64 KB/sec)
- 32KB transfer needs > 20 seconds
- Solution: 120s timeout + 16KB chunks = reliable transfers

## Files
- `sunxi-fel-h713-v6-slow-usb` - Fixed binary
- `test-v6-slow-usb.sh` - Test script
- `FEL_USB_TIMEOUT_FIX_TESTING.md` - Complete documentation
