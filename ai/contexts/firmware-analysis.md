# Firmware Analysis Context

## Stock Android ROM Analysis

### Expected Files in ROM
- **boot0.bin:** SPL with DRAM initialization parameters
- **boot1.bin:** U-Boot secondary bootloader  
- **system.img:** Android system partition
- **userdata.img:** User data partition
- **recovery.img:** Recovery partition

### Analysis Tools

#### binwalk
```bash
# Extract all files from ROM
binwalk -e firmware.img

# List file signatures
binwalk firmware.img

# Extract specific file types
binwalk -D='.*' firmware.img
```

#### Android Image Tools
```bash
# Extract system.img
mkdir system_extracted
sudo mount -o loop system.img system_extracted

# Extract boot.img components
unpackbootimg -i boot.img -o boot_extracted/
```

### Critical Extraction Tasks

#### 1. DRAM Parameters from boot0.bin
- **Location:** Usually first 32KB of ROM
- **Contents:** SPL + DRAM timing configuration
- **Format:** Allwinner EGON header + ARM code
- **Extraction:** Use hexdump, strings, disassembler

#### 2. Device Tree Blob
- **Location:** boot.img or separate partition
- **Purpose:** Hardware configuration
- **Extraction:** `dtc -I dtb -O dts device.dtb`

#### 3. Wireless Firmware
- **Location:** /vendor/firmware/ in system.img
- **Files:** Look for aic8800*.bin files
- **Purpose:** WiFi/Bluetooth functionality

### Analysis Workflow

1. **Initial Examination**
   ```bash
   file firmware.img
   hexdump -C firmware.img | head -20
   strings firmware.img | grep -i allwinner
   ```

2. **Structure Analysis**
   ```bash
   binwalk firmware.img
   fdisk -l firmware.img
   ```

3. **Partition Extraction**
   ```bash
   binwalk -e firmware.img
   cd _firmware.img.extracted/
   ```

4. **boot0.bin Analysis**
   ```bash
   hexdump -C boot0.bin | head -50
   strings boot0.bin
   objdump -D -b binary -m arm boot0.bin
   ```

### DRAM Parameter Extraction
- **Target Values:** Clock speeds, timing, ZQ calibration
- **Format:** CONFIG_DRAM_* variables for U-Boot
- **Method:** Reverse engineer assembly code in boot0.bin
- **Tools:** Ghidra, radare2, or manual analysis