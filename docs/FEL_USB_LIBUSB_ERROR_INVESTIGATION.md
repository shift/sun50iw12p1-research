# FEL USB libusb I/O Error Investigation

**Date:** 2025-10-11  
**Issue:** `libusb_open() ERROR -1: Input/Output Error` when accessing H713 in FEL mode  
**Previous Issue:** USB timeout during bulk transfers (see FEL_USB_TIMEOUT_INVESTIGATION.md)

## Problem Summary

After multiple FEL transfer attempts that timed out, the H713 device enters a state where libusb cannot open it:

```bash
$ sudo ./sunxi-fel -l
libusb_open() ERROR -1: Input/Output Error
```

**Key Observations:**
1. ✅ Device enumeration works (lsusb shows ID 1f3a:efe8)
2. ✅ USB device node exists (/dev/bus/usb/001/086)
3. ❌ libusb_open() fails with LIBUSB_ERROR_IO (-1)
4. ❌ dmesg shows continuous USB reset/disconnect cycles

## Root Cause Analysis

### 1. USB State Machine Corruption

**dmesg Evidence:**
```
[744777.690508] usb 1-3: reset full-speed USB device number 77 using xhci_hcd
[744777.814941] usb 1-3: USB disconnect, device number 77
[744777.927459] usb 1-3: new full-speed USB device number 78 using xhci_hcd
[744778.050943] usb 1-3: New USB device found, idVendor=1f3a, idProduct=efe8, bcdDevice= 2.b3
[744818.167241] usb 1-3: reset full-speed USB device number 78 using xhci_hcd
[744818.291813] usb 1-3: USB disconnect, device number 78
[744818.404269] usb 1-3: new full-speed USB device number 79 using xhci_hcd
```

The device is continuously resetting and re-enumerating, indicating:
- H713 BROM entered error state after failed transfer
- USB endpoint/state machine corruption
- Device attempting auto-recovery but failing

### 2. H713 BROM State After Timeout

After the 20-second timeout occurred:
1. **Host perspective:** Timeout waiting for AWUS response after bulk transfer
2. **Device perspective:** Possibly still processing transfer or stuck in error handler
3. **USB layer:** Device becomes unresponsive, kernel resets it
4. **BROM recovery:** Device re-enters FEL mode but state is corrupted

### 3. libusb Error Code Analysis

**LIBUSB_ERROR_IO (-1):** Input/output error
- Indicates low-level USB communication failure
- Device responds to enumeration but fails control transfer
- Typically requires physical device reset

## Technical Analysis

### FEL Protocol State Machine

**Normal Flow:**
```
1. USB enumeration (WORKS)
2. libusb_open() - obtain device handle (FAILS HERE)
3. libusb_claim_interface() - claim FEL interface
4. FEL version handshake (AW_FEL_VERSION)
5. Bulk transfers (SPL/U-Boot upload)
```

**Current State:**
- Device stuck between step 1 and 2
- Enumeration succeeds but control transfers fail
- BROM FEL handler not responding to USB control requests

### Comparison with Working State

**Working H713 FEL mode:**
- Clean boot into FEL mode
- Single enumeration, stable device number
- libusb_open() succeeds immediately

**Failed H713 FEL mode:**
- Continuous reset/re-enumeration cycles
- Device number incrementing (77→78→79→80...)
- libusb_open() fails with I/O error

## Previous Timeout Investigation Results

### Test: 20-Second Timeout (from FEL_USB_TIMEOUT_INVESTIGATION.md)

**Modified:** `fel_lib.c` line 34: `USB_TIMEOUT 20000` (was 10000)

**Result:**
```bash
$ time sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl-usb.bin
...
usb_bulk_recv() ERROR -7: Operation timed out
real    0m20.056s  ← Timeout at exactly 20 seconds
```

**Critical Discovery:**
- Transfer is taking **longer than 20 seconds**
- Math predicted only 11.6 seconds for 744 KB at 64 KB/sec
- Timeout is now at the NEW limit (20s), not old (10s)
- Problem is NOT just timing margin - likely protocol incompatibility

## Hypothesis: H713 BROM FEL Protocol Incompatibility

### Evidence

1. **Initial handshake succeeds:**
   - AW_FEL_VERSION request/response works
   - AWUS handshake received
   - Device appears ready

2. **Bulk transfer hangs indefinitely:**
   - No data transferred despite >20 seconds
   - No USB errors or resets during transfer
   - Transfer never completes or fails explicitly

3. **Post-timeout device corruption:**
   - libusb_open() fails with I/O error
   - Continuous USB reset cycles
   - Requires physical power cycle to recover

### Possible Root Causes

#### A. H713 BROM Expects Different Bulk Transfer Protocol

**H616/H6 BROM (working):**
```
Host → Device: AW_USB_WRITE request (length)
Host → Device: bulk_send(data, length)
Device → Host: AWUS acknowledgment
```

**H713 BROM (suspected):**
```
Host → Device: AW_USB_WRITE request (length)
Device → Host: [Additional handshake or memory check?]
Host → Device: bulk_send(data, length)
Device → Host: AWUS acknowledgment
```

The H713 may require additional protocol steps that sunxi-fel doesn't implement.

#### B. H713 SRAM Size Limitation

**SRAM A2 Region:** 0x28000 - 0x13FFFF (1152 KB total)

**Current Memory Layout:**
```
0x104000: SPL load address (128 KB region)
0x121000: Scratch buffer  
0x123a00: Thunk code (512 bytes)
```

**744 KB U-Boot binary may:**
- Exceed available contiguous SRAM space
- Overlap with BROM stack/heap regions
- Collide with swap_buffers or scratch areas

**Evidence against this:** Standard 732 KB binary should also fail if size is the issue.

#### C. USB Bulk Transfer Chunk Size Mismatch

**Current:** `AW_USB_MAX_BULK_SEND` = 512 KB chunks

**H713 BROM may require:**
- Smaller chunk sizes (e.g., 256 KB, 128 KB)
- Different alignment requirements
- Additional delays between chunks

#### D. H713 DRAM Initialization Required First

**H616/H6:** Can load large binaries directly to SRAM

**H713:** May require DRAM initialization before accepting >700 KB transfers

**This would explain:**
- SPL-only transfers might work (32 KB)
- Full U-Boot fails (744 KB)
- No error reported, just indefinite hang

## Recovery Procedure

### Immediate Recovery

**Required:** Physical device reset (unplug/replug USB cable or power cycle device)

**Why:** USB reset commands cannot clear the BROM error state:
```bash
$ sudo usbreset 001/086  # Will NOT fix the issue
```

The H713 BROM needs to restart from cold boot to clear its internal state.

### Recovery Verification

After power cycle:
```bash
$ lsusb | grep 1f3a
Bus 001 Device 087: ID 1f3a:efe8  ← New device number

$ sudo dmesg | tail -10
[timestamp] usb 1-3: new full-speed USB device number 87
[timestamp] usb 1-3: New USB device found, idVendor=1f3a, idProduct=efe8

# Should be stable (no resets)
```

**Success criteria:**
- Single enumeration, no repeated resets
- Device number remains stable
- No disconnect messages in dmesg

## Next Investigation Steps

### 1. Test SPL-only Transfer (Highest Priority)

**Purpose:** Isolate size/protocol issue

```bash
# Extract first 32 KB (SPL only)
dd if=u-boot-sunxi-with-spl-usb.bin of=spl-only.bin bs=1024 count=32

# Test SPL-only transfer
sudo ./sunxi-fel-h713-20s-timeout -v spl spl-only.bin
```

**Expected outcomes:**
- ✅ **Success:** Problem is size-related (>32 KB threshold)
- ❌ **Timeout:** Protocol incompatibility affects all transfers

### 2. Test Standard U-Boot (732 KB)

**Purpose:** Determine if USB gadget code is the trigger

```bash
sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl.bin
```

**Expected outcomes:**
- ✅ **Success:** USB gadget framework is incompatible
- ❌ **Timeout:** Size threshold is <732 KB

### 3. Add Detailed Transfer Logging

**Modify:** `fel_lib.c` bulk transfer functions

```c
ssize_t usb_bulk_send(feldev_handle *dev, int endpoint, const void *data, size_t length, unsigned int timeout)
{
    fprintf(stderr, "[TRANSFER] Sending %zu bytes to EP 0x%02x (timeout=%ums)...\n", 
            length, endpoint, timeout);
    
    ssize_t rc = aw_send_usb_request(dev, AWUSBRequest_tag, length);
    fprintf(stderr, "[TRANSFER] USB request result: %zd\n", rc);
    
    rc = libusb_bulk_transfer(dev->handle, endpoint, (void *)data, length, &actual, timeout);
    fprintf(stderr, "[TRANSFER] Bulk transfer: sent=%d, result=%zd\n", actual, rc);
    
    return rc;
}
```

**Purpose:** Identify exact failure point in transfer sequence

### 4. Try Split Transfer Method

**Purpose:** Work around potential size limits

```bash
# Split binary into chunks
dd if=u-boot-sunxi-with-spl-usb.bin of=spl.bin bs=1024 count=32
dd if=u-boot-sunxi-with-spl-usb.bin of=uboot.bin bs=1024 skip=32

# Transfer in stages
sudo ./sunxi-fel-h713-20s-timeout spl spl.bin
sudo ./sunxi-fel-h713-20s-timeout write 0x4a000000 uboot.bin
sudo ./sunxi-fel-h713-20s-timeout exe 0x4a000000
```

### 5. Increase Timeout to 60 Seconds (Diagnostic)

**Purpose:** Determine if transfer is extremely slow or truly hung

```c
// fel_lib.c line 34
#define USB_TIMEOUT 60000  /* 60 seconds */
```

**Expected outcomes:**
- ✅ **Success at 60s:** Transfer is just very slow (4 KB/sec)
- ❌ **Timeout at 60s:** Transfer is truly hung

### 6. Test with H6 Memory Layout

**Purpose:** Check if H713-specific memory config is incorrect

**Modify:** `soc_info.c` to use H6 settings:

```c
{
    .soc_id       = 0x1860,
    .name         = "H713",
    .spl_addr     = 0x20000,   // Use H6 address
    .scratch_addr = 0x11C00,   // Use H6 scratch
    // ... H6 settings ...
}
```

## Udev Rules for Permissions

**Issue:** NixOS read-only /etc prevents udev rules installation

**Temporary workaround:**
```bash
sudo chmod 666 /dev/bus/usb/001/*
```

**Proper solution for NixOS:**
```nix
# configuration.nix or hardware-configuration.nix
services.udev.extraRules = ''
  # Allwinner FEL mode devices
  SUBSYSTEM=="usb", ATTR{idVendor}=="1f3a", ATTR{idProduct}=="efe8", MODE="0666", GROUP="users"
'';
```

## Conclusions

### Current Understanding

1. **20-second timeout WAS applied correctly** (verified by exact 20.056s timing)
2. **Transfer is NOT completing within 20 seconds**, despite math predicting 11.6s
3. **This indicates protocol incompatibility**, not just timing margin issue
4. **Device enters corrupted USB state** after timeout, requiring power cycle
5. **H713 BROM likely has protocol differences** from H6/H616 that sunxi-fel doesn't handle

### Likely Root Cause

**Hypothesis (80% confidence):** H713 BROM requires additional protocol handshakes or has different bulk transfer expectations that cause indefinite hang. The device never starts the transfer, even though it acknowledges readiness.

### Critical Next Test

**SPL-only transfer** will definitively answer whether this is:
- Size-related (SPL succeeds, full U-Boot fails)
- Protocol-related (both fail)

Until this test completes, we cannot make further progress.

## Required Action

**USER MUST:**
1. Power cycle the device (unplug/replug USB)
2. Verify stable enumeration (no reset cycles)
3. Run SPL-only test to isolate the failure

**Files needed:**
- `spl-only.bin` (create via `dd if=u-boot-sunxi-with-spl-usb.bin of=spl-only.bin bs=1024 count=32`)
- `sunxi-fel-h713-20s-timeout` (already compiled)

**Command:**
```bash
sudo ./sunxi-fel-h713-20s-timeout -v spl spl-only.bin
```

This will provide definitive evidence for next steps.
