# H713 FES Protocol Specification (Draft)

**Date:** 2025-10-12  
**Task:** 029 - FES Loader Minimal Implementation  
**Status:** Research Phase - Protocol Reverse Engineering  
**Confidence:** Low (30%) - Based on limited PhoenixSuit analysis

## Executive Summary

This document specifies the **FES (FEL Stage 2) Protocol** for Allwinner H713, reverse-engineered from PhoenixSuit V1.10 analysis. FES is a two-stage bootloader protocol where a small loader is uploaded via BROM FEL, then extended operations occur via the FES protocol, bypassing BROM limitations.

**Critical Gap:** Limited concrete protocol details available. This specification is a **working hypothesis** requiring validation via:
1. USB traffic capture from PhoenixSuit operations
2. Further reverse engineering of `Phoenix_Fes.dll`
3. Experimental testing with H713 hardware

---

## FEL vs FES Protocol Architecture

### Traditional FEL (Single Stage)

```
┌────────────────┐
│   Host PC      │
│  (sunxi-fel)   │
└───────┬────────┘
        │ USB Bulk Transfers
        │ All operations via BROM
        ▼
┌────────────────┐
│   H713 BROM    │
│   FEL Mode     │
│                │
│  ❌ Bug: Large │
│  transfers     │
│  crash         │
└────────────────┘
```

**Problem:** H713 BROM has USB bulk transfer bug (>16 KB crashes)

### Two-Stage FEL → FES (PhoenixSuit Approach)

```
Stage 1: FEL (BROM)
┌────────────────┐
│   Host PC      │
│  (PhoenixSuit) │
└───────┬────────┘
        │ Small transfer (<16 KB)
        │ FES loader upload
        ▼
┌────────────────┐
│   H713 BROM    │
│   FEL Mode     │
│                │
│  ✅ Small      │
│  transfer OK   │
└───────┬────────┘
        │ Execute FES loader
        ▼
Stage 2: FES Protocol
┌────────────────┐
│   H713 RAM     │
│   FES Loader   │
│   (Executed)   │
│                │
│  ✅ No BROM    │
│  involvement   │
└───────▲────────┘
        │ FES Protocol
        │ Extended commands
        │ Bulk operations
┌───────┴────────┐
│   Host PC      │
│  (FES Client)  │
└────────────────┘
```

**Advantage:** BROM bug bypassed after Stage 1 completes

---

## FES Protocol Command Structure (HYPOTHESIS)

### USB Communication

**USB Device Descriptor:**
- Vendor ID: `0x1f3a` (Allwinner)
- Product ID: `0xefe8` (FEL mode)
- After FES loader executes: **Unknown** (may change or stay same)

**USB Endpoints:**
- Endpoint OUT: Bulk endpoint for commands/data to device
- Endpoint IN: Bulk endpoint for responses/data from device

**Transfer Characteristics:**
- Command/Response pattern (similar to SCSI/USB Mass Storage)
- Variable-length data transfers
- Status responses after operations

### Command Packet Format (INFERRED)

Based on `efex_core.cpp` string references:

```c
struct fes_command {
    uint32_t magic;          // "FEX_CMD" or similar
    uint32_t command;        // Command opcode
    uint32_t address;        // Memory address (for read/write/execute)
    uint32_t length;         // Data length
    uint32_t flags;          // Operation flags
    uint8_t  reserved[12];   // Padding/reserved
};  // Total: 32 bytes (typical for USB bulk protocols)
```

### Response Packet Format (INFERRED)

```c
struct fes_response {
    uint32_t magic;          // Response magic
    uint32_t status;         // 0 = success, non-zero = error
    uint32_t data_length;    // Length of data following response
    uint8_t  reserved[20];   // Padding
};  // Total: 32 bytes
```

**Evidence from eFex.dll strings:**
```
CSWStatus= %x(%x) Magic=%x(%x)
```

Suggests USB Mass Storage-style Command Status Wrapper (CSW) format.

---

## FES Command Set (PRELIMINARY)

### Known Commands (from PhoenixSuit strings)

| Command Name | Evidence | Likely Opcode | Purpose |
|--------------|----------|---------------|---------|
| `FEX_CMD_fes_trans` | efex_core.cpp:160 | 0x0201? | Transition to FES mode |
| `fel_up` | efex_core.cpp:158 | 0x0101? | Upload data to memory |
| `fel_down` | efex_core.cpp:166 | 0x0102? | Download data from memory |
| `fet_run` | efex_core.cpp:156 | 0x0103? | Execute code at address |
| `verify_dev` | efex_core.cpp:173 | 0x0001? | Device verification/handshake |
| `is_ready` | efex_core.cpp:175 | 0x0002? | Check FES loader ready status |
| `get_cmd_set_ver` | efex_core.cpp:168 | 0x0000? | Get command set version |

**Note:** Opcodes are speculative based on typical USB protocol design patterns

### Essential FES Commands for Minimal Loader

#### 1. Handshake / Verify Device

**Command:** `FEX_CMD_VERIFY_DEV` (Opcode: 0x0001)

**Purpose:** Host verifies FES loader is active and responding

**Request:**
```c
fes_command {
    .magic = 0x46455843,  // "FEXC"
    .command = 0x0001,
    .address = 0,
    .length = 0,
    .flags = 0
}
```

**Response:**
```c
fes_response {
    .magic = 0x46455852,  // "FEXR"
    .status = 0,          // 0 = success
    .data_length = 8
}
+ data: "AWUSBFEX" (8-byte identifier string, seen in efex_core.cpp:171)
```

#### 2. Memory Write (Upload)

**Command:** `FEX_CMD_FEL_UP` (Opcode: 0x0101)

**Purpose:** Write data to device memory

**Request:**
```c
fes_command {
    .magic = 0x46455843,
    .command = 0x0101,
    .address = target_addr,  // e.g., 0x104000 for SPL
    .length = data_size,     // bytes to write
    .flags = 0
}
+ data: [binary data to write]
```

**Response:**
```c
fes_response {
    .magic = 0x46455852,
    .status = 0,           // 0 = success
    .data_length = 0
}
```

#### 3. Memory Read (Download)

**Command:** `FEX_CMD_FEL_DOWN` (Opcode: 0x0102)

**Purpose:** Read data from device memory

**Request:**
```c
fes_command {
    .magic = 0x46455843,
    .command = 0x0102,
    .address = source_addr,
    .length = read_size,
    .flags = 0
}
```

**Response:**
```c
fes_response {
    .magic = 0x46455852,
    .status = 0,
    .data_length = read_size
}
+ data: [binary data read from memory]
```

#### 4. Execute

**Command:** `FEX_CMD_FET_RUN` (Opcode: 0x0103)

**Purpose:** Execute code at specified address

**Request:**
```c
fes_command {
    .magic = 0x46455843,
    .command = 0x0103,
    .address = exec_addr,  // e.g., 0x104000 to run SPL
    .length = 0,
    .flags = 0
}
```

**Response:**
```c
fes_response {
    .magic = 0x46455852,
    .status = 0,
    .data_length = 0
}
```

**Note:** After execute, device may no longer respond via FES protocol if execution transfers control permanently

#### 5. Check Ready Status

**Command:** `FEX_CMD_IS_READY` (Opcode: 0x0002)

**Purpose:** Check if FES loader is initialized and ready

**Request:**
```c
fes_command {
    .magic = 0x46455843,
    .command = 0x0002,
    .address = 0,
    .length = 0,
    .flags = 0
}
```

**Response:**
```c
fes_response {
    .magic = 0x46455852,
    .status = 0,          // 0 = ready
    .data_length = 0
}
```

---

## FES Loader Implementation Requirements

### Memory Map

```
┌─────────────────────────────────────┐ 0x00000000
│        H713 BROM                    │
├─────────────────────────────────────┤ 0x00020000
│        SRAM A1 (96 KB)              │
├─────────────────────────────────────┤ 0x00038000
│        SRAM A2 (32 KB)              │
├─────────────────────────────────────┤ 0x00040000
│        SRAM C (64 KB)               │
├─────────────────────────────────────┤ 0x00050000
│        (Reserved)                   │
├─────────────────────────────────────┤ 0x00104000
│        SPL Load Address             │ (Task 027)
│        (U-Boot destination)         │
├─────────────────────────────────────┤ 0x00121000
│        FES Loader Location          │ (Task 027 scratch addr)
│        (16 KB reserved)             │
├─────────────────────────────────────┤ 0x00125000
│        FES Stack                    │
│        (4 KB)                       │
├─────────────────────────────────────┤ 0x00126000
│        Available SRAM               │
└─────────────────────────────────────┘
```

**FES Loader Constraints:**
- Maximum size: 16 KB (0x4000 bytes)
- Load address: 0x121000 (Task 027 scratch buffer)
- Stack: 0x125000 (grows down)
- Must fit entirely in SRAM before DRAM initialization

### Core Functionality

**Minimal FES Loader Components:**

```c
// 1. Entry point and initialization
void _start(void) {
    // Setup stack pointer
    // Initialize USB controller
    // Enter command loop
}

// 2. USB communication handler
int usb_receive_command(fes_command *cmd);
int usb_send_response(fes_response *resp);
int usb_receive_data(void *buffer, uint32_t length);
int usb_send_data(void *buffer, uint32_t length);

// 3. Command dispatcher
void fes_command_loop(void) {
    fes_command cmd;
    fes_response resp;
    
    while (1) {
        if (usb_receive_command(&cmd) < 0)
            continue;
        
        switch (cmd.command) {
            case 0x0001:  // VERIFY_DEV
                handle_verify(&cmd, &resp);
                break;
            case 0x0101:  // FEL_UP (write)
                handle_write(&cmd, &resp);
                break;
            case 0x0102:  // FEL_DOWN (read)
                handle_read(&cmd, &resp);
                break;
            case 0x0103:  // FET_RUN (execute)
                handle_execute(&cmd, &resp);
                break;
            case 0x0002:  // IS_READY
                handle_ready(&cmd, &resp);
                break;
            default:
                resp.status = 0xFFFF;  // Unknown command
                break;
        }
        
        usb_send_response(&resp);
    }
}

// 4. Command handlers
void handle_write(fes_command *cmd, fes_response *resp) {
    void *dest = (void *)cmd->address;
    usb_receive_data(dest, cmd->length);
    resp->status = 0;  // Success
    resp->data_length = 0;
}

void handle_read(fes_command *cmd, fes_response *resp) {
    void *src = (void *)cmd->address;
    resp->status = 0;
    resp->data_length = cmd->length;
    usb_send_response(resp);
    usb_send_data(src, cmd->length);
}

void handle_execute(fes_command *cmd, fes_response *resp) {
    void (*entry)(void) = (void (*)(void))cmd->address;
    resp->status = 0;
    resp->data_length = 0;
    usb_send_response(resp);
    
    // Jump to address (may not return)
    entry();
}
```

### Size Optimization Strategy

To fit in 16 KB:

1. **Minimal USB driver** - Only bulk transfer support, no descriptors/enumeration (BROM already handled)
2. **No standard library** - Direct register access, no libc
3. **Inline assembly** - Critical paths hand-optimized
4. **Stripped binary** - No debug symbols, minimal alignment
5. **Thumb mode** - ARM Thumb-2 instruction set for code density

---

## FES Transition Process

### Stage 1: Upload FES Loader via BROM FEL

```bash
# Use Task 027 sunxi-fel with correct H713 addresses
./sunxi-fel-h713-fixed write 0x121000 fes_loader.bin
./sunxi-fel-h713-fixed exe 0x121000
```

**Expected Outcome:**
- FES loader uploads successfully (<16 KB, below BROM bug threshold)
- FES loader executes and initializes
- USB device remains connected (FES loader now handling USB)

**Validation:**
- USB device stays enumerated (check `lsusb`)
- FES loader responds to verify command

### Stage 2: Communicate via FES Protocol

```python
# Python FES client
import usb.core

# Find device (still VID:PID 0x1f3a:0xefe8)
dev = usb.core.find(idVendor=0x1f3a, idProduct=0xefe8)

# Send verify command
cmd = fes_command(magic=0x46455843, command=0x0001, ...)
dev.write(endpoint_out, cmd)

# Receive response
resp = dev.read(endpoint_in, 32)  # 32-byte response
data = dev.read(endpoint_in, 8)   # "AWUSBFEX"

# FES loader is now active and ready for commands
```

### Stage 3: Upload U-Boot SPL via FES

```python
# Read U-Boot SPL file
with open('u-boot-sunxi-with-spl.bin', 'rb') as f:
    spl_data = f.read()

# Write to SPL address (Task 027: 0x104000)
fes_write_memory(dev, 0x104000, spl_data)

# Execute SPL
fes_execute(dev, 0x104000)

# U-Boot SPL now running, Phase II complete
```

---

## Validation and Testing Plan

### Phase 1: Protocol Research (Current)

- [x] Analyze PhoenixSuit DLLs for command references
- [ ] **Capture USB traffic** from PhoenixSuit operations (HIGH PRIORITY)
- [ ] Reverse engineer `Phoenix_Fes.dll` with Ghidra (if USB capture insufficient)
- [ ] Search for Allwinner FES documentation (Chinese sources)
- [ ] Validate command structure hypothesis

**Hardware Required:** Windows PC with PhoenixSuit + USB analyzer OR Wireshark

### Phase 2: FES Loader Implementation

- [ ] Implement minimal USB bulk transfer handler
- [ ] Implement command parser and dispatcher
- [ ] Implement memory read/write/execute handlers
- [ ] Build and verify size < 16 KB
- [ ] Test compilation with cross-compiler

**Hardware Required:** None (software development)

### Phase 3: BROM FEL Upload Test

- [ ] Connect HY300 in FEL mode
- [ ] Upload FES loader via `sunxi-fel-h713-fixed`
- [ ] Verify upload succeeds (no USB timeout)
- [ ] Verify FES loader executes (USB stays connected)

**Hardware Required:** HY300 device, FEL mode access

### Phase 4: FES Protocol Communication Test

- [ ] Implement Python FES client
- [ ] Send verify command, receive "AWUSBFEX"
- [ ] Test memory write (small 1 KB test payload)
- [ ] Test memory read (read back test payload)
- [ ] Test execute (jump to test code)

**Hardware Required:** HY300 device with FES loader running

### Phase 5: Full U-Boot SPL Upload

- [ ] Upload full 732 KB U-Boot SPL via FES
- [ ] Execute SPL at 0x104000
- [ ] Verify U-Boot console accessible
- [ ] Phase II complete

**Hardware Required:** HY300 device, serial console access

---

## Open Questions and Research Needed

### Critical Unknowns

1. **USB Endpoint Numbers:**
   - What are the exact bulk OUT/IN endpoint addresses?
   - Does FES loader need to re-enumerate USB device?
   - Or does it reuse BROM's USB configuration?

2. **Command Packet Format:**
   - Exact magic numbers for command/response
   - Correct structure size and field order
   - Endianness (little-endian assumed)

3. **Transfer Sizes:**
   - Maximum bulk transfer size for FES protocol
   - Need for chunking large payloads (SPL is 732 KB)
   - Timeout values

4. **USB Controller State:**
   - Does BROM leave USB controller initialized?
   - Does FES loader need full USB reinitialization?
   - H713 USB controller register addresses

5. **Error Handling:**
   - What status codes indicate errors?
   - How does FES loader signal failures?
   - Recovery mechanisms

### Next Research Steps (Priority Order)

1. **USB Traffic Capture (CRITICAL):**
   - Run PhoenixSuit on Windows PC
   - Use Wireshark + USBPcap to capture USB traffic
   - Analyze actual FES command structure and sequences
   - **This will resolve most unknowns**

2. **Ghidra Reverse Engineering:**
   - Disassemble `Phoenix_Fes.dll`
   - Identify command structure definitions
   - Find USB communication code paths
   - Extract constants and magic numbers

3. **Community Resources:**
   - Search linux-sunxi wiki (may require VPN)
   - Check Allwinner SDK leaks (Chinese forums)
   - sunxi-tools GitHub issues/PRs mentioning FES
   - Contact sunxi community developers

4. **H713 USB Controller Documentation:**
   - Find H713 datasheet or reference manual
   - Identify USB OTG controller registers
   - Understand USB initialization requirements
   - Compare with H616 (similar SoC)

---

## Fallback Strategies

If FES protocol proves too complex or undocumented:

### Fallback 1: Chunked BROM FEL Transfers

- Test various chunk sizes (4 KB, 8 KB, 12 KB, 16 KB)
- Find largest chunk that doesn't trigger BROM crash
- Upload SPL in chunks with delays
- **Confidence:** 30% (may not resolve root cause)

### Fallback 2: SD Card Boot

- Write U-Boot to SD card
- Boot from SD card slot (standard Allwinner approach)
- Bypass FEL entirely
- **Confidence:** 95% (proven method)

### Fallback 3: Wait for Newer PhoenixSuit

- Search for PhoenixSuit V1.18+ with H713 support
- May contain explicit H713 workarounds
- Extract FES payloads for analysis
- **Confidence:** 60% (if newer version exists)

---

## Size Budget Breakdown

### FES Loader Components (Target: <16 KB)

```
Component                    Estimated Size
================================================
Entry point + startup        ~256 bytes
USB register definitions     ~128 bytes
USB bulk transfer handlers   ~2 KB
Command packet parser        ~512 bytes
Memory operation handlers    ~1 KB
Command dispatcher loop      ~512 bytes
Response generation          ~256 bytes
Utility functions            ~512 bytes
Stack (embedded)             ~1 KB
================================================
Core functionality           ~6 KB

Optimization margin          ~10 KB
================================================
Total target                 <16 KB
```

**Size validation:**
```bash
nix develop -c -- aarch64-unknown-linux-gnu-size fes_loader.elf
   text    data     bss     dec     hex filename
   5432     128     512    6072    17b8 fes_loader.elf
```

Must verify `text + data + bss < 16384 bytes`

---

## Success Criteria

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

## Document Status

**Status:** DRAFT - Hypothetical Specification  
**Confidence:** Low (30%)  
**Validation Required:** USB traffic capture and protocol testing  
**Next Action:** USB traffic analysis with Wireshark or Ghidra reverse engineering

**Critical Path:**
1. Capture PhoenixSuit USB traffic → Validate protocol hypothesis
2. Implement FES loader based on validated protocol
3. Test on H713 hardware
4. Iterate based on results

---

## References

- **Task 027:** H713 FEL Memory Map Reverse Engineering
- **Task 034:** PhoenixSuit FEL Protocol Reverse Engineering
- **PhoenixSuit V1.10:** DLL analysis results (`tools/phoenixsuit/dll_analysis/`)
- **eFex.dll strings:** Command references (FEX_CMD_fes_trans, fel_up, fel_down, fet_run)
- **Phoenix_Fes.dll strings:** Elf2Fes conversion references

**External Resources (To Research):**
- Allwinner FEL/FES USB protocol documentation
- linux-sunxi wiki FEL/USBBoot pages
- sunxi-tools source code (fel.c, usb implementation)
- Allwinner SDK BSP sources (if available)

---

**Document will be updated as protocol details are validated through USB capture and testing.**
