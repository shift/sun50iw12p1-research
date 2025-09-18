# HY300 ROM Analysis Report

## ROM Image Overview
- **File:** update.img
- **Size:** 1.6GB
- **Format:** Custom Allwinner IMAGEWTY format
- **Header:** IMAGEWTY signature at offset 0x0

## Extracted Components

### Boot Loaders (eGON.BT0)
Found 5 eGON.BT0 bootloader instances:
1. **Primary boot0:** Offset 136196 (0x21404) - Extracted as boot0.bin
2. **Secondary boot0:** Offset 168964 (0x29404) 
3. **Additional boot0:** Offset 612168 (0x957C8)
4. **Additional boot0:** Offset 843780 (0xCE004)
5. **Additional boot0:** Offset 1270600 (0x136648)

### Device Tree Blobs (DTB)
Found 4 device tree blobs:
1. **DTB 1:** Offset 64512 (0xFC00) - Size: 67150 bytes
2. **DTB 2:** Offset 816476 (0xC755C) - Size: 16802 bytes  
3. **DTB 3:** Offset 1474908 (0x16815C) - Size: 16802 bytes
4. **DTB 4:** Offset 2010112 (0x1EAC00) - Size: 67150 bytes

### Partition Table
- **GPT Table:** Offset 2762752 (0x2A2800)
- Multiple Windows PE binaries found (likely Android system components)

## Key Findings

### DRAM Parameters
- Boot0.bin extracted successfully (32KB)
- Contains Allwinner H713 DRAM initialization code
- **Next Step:** Reverse engineer DRAM parameters from boot0.bin assembly

### Hardware Configuration
- Multiple DTB files suggest different hardware variants
- Need to decompile DTBs to identify:
  - Memory configuration
  - GPIO mappings
  - Peripheral addresses
  - WiFi module configuration

### Wireless Module
- Found "wlan_regon" GPIO references in strings
- Confirms AW869A/AIC8800 wireless module
- Firmware blobs likely in system partition

## Analysis Tools Used
- **binwalk:** ROM structure analysis and extraction
- **hexdump:** Binary inspection
- **strings:** Text pattern analysis
- **dd:** Boot0 extraction

## Next Steps
1. Reverse engineer DRAM parameters from boot0.bin
2. Decompile device tree blobs for hardware mapping
3. Extract wireless firmware from system partition
4. Create U-Boot configuration based on findings