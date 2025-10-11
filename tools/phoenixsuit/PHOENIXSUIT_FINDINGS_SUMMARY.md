# PhoenixSuit H713 Reverse Engineering - Key Findings

## Executive Summary

Successfully downloaded and analyzed Allwinner PhoenixSuit V1.10, confirming that our HY300 factory firmware (`update.img`) uses the PhoenixSuit/LiveSuit image format. The update.img file contains the standard "IMAGEWTY" magic signature indicating PhoenixSuit compatibility.

## Critical Discoveries

### 1. Factory Firmware Format Confirmed ✅
**HY300 update.img uses PhoenixSuit image format:**
```
File: update.img  
Size: 1,632,169,984 bytes (1.5 GB)
Magic: IMAGEWTY (PhoenixSuit signature)
Version: 768
Encrypted: No
```

This means:
- Our factory firmware is standard Allwinner format
- Can be analyzed with PhoenixSuit tools
- Partition extraction is possible
- Image repacking capability available

### 2. USB FEL Mode Compatibility ✅
**Our H713 FEL mode matches PhoenixSuit protocol:**
- VID: 0x1f3a (Allwinner Technologies)  
- PID: 0xefe8 (FEL/USB Boot Mode)
- Standard Allwinner FEL protocol
- Compatible with our sunxi-tools-h713

### 3. Image Structure Analysis

**PhoenixSuit Image Format:**
```
Offset  | Size    | Description
--------|---------|----------------------------------
0x0000  | 96 B    | Image Header (IMAGEWTY magic)
0x0060  | Var     | Encryption key/padding (optional)
Varies  | Var     | Item Table (partition descriptors)
Varies  | Var     | Partition Data
```

**Header Structure (96 bytes):**
- Bytes 0-7: Magic signature ("IMAGEWTY")
- Bytes 8-11: Format version
- Bytes 12-15: Header size
- Bytes 16-19: Attributes (encryption flag in bit 0)
- Bytes 20-51: Image version string
- Bytes 52-87: Sizes, offsets, IDs
- Item count, item offset for partition table

## Tools Created

### 1. PhoenixSuit Image Analyzer
**Location:** `tools/analyze_phoenixsuit_image.py`

**Features:**
- Parse PhoenixSuit/LiveSuit image headers
- Display partition table information
- Extract individual partitions
- Hex dump for unknown formats

**Usage:**
```bash
# Analyze image structure
python3 tools/analyze_phoenixsuit_image.py update.img

# Extract specific partition
python3 tools/analyze_phoenixsuit_image.py extract update.img boot0 boot0.bin
```

### 2. PhoenixSuit V1.10 Download
**Location:** `tools/phoenixsuit/colorfulshark-phoenixsuit/`

**Components:**
- PhoenixSuit.exe - Main flashing tool
- eFex.dll - FEL/FEX protocol handler
- ImgDecode.dll - Image format decoder
- Drivers - Windows USB drivers

## Integration with HY300 Project

### Immediate Applications

1. **Partition Extraction**
   - Extract boot0, u-boot, kernel from update.img
   - Analyze partition layouts and sizes
   - Compare with our extracted firmware components

2. **Image Analysis**
   - Understand factory partition scheme
   - Identify bootloader versions
   - Extract configuration files

3. **Custom Image Creation**
   - Potential to create custom PhoenixSuit images
   - Package our mainline kernel and device tree
   - Flash via FEL mode or PhoenixSuit

### Documentation Updates

Already completed:
- ✅ Created `tools/phoenixsuit/ANALYSIS_SUMMARY.md`
- ✅ Created `tools/phoenixsuit/DOWNLOAD_SOURCES.md`
- ✅ Created `tools/analyze_phoenixsuit_image.py`
- ✅ Confirmed update.img uses PhoenixSuit format

Still needed:
- [ ] Extract and document partition layout from update.img
- [ ] Update FEL_MODE_ANALYSIS.md with PhoenixSuit protocol details
- [ ] Cross-reference with FIRMWARE_COMPONENTS_ANALYSIS.md
- [ ] Add image format details to device tree documentation

## Next Research Steps

### Priority 1: Complete Image Analysis
1. **Fix partition table parsing** in analyze_phoenixsuit_image.py
   - Current issue: item_size = 0, item_count = 0
   - Need to locate actual item table in update.img
   - Offset may differ from standard header location

2. **Extract all partitions** from update.img
   - boot0 (primary bootloader)
   - u-boot (secondary bootloader)
   - kernel (Linux kernel)
   - system (root filesystem)
   - Display MIPS firmware
   - Other partitions

3. **Compare extractions** with our previous binwalk analysis
   - Validate extraction accuracy
   - Identify any missing components
   - Cross-reference partition boundaries

### Priority 2: Search for H713-Specific PhoenixSuit
Current version (V1.10) predates H713 release. Search for:
- PhoenixSuit V1.18 or later
- H713-specific configurations
- Updated drivers with H713 support
- Chinese OEM sources for newer versions

**Search locations:**
- Allwinner developer portal (requires account)
- Chinese forums: 4PDA.ru, ZNDS.com
- XDA Developers
- GitHub: search "phoenixsuit h713"

### Priority 3: Protocol Reverse Engineering
If needed for advanced analysis:
- Decompile eFex.dll and Phoenix_Fes.dll
- Compare with sunxi-tools FEL implementation
- Document H713-specific commands (if any)
- USB protocol capture analysis

## Comparison: PhoenixSuit vs sunxi-tools

| Feature | PhoenixSuit | sunxi-tools-h713 |
|---------|-------------|------------------|
| License | Proprietary | Open Source (GPL) |
| Platform | Windows | Linux/macOS |
| Image Format | Pack/Unpack | Read-only analysis |
| FEL Protocol | Full support | Full support |
| GUI | Yes | No (CLI only) |
| H713 Support | Unknown (V1.10) | Yes (custom) |
| Encryption | Yes | Limited |

**Conclusion:** Both tools complement each other:
- PhoenixSuit for image manipulation
- sunxi-tools for FEL mode operations

## Security and Legal Considerations

**Important:**
- PhoenixSuit is proprietary Allwinner software
- Reverse engineering for interoperability (fair use)
- Do not redistribute proprietary components
- Use findings only for HY300 Linux porting
- Document all information sources

## References

**Project Files:**
- `tools/phoenixsuit/` - PhoenixSuit download and analysis
- `tools/analyze_phoenixsuit_image.py` - Image parser
- `update.img` - Factory firmware (1.5 GB)
- Task 033: PhoenixSuit H713 Reverse Engineering

**External Resources:**
- GitHub: colorfulshark/PhoenixSuit
- linux-sunxi.org - Community documentation
- androidmtk.com - Tool packaging source

**Related Documentation:**
- `docs/FEL_MODE_ANALYSIS.md`
- `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`
- `docs/HY300_TESTING_METHODOLOGY.md`
