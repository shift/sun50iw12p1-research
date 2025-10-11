# PhoenixSuit Analysis Summary

## Downloaded Version
- **Version**: PhoenixSuit V1.10 (version code: 1100)
- **Source**: GitHub repository colorfulshark/PhoenixSuit
- **Repository**: https://github.com/colorfulshark/PhoenixSuit
- **Date Packaged**: Packed by androidmtk.com

## Key Findings

### USB Communication
**FEL Mode USB IDs:**
- VID: `0x1f3a` (Allwinner Technologies)
- PID: `0xefe8` (FEL/USB Boot Mode)
- **Match with our H713**: ✅ Confirmed - Same VID/PID as our device

**LiveSuit/PhoenixSuit IDs:**
- VID: `0x1f3a`
- PID: `0x1000` (Mass Storage/Flash Mode)

### Driver Analysis
**Location**: `Drivers/AW_Driver/usbdrv.inf`
- Driver Date: January 24, 2013
- Driver Version: 1.0.0.1
- Supports: Windows x86, x64, IA64
- Service Name: `usbUDisc`

### Tool Components

**Core Executables:**
- `PhoenixSuit.exe` - Main flashing tool (2.2 MB, PE32)
- `PhoenixDaemon.exe` - Background service (797 KB)
- `PhoenixDrvInstall.exe` - Driver installer (1.7 MB)
- `adb.exe` - Android Debug Bridge (797 KB)

**Key DLLs:**
- `eFex.dll` - FEL/FEX protocol handler (44 KB)
- `Phoenix_Fes.dll` - FES protocol implementation (36 KB)
- `ImgDecode.dll` - Image format decoder (44 KB)
- `ImgDecode32.dll` / `ImgDecode64.dll` - Platform-specific decoders
- `KSDecode.dll` - Encryption/signing handler (100 KB)

**Lua Scripting System:**
- `lua5.1.dll` - Lua 5.1 runtime (617 KB)
- `luaBase.dll` - Base functions (128 KB)
- `luaDec.dll` - Decryption functions (36 KB)
- `luaeFex.dll` - eFex Lua bindings (40 KB)
- Compiled scripts: `imgdec_fun.lua`, `ini_fun.lua`, `common_fun.lua`

### Firmware Image Format

**Image Structure (from imgdec_fun.lua analysis):**
```
IMAGE_HEAD (96 bytes):
  - MAGIC: Image signature
  - VERSION: Image format version
  - SIZE: Total image size
  - ATTR: Image attributes
  - IMG_VERSION: Firmware version
  - LENLO/LENHI: 64-bit length
  - PID: Product ID
  - VID: Vendor ID
  - HARDWAREID: Hardware identifier
  - FIRMWAREID: Firmware identifier
  - ITEMATTR: Item attributes
  - ITEMSIZE: Size of each item
  - ITEMCNT: Number of items/partitions
  - ITEMOFFSET: Offset to item table
  - IMAGEATTR: Additional attributes
  - APPENDSIZE: Append data size
  - APPENDOFFSET: Append data offset

ITEM_TABLE (variable):
  Each item (64/32 bytes depending on version):
    - VERSION: Item format version
    - SIZE: Item size
    - MAINTYPE: Main type identifier
    - SUBTYPE: Subtype identifier
    - ATTR: Item attributes
    - DATALENLO/HI: 64-bit data length
    - FILELENLO/HI: 64-bit file length
    - OFFSETLO/HI: 64-bit offset in image
    - CHECKSUM: Item checksum
    - NAME: Item name string
    - ENCRYPTID: Encryption ID
```

**Constants:**
- HEAD_ID = 1
- TABLE_ID = 2
- DATA_ID = 3
- IMAGE_HEAD_SIZE = 96 bytes
- IMAGE_ITEM_SIZE = 64 bytes (v2) or 32 bytes (v1)
- ENCODE_LEN = 16 bytes (encryption block size)

**Supported Item Types:**
- ITEM_PHOENIX_TOOLS (special tools partition)
- Standard partitions: boot0, boot, system, etc.

### Encryption/Security
- **Encryption**: Optional RC4-like encryption (ENCODE_LEN = 16)
- **Global Flag**: `g_bEncypt` determines if decryption needed
- **Key System**: Uses `LiveSuit.dat` as key file (MAX_KEY_SIZE = 64)
- **Signing**: Handled by `KSDecode.dll`

### Version History
Available versions: 1.0.0.3, 1.0.0.4, 1.0.0.5, 1.02 through 1.10

**Major Changes:**
- V1.04: Added ADB support
- V1.05: Updated AW_Driver support
- V1.08: Added plugin system (AwPluginVector.dll, LangPlg.dll)
- V1.10: Current version - Added checkver.ulf

## H713 Support Analysis

### Current Status
**PhoenixSuit V1.10 Analysis:**
- ❓ **No explicit H713 references found** in strings or configuration files
- ✅ **USB VID/PID matches** our H713 FEL mode (0x1f3a:0xefe8)
- ⚠️ **Tool is from 2019** - predates H713 release
- 🔍 **Newer versions may exist** - need to search for V1.18+ with H713 support

### Comparison with Our Tools
**sunxi-tools vs PhoenixSuit:**
- Both use same USB VID/PID for FEL mode
- sunxi-tools is open source, PhoenixSuit is proprietary
- PhoenixSuit has image packing/unpacking functionality
- PhoenixSuit includes encryption/signing support

### Actionable Insights

#### 1. Image Format Compatibility
Our factory ROM (`/root/hy300-android-rom.img`) likely uses this format:
- 96-byte header with magic signature
- Item table describing partitions
- Optional RC4-like encryption per block
- Can be analyzed with custom Python tool

#### 2. FEL Protocol Compatibility
- USB communication protocol likely similar
- Standard Allwinner FEL commands
- Our sunxi-tools-h713 implementation should be compatible

#### 3. Firmware Packaging
Could extract or create images using this format:
- Boot0 (primary bootloader)
- U-Boot (secondary bootloader)
- Boot (kernel + dtb)
- System (rootfs)
- Custom partitions

## Next Steps

### Immediate Actions
1. ✅ **Analyze factory ROM structure** - Check for PhoenixSuit image format
2. 🔄 **Search for newer PhoenixSuit versions** - Look for V1.18+ with explicit H713 support
3. 🔄 **Create image parser** - Python tool to extract/analyze PhoenixSuit images
4. 🔄 **Compare protocols** - Analyze differences between PhoenixSuit and sunxi-tools

### Future Investigation
1. **Decompile DLLs** - Use Ghidra/IDA to reverse engineer eFex.dll and Phoenix_Fes.dll
2. **Protocol capture** - Use USB protocol analyzer to capture PhoenixSuit communications
3. **LiveSuit Linux** - Investigate Linux version (if available) for easier analysis
4. **Image creation** - Build custom images for H713 testing

### Research Directions
1. **Search online for:**
   - "PhoenixSuit H713"
   - "PhoenixSuit V1.18"
   - "Allwinner H713 flashing tool"
   - Chinese forums: "全志 H713" (Allwinner H713)

2. **Alternative sources:**
   - Allwinner developer portal (requires account)
   - Chinese OEM websites
   - XDA Developers forums
   - 4PDA.ru (Russian community)

3. **Community resources:**
   - linux-sunxi.org forums
   - Armbian forums
   - Reddit r/SBCs

## Integration with HY300 Project

### Applicable Knowledge
1. **USB Communication**
   - Confirmed FEL mode VID/PID
   - Standard Allwinner USB boot protocol
   - Compatible with our sunxi-tools-h713

2. **Firmware Structure**
   - Understanding of partition layout
   - Image format for potential custom ROMs
   - Encryption/signing mechanisms

3. **Testing Approach**
   - PhoenixSuit can flash via USB (FEL mode)
   - Could use for firmware recovery
   - Backup/restore functionality

### Documentation Updates Required
- [ ] Update `docs/FEL_MODE_ANALYSIS.md` with PhoenixSuit findings
- [ ] Cross-reference with `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`
- [ ] Add image format details to `docs/FACTORY_DTB_ANALYSIS.md`
- [ ] Update task 028 with findings and next steps

## Tools to Build

### Priority 1: Image Analyzer
```python
#!/usr/bin/env python3
"""
PhoenixSuit Image Analyzer for HY300 Project
Parses and extracts PhoenixSuit/LiveSuit .img files
"""

def parse_image_header(data):
    """Parse 96-byte PhoenixSuit image header"""
    # Extract magic, version, sizes, IDs
    pass

def extract_item_table(data, offset, count):
    """Extract partition item table"""
    pass

def decrypt_data(data, key):
    """Decrypt RC4-like encrypted blocks"""
    pass

def extract_partition(img_file, item_name, output_file):
    """Extract specific partition by name"""
    pass
```

### Priority 2: Protocol Analyzer
- USB packet capture comparison
- FEL command sequence documentation
- H713-specific command discovery

## Security Considerations

**Important Notes:**
- PhoenixSuit is proprietary Allwinner software
- Reverse engineering may have legal implications
- Use findings only for HY300 Linux porting project
- Do not redistribute Allwinner proprietary code
- Document provenance of all information

## References

**Downloaded Tools:**
- Location: `/home/shift/code/android_projector/tools/phoenixsuit/`
- Repository: `colorfulshark-phoenixsuit/`

**External Resources:**
- androidmtk.com - Original packer
- linux-sunxi.org - Community documentation
- GitHub: colorfulshark/PhoenixSuit

**Related Project Documentation:**
- `docs/FEL_MODE_ANALYSIS.md`
- `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`
- `docs/HY300_TESTING_METHODOLOGY.md`
- Task 028: PhoenixSuit H713 Reverse Engineering
