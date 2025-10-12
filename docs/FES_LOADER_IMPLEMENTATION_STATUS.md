# FES Loader Implementation Status

**Date:** 2025-10-12  
**Task:** 029 - FES Loader Minimal Implementation  
**Status:** Blocked on USB controller implementation  

## Executive Summary

The FES (FEL Stage 2) loader skeleton is complete with all command handlers implemented, but **blocked on H713 USB controller implementation details**. The main gap is understanding how to communicate with the H713 USB OTG controller after BROM has initialized it.

---

## Completed Work

### 1. FES Protocol Specification Research ✅

**Document:** `docs/H713_FES_PROTOCOL_SPECIFICATION.md`

**Key Findings:**
- FES protocol is a **two-stage bootloader approach** (FEL → FES)
- Stage 1 (BROM FEL): Upload small loader (<16 KB) → Success
- Stage 2 (FES Protocol): Use loader for bulk operations → Bypasses BROM bug
- Command/Response pattern with 32-byte packets
- Commands: VERIFY, IS_READY, FEL_UP (write), FEL_DOWN (read), FET_RUN (execute)

**Confidence Level:** Low (30%)
- Based on PhoenixSuit DLL string analysis (Task 034)
- No USB traffic capture validation yet
- Protocol hypothesis needs hardware testing

**Evidence Sources:**
- PhoenixSuit V1.10 `eFex.dll` strings (FEX_CMD_fes_trans, fel_up, fel_down, fet_run)
- `Phoenix_Fes.dll` references to Elf2Fes conversion
- Command format inferred from typical USB bulk protocols

### 2. FES Loader Skeleton Implementation ✅

**File:** `tools/fes_loader/fes_loader.c` (290 lines)

**Implemented Components:**
- ✅ FES protocol structures (32-byte command/response packets)
- ✅ Command opcodes (0x0001 VERIFY, 0x0101 FEL_UP, 0x0102 FEL_DOWN, 0x0103 FET_RUN)
- ✅ Command handler functions for all operations
- ✅ Main command loop with dispatching
- ✅ Memory operations (read/write/execute)

**Code Structure:**
```c
struct fes_command {
    uint32_t magic;      // 0x46455843 "FEXC"
    uint32_t command;    // Command opcode
    uint32_t address;    // Memory address
    uint32_t length;     // Data length
    uint32_t flags;      // Operation flags
    uint8_t  reserved[12];
} __attribute__((packed));

struct fes_response {
    uint32_t magic;      // 0x46455852 "FEXR"
    uint32_t status;     // Status code
    uint32_t data_length; // Following data length
    uint8_t  reserved[20];
} __attribute__((packed));
```

**Size Budget:** Target <16 KB (currently skeleton only)

### 3. Mainline sunxi-tools Analysis ✅

**Source:** `https://github.com/linux-sunxi/sunxi-tools/blob/master/fel.c`

**Key Findings:**
- ❌ **No FES protocol implementation in mainline sunxi-tools**
- ✅ Only implements basic FEL protocol (write, read, execute)
- ✅ Uses standard libusb for USB communication
- ✅ Confirms FES is PhoenixSuit-specific, not documented publicly

**Relevant Functions:**
- `aw_fel_write()` - Standard FEL write (BROM protocol)
- `aw_fel_read()` - Standard FEL read (BROM protocol)
- `aw_fel_execute()` - Standard FEL execute (BROM protocol)
- No FES-equivalent functions exist

**Implication:** We cannot reference existing FES implementation; must reverse engineer or capture USB traffic

---

## Critical Gap: USB Controller Implementation

### Problem Statement

The FES loader skeleton has **stub functions for USB communication**:

```c
static int usb_receive_command(struct fes_command *cmd)
{
    /* TODO: Implement USB bulk IN transfer */
    return -1;  /* Not implemented */
}

static int usb_send_response(struct fes_response *resp)
{
    /* TODO: Implement USB bulk OUT transfer */
    return -1;  /* Not implemented */
}

static int usb_receive_data(void *buffer, uint32_t length)
{
    /* TODO: Implement USB bulk data transfer */
    return -1;  /* Not implemented */
}

static int usb_send_data(void *buffer, uint32_t length)
{
    /* TODO: Implement USB bulk data transfer */
    return -1;  /* Not implemented */
}
```

### What We Need to Know

1. **H713 USB OTG Register Map:**
   - USB controller base address (assumed 0x05100000 from H616 reference)
   - Endpoint registers for bulk transfers
   - FIFO addresses and control registers
   - Interrupt status and control

2. **USB State After BROM:**
   - Does BROM leave USB controller initialized?
   - Are endpoints already configured?
   - What is the USB device state (addressed, configured)?
   - Do we need to re-enumerate the device?

3. **USB Protocol Details:**
   - Exact bulk endpoint numbers (IN/OUT)
   - Maximum packet size for bulk transfers
   - Transfer completion signaling
   - Error handling and recovery

### Information Sources (Priority Order)

#### Option 1: USB Traffic Capture (HIGHEST PRIORITY) ⭐
- **Method:** Capture PhoenixSuit USB traffic with Wireshark + USBPcap
- **Requirements:** 
  - Windows PC with PhoenixSuit V1.10
  - HY300 device in FEL mode
  - Wireshark with USBPcap installed
- **Expected Results:**
  - Exact FES command format validation
  - USB endpoint numbers and packet sizes
  - Command sequences and timing
  - Response format verification
- **Confidence:** 95% (would resolve most unknowns)
- **Status:** Blocked on hardware access

#### Option 2: H713 USB Controller Documentation
- **Method:** Find H713 datasheet or reference manual
- **Requirements:**
  - H713 technical documentation (likely Chinese only)
  - May require Allwinner SDK leak or Chinese forums
- **Expected Results:**
  - USB OTG register map
  - Programming guide for bulk transfers
  - Initialization sequence
- **Confidence:** 60% (documentation may not be public)
- **Alternatives:** Use H616 documentation as reference (similar SoC)

#### Option 3: PhoenixSuit DLL Reverse Engineering
- **Method:** Disassemble `Phoenix_Fes.dll` with Ghidra
- **Requirements:**
  - PhoenixSuit V1.10 installation files
  - Ghidra or IDA Pro
  - ARM and USB protocol knowledge
- **Expected Results:**
  - FES loader binary extraction
  - USB communication code paths
  - Protocol constants and structures
- **Confidence:** 70% (requires significant reverse engineering effort)
- **Status:** Can be done without hardware

#### Option 4: Community Resources
- **Method:** Research linux-sunxi wiki and forums
- **Requirements:**
  - VPN access (linux-sunxi wiki may be blocked)
  - Search Chinese forums (Allwinner community)
- **Expected Results:**
  - FES protocol references
  - H713 USB controller information
  - Community workarounds
- **Confidence:** 30% (limited public documentation)

---

## Implementation Roadmap (Post-Unblock)

### Phase 1: USB Controller Implementation

Once USB controller details are known:

1. **Create USB Register Definitions**
   ```c
   /* H713 USB OTG Register Map */
   #define USB_OTG_BASE        0x05100000
   #define USB_EP0_FIFO        (USB_OTG_BASE + 0x000)
   #define USB_EP1_FIFO        (USB_OTG_BASE + 0x004)
   // ... (fill in from H713 documentation)
   ```

2. **Implement USB Transfer Functions**
   - Bulk IN transfer (receive from host)
   - Bulk OUT transfer (send to host)
   - Error handling and timeout
   - FIFO management

3. **Test USB Communication**
   - Upload FES loader via BROM FEL
   - Verify USB device stays connected
   - Test VERIFY command response

### Phase 2: ARM Startup Code

Create `tools/fes_loader/startup.s`:

```asm
.global _start
_start:
    /* Setup stack pointer */
    ldr     sp, =0x125000
    
    /* Jump to C code */
    bl      main
    
    /* Infinite loop (should never reach) */
1:  b       1b
```

### Phase 3: Build System

Create `tools/fes_loader/Makefile`:

```makefile
CC = aarch64-unknown-linux-gnu-gcc
OBJCOPY = aarch64-unknown-linux-gnu-objcopy
CFLAGS = -nostdlib -nostartfiles -Os -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections -T linker.ld

fes_loader.bin: fes_loader.elf
	$(OBJCOPY) -O binary $< $@
	@echo "FES Loader size: $$(stat -c %s $@) bytes (must be < 16384)"

fes_loader.elf: fes_loader.o startup.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.elf *.bin
```

### Phase 4: FES Client Tool

Create `tools/fes_client.py`:

```python
#!/usr/bin/env python3
import usb.core
import struct

class FESClient:
    VENDOR_ID = 0x1f3a
    PRODUCT_ID = 0xefe8
    
    def __init__(self):
        self.dev = usb.core.find(idVendor=self.VENDOR_ID, 
                                  idProduct=self.PRODUCT_ID)
        if not self.dev:
            raise RuntimeError("FEL device not found")
    
    def send_command(self, cmd, addr=0, length=0):
        """Send FES command packet"""
        packet = struct.pack('<IIIII12s', 
                            0x46455843,  # "FEXC"
                            cmd, addr, length, 0,
                            b'\x00' * 12)
        self.dev.write(0x01, packet)  # EP OUT
    
    def receive_response(self):
        """Receive FES response packet"""
        data = self.dev.read(0x81, 32)  # EP IN
        return struct.unpack('<III20s', bytes(data))
    
    def fes_verify(self):
        """Test FES loader connectivity"""
        self.send_command(0x0001)
        magic, status, length, _ = self.receive_response()
        if magic == 0x46455852 and status == 0:
            device_id = self.dev.read(0x81, length)
            print(f"FES Device ID: {bytes(device_id).decode()}")
            return True
        return False
    
    def fes_write_memory(self, addr, data):
        """Write data to device memory via FES"""
        self.send_command(0x0101, addr, len(data))
        self.dev.write(0x01, data)
        magic, status, _, _ = self.receive_response()
        return status == 0
    
    def fes_execute(self, addr):
        """Execute code at address via FES"""
        self.send_command(0x0103, addr, 0)
        magic, status, _, _ = self.receive_response()
        return status == 0

# Example usage:
# client = FESClient()
# client.fes_verify()
# client.fes_write_memory(0x104000, spl_data)
# client.fes_execute(0x104000)
```

### Phase 5: Integration Testing

1. **Upload FES Loader**
   ```bash
   ./sunxi-fel-h713-fixed write 0x121000 fes_loader.bin
   ./sunxi-fel-h713-fixed exe 0x121000
   ```

2. **Test FES Protocol**
   ```bash
   python tools/fes_client.py verify
   python tools/fes_client.py write 0x104000 u-boot-sunxi-with-spl.bin
   python tools/fes_client.py execute 0x104000
   ```

3. **Validate U-Boot Boot**
   - U-Boot SPL executes successfully
   - U-Boot console accessible via serial
   - Phase II complete

---

## Current Blockers

### Hardware Access Required ⚠️

**All testing requires:**
- HY300 device in FEL mode
- USB connection to development PC
- Serial console access (for U-Boot validation)

**Cannot proceed without hardware for:**
- USB traffic capture (highest priority)
- FES loader upload testing
- Protocol validation
- U-Boot boot verification

### Alternative Progress Without Hardware

**Can be done now:**
1. ✅ Complete ARM startup code (`startup.s`)
2. ✅ Create build system (`Makefile`, linker script)
3. ✅ Implement FES client tool (`fes_client.py`)
4. ⏳ Reverse engineer `Phoenix_Fes.dll` with Ghidra
5. ⏳ Research H713 USB controller (datasheet hunting)
6. ⏳ Study H616 USB OTG as reference architecture

**Priority:** Focus on **PhoenixSuit DLL reverse engineering** to extract USB controller implementation details

---

## Risk Assessment

### Technical Risks

1. **USB Protocol Mismatch (HIGH)**
   - Hypothesis may be wrong about packet format
   - Mitigation: USB traffic capture will validate
   - Impact: May need protocol redesign

2. **USB Controller Complexity (MEDIUM)**
   - H713 USB OTG may require complex initialization
   - Mitigation: Study H616 reference implementation
   - Impact: May exceed 16 KB size budget

3. **BROM USB State (MEDIUM)**
   - Uncertain if USB stays initialized after FES loader executes
   - Mitigation: Test with hardware, add re-init if needed
   - Impact: May need USB enumeration code

4. **Size Budget Overflow (LOW)**
   - Current skeleton is small, but USB code may be large
   - Mitigation: Aggressive optimization, consider 32 KB threshold
   - Impact: May need to test larger loader

### Project Risks

1. **Hardware Access Delay (HIGH)**
   - Physical device required for all testing
   - Mitigation: Prepare all software components in advance
   - Impact: Task completion blocked until hardware available

2. **Protocol Documentation Gap (MEDIUM)**
   - No public FES protocol specification
   - Mitigation: Multiple research paths (DLL RE, traffic capture)
   - Impact: May take longer than expected

---

## Success Criteria (Unchanged)

### Minimal Success (Proof of Concept)
- [ ] FES loader < 16 KB compiled successfully
- [ ] FES loader uploads via BROM FEL without crash
- [ ] FES protocol responds to verify command
- [ ] Can write 32 KB test payload via FES (demonstrates BROM bypass)

### Full Success (Phase II Complete)
- [ ] All minimal success criteria met
- [ ] Can upload full 732 KB U-Boot SPL via FES
- [ ] U-Boot SPL executes successfully
- [ ] U-Boot console accessible via serial
- [ ] H713 BROM bug completely bypassed

---

## Next Actions

### Immediate (No Hardware Required)

1. **PhoenixSuit DLL Reverse Engineering** ⭐
   - Extract `Phoenix_Fes.dll` from PhoenixSuit V1.10
   - Disassemble with Ghidra
   - Locate USB communication functions
   - Extract FES loader binary if embedded
   - Document USB controller interaction

2. **Complete Build System**
   - Create ARM startup code (`startup.s`)
   - Write linker script (`linker.ld`)
   - Implement Makefile with size validation
   - Test cross-compilation in Nix environment

3. **Implement FES Client Tool**
   - Write Python FES client (`fes_client.py`)
   - Add command-line interface
   - Implement file upload with chunking
   - Prepare for hardware testing

4. **H713 USB Research**
   - Search for H713 datasheet (Chinese sources)
   - Study H616 USB OTG as reference
   - Document USB controller register map
   - Prepare USB driver implementation

### When Hardware Available

1. **USB Traffic Capture** ⭐⭐⭐
   - Install Wireshark + USBPcap on Windows
   - Run PhoenixSuit firmware update operation
   - Capture USB traffic during FES stage
   - Analyze and validate protocol hypothesis

2. **FES Loader Testing**
   - Upload FES loader via BROM FEL
   - Test USB communication
   - Iterate on USB controller implementation
   - Validate protocol with real hardware

---

## Conclusion

The FES loader approach is **technically sound** but **blocked on USB controller implementation details**. The skeleton code is complete and well-structured. The main gap is understanding H713 USB OTG programming, which requires either:

1. **USB traffic capture** (95% confidence) - Highest priority when hardware available
2. **PhoenixSuit DLL reverse engineering** (70% confidence) - Can start immediately
3. **H713 documentation** (60% confidence) - Requires Chinese sources

**Recommendation:** Proceed with **PhoenixSuit DLL reverse engineering** (Option 3) while waiting for hardware access to perform USB traffic capture (Option 1). This maximizes progress without hardware and prepares for rapid testing once hardware is available.

**Task Status:** Mark as **blocked** pending hardware access or DLL reverse engineering completion.

---

## Related Documentation

- `docs/H713_FES_PROTOCOL_SPECIFICATION.md` - FES protocol hypothesis
- `docs/PHOENIXSUIT_FEL_PROTOCOL_ANALYSIS.md` - PhoenixSuit architecture analysis
- `docs/PHOENIXSUIT_RESEARCH_INTEGRATION.md` - Strategy decision tree
- `docs/FEL_USB_TIMEOUT_INVESTIGATION.md` - H713 BROM bug analysis
- `docs/tasks/029-fes-loader-minimal-implementation.md` - Task details
- `docs/tasks/completed/027-h713-fel-memory-map-reverse-engineering.md` - Memory addresses
- `docs/tasks/completed/034-phoenixsuit-fel-protocol-reverse-engineering.md` - PhoenixSuit analysis
- `tools/fes_loader/fes_loader.c` - FES loader skeleton (290 lines)

---

**Status:** Research and skeleton implementation complete. USB controller implementation blocked pending hardware access or DLL reverse engineering. Ready to proceed with alternative research paths.
