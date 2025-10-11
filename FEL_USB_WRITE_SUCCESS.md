# H713 FEL USB Write Success - Problem Solved

## Problem Summary
When attempting to write 32KB files (like boot0.bin) using FEL mode, the operation would timeout with:
```
ERROR -7: Operation timed out
```

## Root Cause Analysis
The issue was **NOT** with the H713 BROM or USB transfers themselves. The problem was introduced during debugging when the `AW_USB_MAX_BULK_SEND` constant was reduced from 512KB to smaller values (16KB, then 4KB) in an attempt to find optimal chunk sizes.

### What Went Wrong
1. Reduced chunk size from 512KB to 16KB - writes started timing out
2. Further reduced to 4KB - even small writes and `version` command hung
3. The small chunk size apparently caused the H713 BROM to not respond properly

### What Actually Works
**The original sunxi-tools configuration works fine with H713** when combined with our receive buffer workaround:
- **USB_TIMEOUT**: 20 seconds (increased from 10s for safety)
- **AW_USB_MAX_BULK_SEND**: 512 KB (original value - DO NOT REDUCE)
- **Receive buffer workaround**: 64-byte buffer for ≤8 byte reads (H713 returns more data than requested for status messages)

## Working Configuration

### Changes to `fel_lib.c`:

1. **Timeout increase** (line 34):
```c
#define USB_TIMEOUT 20000 /* 20 seconds (was 10000) */
```

2. **Receive buffer workaround** (usb_bulk_recv function, ~line 102):
```c
static void usb_bulk_recv(libusb_device_handle *usb, int ep, void *data,
			  int length)
{
	int rc, recv;
	
	/* H713 BROM workaround: allocate larger buffer for small reads */
	unsigned char temp_buffer[64];
	unsigned char *recv_ptr = data;
	int buffer_size = length;
	
	if (length <= 8) {
		recv_ptr = temp_buffer;
		buffer_size = 64;  /* H713 sends more than requested for status */
	}
	
	while (length > 0) {
		rc = libusb_bulk_transfer(usb, ep, recv_ptr, buffer_size,
					  &recv, USB_TIMEOUT);
		if (rc != 0)
			usb_error(rc, "usb_bulk_recv()", 2);
		
		/* For small reads, copy only requested amount and exit */
		if (recv_ptr == temp_buffer) {
			int copy_len = (recv < length) ? recv : length;
			memcpy(data, temp_buffer, copy_len);
			length = 0;  /* Done */
		} else {
			length -= recv;
			data += recv;
		}
	}
}
```

## Verified Working Operations

### File Writes ✅
```bash
# 4KB test file
sudo ./sunxi-fel-h713-v5 write 0x104000 /tmp/test-4k.bin

# 32KB test file
sudo ./sunxi-fel-h713-v5 write 0x104000 /tmp/test-32k.bin

# Factory boot0.bin (32KB)
sudo ./sunxi-fel-h713-v5 write 0x104000 firmware/boot0.bin
```

### U-Boot Upload ✅
```bash
# Upload 732KB U-Boot binary
sudo ./sunxi-fel-h713-v5 uboot u-boot-sunxi-with-spl.bin
```

All operations complete successfully without timeout, and the device remains connected in FEL mode (Device ID `1f3a:efe8`).

## Key Insights

1. **H713 BROM is compatible with standard FEL protocol** - No special modifications needed beyond the receive buffer workaround

2. **Chunk size matters** - The 512KB chunk size is optimal; smaller chunks cause communication issues

3. **Status read quirk** - H713 returns 13 bytes (or more) for status reads even when 8 bytes are requested; our 64-byte buffer workaround handles this cleanly

4. **No stdout with sudo** - FEL tool produces no visible output when run with sudo, but operations complete successfully (check exit codes and device status instead)

## Next Steps

Now that write operations are confirmed working, we can proceed with:

1. ✅ Upload U-Boot to correct load address
2. 🔄 Execute U-Boot and capture boot output (serial console recommended)
3. 🔄 Test mainline kernel boot
4. 🔄 Full hardware validation with mainline device tree

## Build Instructions

To build the working H713 FEL tool:

```bash
cd build/sunxi-tools
git checkout fel_lib.c  # Start clean

# Apply H713 fixes
sed -i 's/#define USB_TIMEOUT\s\+10000/#define USB_TIMEOUT 20000/' fel_lib.c

# Apply receive buffer patch (see patch above)
patch -p1 < h713-recv-fix.patch

# Build
nix develop -c -- make CFLAGS='-O2 -g' tools

# Copy binary
cp sunxi-fel ../../sunxi-fel-h713-v5
```

## File Artifacts

- `sunxi-fel-h713-v5` - Working FEL tool with H713 fixes
- `build/sunxi-tools/apply-h713-fixes.sh` - Script to reapply fixes
- `build/sunxi-tools/h713-recv-fix.patch` - Receive buffer workaround patch

---

**Status**: FEL write operations WORKING ✅  
**Date**: 2025-10-11  
**Session**: Phase VIII Hardware Testing
