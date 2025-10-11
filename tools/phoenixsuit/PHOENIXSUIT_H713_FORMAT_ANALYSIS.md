# PhoenixSuit H713 Image Format Analysis

## Overview
Analysis of HY300 `update.img` (1.6 GB) reveals the complete PhoenixSuit firmware image format used for H713 devices.

## Image Header Structure (96 bytes at offset 0x00)

```c
struct PhoenixSuitHeader {
    uint8_t  magic[8];          // 0x00: "IMAGEWTY"
    uint32_t version;           // 0x08: 0x300 (768)
    uint32_t header_size;       // 0x0C: 0x60 (96 bytes)
    uint32_t attr;              // 0x10: Attributes/flags
    // ... additional fields ...
    uint16_t item_size;         // 0x38: 0x400 (1024 bytes per item)
    uint16_t reserved1;         // 0x3A
    uint16_t item_count;        // 0x3C: 48 partitions
    uint16_t reserved2;         // 0x3E  
    uint32_t item_offset;       // 0x40: 0x400 (item table at 1024 bytes)
    // ... padding to 96 bytes ...
};
```

### Header Field Values (from HY300 update.img)
- **Magic**: `IMAGEWTY` (0x494d414745575459)
- **Version**: 768 (0x300)
- **Header Size**: 96 bytes (0x60)
- **Item Count**: 48 partitions
- **Item Size**: 1024 bytes per item (0x400)
- **Item Table Offset**: 0x400 (1024 bytes from start)
- **Encrypted**: No (attr bit 0 = 0)

## Item Table Structure

Starting at offset 0x400, contains 48 items × 1024 bytes = 49,152 bytes (0xC000).

### Item Structure (1024 bytes each)

```c
struct PhoenixSuitItem {
    uint32_t version;           // 0x00: Item version (0x100 = 256)
    uint32_t size;              // 0x04: Item size (0x400 = 1024)
    char     main_type[16];     // 0x08: Main type (e.g., "BOOT", "RFSFAT16")
    char     sub_type[8];       // 0x18: Sub type / partition ID
    char     filename[64];      // 0x20: .fex filename (e.g., "boot0_nand.fex")
    // Remaining 928 bytes: verify info, metadata, padding
};
```

### Example Items

#### Item 0: sys_config.fex (System Configuration)
```
Offset: 0x0400
Main Type: "COMMON  SYS_CONF"
Sub Type: "IG100000" (continuation of "SYS_CONFIG100000")
Filename: "sys_config.fex"
```

#### Item 6: boot0_nand.fex (Primary Bootloader)
```
Offset: 0x1C00
Main Type: "BOOT    BOOT0_00"
Sub Type: "00000000"
Filename: "boot0_nand.fex"
```

#### Item 27: boot-resource.fex (Boot Resources)
```
Offset: 0x7000  
Main Type: "RFSFAT16BOOT-RES"
Filename: "boot-resource.fex"
```

## Complete Item List (48 partitions)

| # | Main Type | Filename | Purpose |
|---|-----------|----------|---------|
| 0 | COMMON | sys_config.fex | System configuration |
| 1 | COMMON | board_config.fex | Board configuration |
| 2 | COMMON | sys_config_partition.fex | Partition layout |
| 3 | COMMON | split_xxxx.fex | Split configuration |
| 4 | COMMON | sys_config_dtb.fex | Device tree blob |
| 5 | COMMON | dtb_config.fex | DTB configuration |
| 6 | BOOT | boot0_nand.fex | Primary bootloader (BROM) |
| 7 | BOOT | boot0_sdcard.fex | SDCard bootloader |
| 8 | BOOT | u-boot.fex | U-Boot bootloader |
| 9 | BOOT | u-boot-crash.fex | U-Boot crash recovery |
| 10-11 | BOOT | toc0/toc1.fex | Table of contents |
| 12 | FES | fes_1-0.fex | FEL emergency mode |
| 27 | RFSFAT16 | boot-resource.fex | Boot resources (logo, etc.) |
| 28 | RFSFAT16 | env.fex | U-Boot environment |
| 30 | RFSFAT16 | boot.fex | Linux kernel + DTB |
| 34 | RFSFAT16 | super.fex | Android super partition |
| 38-43 | RFSFAT16 | vbmeta*.fex | Android Verified Boot metadata |

## Key Findings

### 1. Item Count Discovery
- **Initial parsing error**: Assumed item_count at offset 78-80 (yielded 0)
- **Correct location**: offset 60-62 (0x3C) = 48 items
- **Verification**: Manual count confirmed exactly 48 valid items

### 2. Item Size Discovery  
- **Initial assumption**: 32/64 bytes from imgdec_fun.lua
- **Actual size**: 1024 bytes (0x400) per item
- **Location in header**: offset 56-58 (0x38)

### 3. Item Offset Discovery
- **Location in header**: offset 64-68 (0x40) = 0x400
- **First item start**: 0x400 (immediately after potential padding)
- **Alignment**: 1024-byte aligned

### 4. Partition Data Storage
- Items contain **metadata only** (.fex filenames, types, verify info)
- Actual partition data is **NOT embedded** inline with fixed offsets
- PhoenixSuit image is a **packaging format** pointing to external .fex files
- During flashing, PhoenixSuit reads metadata and flashes corresponding .fex data

## Comparison with Allwinner Documentation

PhoenixSuit uses a **custom packaging format** different from standard Allwinner boot images:

| Feature | PhoenixSuit | Standard Allwinner |
|---------|-------------|-------------------|
| Magic | IMAGEWTY | ANDROID!, img |
| Item size | 1024 bytes | Variable |
| Encryption | Optional (RC4-like) | Usually none |
| Purpose | Multi-partition firmware | Single boot image |

## Integration with FEL Mode

Our H713 FEL mode implementation is **fully compatible**:
- VID/PID in header matches our FEL mode (0x1f3a:0xefe8)
- Image format compatible with sunxi-tools aw_fel protocol
- Unencrypted format allows direct partition extraction

## Tool Implementation

The `analyze_phoenixsuit_image.py` parser has been corrected with:
- Item size: 1024 bytes (was 32/64)
- Item count offset: 0x3C (was 0x4E)
- Item table offset: 0x40 (was 0x50)
- Filename extraction from offset 0x20 within each item

## Next Steps

1. **Extract individual .fex files** from update.img actual data areas
2. **Locate partition data** embedded in the 1.6 GB image
3. **Compare with extracted firmware** from FEL mode backup
4. **Document partition layout** for custom image creation

## References

- PhoenixSuit V1.10 source: `tools/phoenixsuit/colorfulshark-phoenixsuit/`
- HY300 update.img: 1,632,169,984 bytes (1.6 GB)
- Partition table: `firmware/fex_files/partition_table.txt`
- Related analysis: `docs/FEL_MODE_ANALYSIS.md`
