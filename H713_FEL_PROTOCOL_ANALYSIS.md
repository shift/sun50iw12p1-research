# H713 FEL Protocol Analysis

## CRITICAL FINDING: BROM Crashes on Device Access

### Problem Summary
**H713 BROM has a firmware bug that causes it to crash immediately when ANY program attempts to open the USB device.** This is NOT a protocol issue - the device crashes before any FEL commands can be sent.

### Device Behavior
- ✅ Device enumerates correctly on USB (VID/PID: 1f3a:efe8, bcdDevice: 2.b3)
- ❌ **Crashes immediately when libusb attempts to open the device**
- ⚠️ Enters continuous reset loop: crash → reset → re-enumerate (every ~300ms)
- ⚠️ Crash occurs with ALL tools: custom `sunxi-fel-h713-v3`, stock `sunxi-fel`, minimal libusb test programs
- ⚠️ Even `lsusb -v` descriptor read causes device crash
- ✅ Without interaction, device remains stable in FEL mode

### Testing Evidence
1. **Minimal libusb test** (only calls `libusb_open_device_with_vid_pid()`): Device crashes
2. **Stock upstream sunxi-fel**: Device crashes  
3. **Modified sunxi-fel-h713-v3**: Device crashes
4. **Result**: `errno=5 EIO` when attempting to open `/dev/bus/usb/001/XXX`

### Root Cause Assessment
**The H713 BROM crashes during USB device initialization**, likely during:
- Configuration descriptor read
- Interface claim operations
- Initial control transfers
- Some undocumented initialization sequence requirement

**This is a BROM firmware bug**, not a sunxi-tools implementation issue.

### Alternative Access Methods Required
Since FEL mode is currently inaccessible, hardware testing requires:
1. **Serial console (UART)** - Primary recommendation for U-Boot/Linux boot testing
2. **Android ADB** - Boot Android first, then trigger FEL mode from userspace
3. **Different USB host** - Try Windows or USB 2.0 hub (may have different driver behavior)
4. **Hardware investigation** - USB sniffer to capture exact crash sequence

---

## Secondary Finding: USB Response Protocol Difference

### Problem Summary (Superseded by BROM crash issue above)
H713 BROM uses **different USB response length** than other Allwinner SoCs (H616, H6, etc.), causing FEL operations to timeout.

### USB Protocol Trace Analysis

Captured with `LIBUSB_DEBUG=4`, attempting `write 0x20000 /dev/urandom`:

```
Transfer 1 (32 bytes): AWUC + FEL write command
  ✅ Status=0, transferred=32 bytes
  
Transfer 2 (16 bytes): First AWUS response
  ✅ Status=0, transferred=16 bytes
  
Transfer 3 (16 bytes requested): Second AWUS response
  ⚠️ Status=0, but only transferred=13 bytes (expected 16)
  
Transfer 4 (3 bytes): Attempting to read remaining bytes
  ❌ TIMEOUT - BROM never sends them
```

### Root Cause

**H713 BROM sends exactly 13 bytes for `AWUS` response**, not 16 bytes like we assumed.

Our code (from previous fix attempt):
```c
static void aw_read_usb_response(feldev_handle *dev)
{
	char buf[16];  /* H713 requires 16-byte aligned responses */
	usb_bulk_recv(dev->usb->handle, dev->usb->endpoint_in,
		      buf, sizeof(buf));  // Requests 16 bytes
	assert(strcmp(buf, "AWUS") == 0);
}
```

USB bulk transfer behavior:
- Requests 16 bytes
- H713 sends 13 bytes
- `usb_bulk_recv` blocks waiting for remaining 3 bytes
- 10-second timeout expires
- `LIBUSB_ERROR_TIMEOUT (-7)`

### The Fix (v3)

**Change response buffer from 16 to 13 bytes:**

```diff
 static void aw_read_usb_response(feldev_handle *dev)
 {
-	char buf[16];  /* H713 requires 16-byte aligned responses */
+	char buf[13];  /* H713 sends exactly 13 bytes */
 	usb_bulk_recv(dev->usb->handle, dev->usb->endpoint_in,
 		      buf, sizeof(buf));
-	assert(strcmp(buf, "AWUS") == 0);
+	assert(strncmp(buf, "AWUS", 4) == 0);  // Only check first 4 bytes
 }
```

### Implementation Status

- ✅ **Patch created**: `build/sunxi-tools/response_fix.patch`
- ✅ **Compiled**: `sunxi-fel-h713-v3` (77KB)
- ⏸️ **Testing blocked**: Device in BROM crash loop from previous test

### BROM Crash Loop Issue

**Critical**: After failed FEL operations, H713 BROM enters a **crash/reset cycle**:
- Every ~6 seconds: USB reset → disconnect → re-enumerate
- `dmesg` shows constant: "USB disconnect, device number X" → "new full-speed USB device number X+1"
- This persists until **physical power cycle** (unplug/replug device or power)
- Software reset via `sunxi-fel` does NOT clear this state

This crash loop was caused by the **16-byte response timeout** - BROM crashes when USB protocol gets out of sync.

### Testing Requirements

**Before testing v3:**
1. **Physically power cycle the H713 device** (unplug USB or device power)
2. Wait for clean USB enumeration (no reset loop in `dmesg`)
3. Verify with: `sudo ./sunxi-fel-h713-v3 version`
4. If version succeeds, test: `sudo ./sunxi-fel-h713-v3 write 0x20000 /tmp/test16`

### Expected Results

With the 13-byte response fix:
- ✅ `sunxi-fel version` should succeed
- ✅ `sunxi-fel write` should not timeout
- ✅ Device should remain stable (no reset loop)

### Next Steps After Testing

If v3 works:
1. Test memory read: `sunxi-fel read 0x7e00 256 /tmp/test`
2. Test SPL upload: `sunxi-fel spl u-boot-sunxi-with-spl.bin`
3. Document complete H713 FEL protocol specification
4. Submit findings to sunxi-tools upstream

### Files Changed

- `build/sunxi-tools/fel_lib.c` (line 156): Buffer size 16→13
- `build/sunxi-tools/fel_lib.c` (line 159): `strcmp`→`strncmp` for partial comparison

### Build Command

```bash
cd build/sunxi-tools
nix develop /home/shift/code/android_projector -c bash -c \
  "make sunxi-fel"
cp sunxi-fel ../../sunxi-fel-h713-v3
```

### USB Response Format Analysis

The 13-byte `AWUS` response likely contains:
- Bytes 0-3: "AWUS" signature (4 bytes)
- Bytes 4-12: Unknown padding/status data (9 bytes)
- **Total: 13 bytes**

This differs from standard Allwinner FEL which expects:
- Bytes 0-3: "AWUS" (4 bytes)
- Bytes 4-15: Padding (12 bytes)
- **Total: 16 bytes**

The 3-byte difference causes the protocol desynchronization and subsequent BROM crashes.

### Comparison with Other SoCs

| SoC Family | AWUS Response Length | Notes |
|------------|---------------------|-------|
| H6, H616, A64, etc. | 16 bytes (assumed) | Standard implementation |
| **H713** | **13 bytes** | **Unique protocol variant** |

### Hypothesis: Why 13 Bytes?

Possible reasons for H713's different response length:
1. **BROM optimization**: H713 BROM may have been optimized/refactored
2. **Protocol version difference**: H713 may use newer/different FEL protocol revision
3. **Hardware constraint**: USB FIFO or DMA limitations in H713
4. **Intentional change**: Allwinner may have standardized on 13 bytes for newer SoCs

### Testing Log

**Previous Tests (v1, v2 with 16-byte buffer):**
- ❌ `sunxi-fel version` - Timeout
- ❌ `sunxi-fel write 0x20000` - Timeout
- ❌ `sunxi-fel write 0x104000` - Timeout  
- ❌ `sunxi-fel read 0x20000` - Timeout
- Result: Device enters reset loop, requires power cycle

**v3 Testing (13-byte buffer): PENDING**
- ⏸️ Awaiting physical power cycle to clear BROM crash state
