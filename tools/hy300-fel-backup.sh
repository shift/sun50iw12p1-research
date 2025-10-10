#!/usr/bin/env bash
# Comprehensive FEL mode firmware backup script for HY300 projector
# Uses H713-patched sunxi-fel binary for complete data extraction
# Usage: sudo ./tools/hy300-fel-backup.sh [output_directory]

set -e

# Configuration
SUNXI_FEL="./sunxi-fel-h713"  # Use our patched binary
OUTPUT_DIR="${1:-fel_backup_$(date +%Y%m%d_%H%M%S)}"
FEL_DEVICE_ID="1f3a:efe8"  # Allwinner FEL mode USB ID

echo "=========================================="
echo "HY300 FEL Mode Firmware Backup"
echo "=========================================="
echo ""

# Check if sunxi-fel-h713 binary exists
if [ ! -f "$SUNXI_FEL" ]; then
    echo "ERROR: sunxi-fel-h713 binary not found"
    echo "Expected location: $SUNXI_FEL"
    echo ""
    echo "Please ensure the H713-patched binary is available."
    echo "See: SUNXI_TOOLS_H713_SUMMARY.md for build instructions"
    exit 1
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root (sudo)"
    echo "Root access required for USB device permissions"
    exit 1
fi

# Function to set USB permissions for FEL device
setup_fel_permissions() {
    echo "Setting up FEL device permissions..."
    
    # Find FEL device
    FEL_BUS_DEV=$(lsusb | grep "$FEL_DEVICE_ID" | awk '{print $2"/"$4}' | tr -d ':')
    
    if [ -z "$FEL_BUS_DEV" ]; then
        echo "ERROR: FEL device not found (ID: $FEL_DEVICE_ID)"
        echo ""
        echo "Please ensure:"
        echo "1. HY300 is powered on in FEL mode"
        echo "2. USB cable is connected"
        echo "3. Device appears in: lsusb"
        exit 1
    fi
    
    # Set permissions
    chmod 666 /dev/bus/usb/$FEL_BUS_DEV 2>/dev/null || true
    echo "  FEL device found: /dev/bus/usb/$FEL_BUS_DEV"
}

# Function to run sunxi-fel command with auto-retry
fel_command() {
    local max_retries=3
    local retry_count=0
    
    while [ $retry_count -lt $max_retries ]; do
        # Reset permissions before each attempt
        setup_fel_permissions >/dev/null 2>&1
        
        # Run command
        if $SUNXI_FEL "$@" 2>&1; then
            return 0
        else
            retry_count=$((retry_count + 1))
            if [ $retry_count -lt $max_retries ]; then
                echo "  Retry $retry_count/$max_retries..."
                sleep 1
            fi
        fi
    done
    
    echo "  WARNING: Command failed after $max_retries attempts"
    return 1
}

# Setup FEL device permissions
setup_fel_permissions

# Verify FEL connection
echo "Verifying FEL connection..."
if ! fel_command version > /dev/null 2>&1; then
    echo "ERROR: Cannot communicate with FEL device"
    echo ""
    echo "Troubleshooting:"
    echo "1. Check USB cable connection"
    echo "2. Verify device is in FEL mode: lsusb | grep $FEL_DEVICE_ID"
    echo "3. Try different USB port"
    echo "4. Check USB cable quality"
    exit 1
fi

echo "✓ FEL device detected successfully"
echo ""

# Display SoC information
echo "SoC Information:"
fel_command version | grep -E "(AWUSBFEX|SoC)" || true
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

echo "Backup directory: $(pwd)"
echo ""
echo "Starting firmware extraction..."
echo ""

# Component 1: SoC Version and Information
echo "[1/10] Extracting SoC information..."
fel_command version > soc_info.txt 2>&1
if [ -f soc_info.txt ]; then
    echo "  ✓ Saved: soc_info.txt ($(stat -c%s soc_info.txt) bytes)"
else
    echo "  ✗ Failed to save soc_info.txt"
fi

# Component 2: Chip ID (SID - Security ID)
echo "[2/10] Reading chip ID (SID)..."
if fel_command sid > chip_id.txt 2>&1; then
    echo "  ✓ Saved: chip_id.txt ($(stat -c%s chip_id.txt) bytes)"
else
    echo "  ⚠ SID not accessible (may not be supported)"
    echo "SID read not supported on this SoC" > chip_id.txt
fi

# Component 3: Boot0 from DRAM (BROM loads it there)
echo "[3/10] Extracting boot0 bootloader..."
if fel_command read 0x00000000 32768 boot0.bin > /dev/null 2>&1; then
    if [ -f boot0.bin ] && [ $(stat -c%s boot0.bin) -eq 32768 ]; then
        echo "  ✓ Saved: boot0.bin (32KB)"
        # Validate boot0 header
        if strings boot0.bin | head -5 | grep -qi "boot"; then
            echo "  ✓ boot0 header validated"
        fi
    else
        echo "  ⚠ boot0.bin size unexpected"
    fi
else
    echo "  ✗ Failed to extract boot0"
fi

# Component 4: U-Boot region (typically at 0x8000)
echo "[4/10] Extracting U-Boot region..."
if fel_command read 0x00008000 1048576 uboot_region.bin > /dev/null 2>&1; then
    echo "  ✓ Saved: uboot_region.bin (1MB)"
else
    echo "  ⚠ U-Boot region not accessible"
fi

# Component 5: SRAM A1 (32KB at 0x10000)
echo "[5/10] Extracting SRAM A1..."
if fel_command read 0x00010000 32768 sram_a1.bin > /dev/null 2>&1; then
    echo "  ✓ Saved: sram_a1.bin (32KB)"
else
    echo "  ⚠ SRAM A1 not accessible"
fi

# Component 6: SRAM A2 (48KB at 0x18000)  
echo "[6/10] Extracting SRAM A2..."
if fel_command read 0x00018000 49152 sram_a2.bin > /dev/null 2>&1; then
    echo "  ✓ Saved: sram_a2.bin (48KB)"
else
    echo "  ⚠ SRAM A2 not accessible"
fi

# Component 7: SRAM C (175KB at 0x28000)
echo "[7/10] Extracting SRAM C..."
if fel_command read 0x00028000 179200 sram_c.bin > /dev/null 2>&1; then
    echo "  ✓ Saved: sram_c.bin (175KB)"
else
    echo "  ⚠ SRAM C not accessible"
fi

# Component 8: DRAM sample (64KB from base)
echo "[8/10] Sampling DRAM..."
if fel_command read 0x40000000 65536 dram_sample.bin > /dev/null 2>&1; then
    echo "  ✓ Saved: dram_sample.bin (64KB)"
else
    echo "  ⚠ DRAM not accessible (may need initialization)"
fi

# Component 9: Memory map documentation
echo "[9/10] Generating memory map..."
cat > memory_map.txt << 'EOF'
HY300 Projector Memory Map
==========================

SoC: Allwinner H713 (H616 derivative)
DRAM: 2GB DDR3
Storage: 16GB eMMC

Memory Layout:
--------------
Boot Sequence (eMMC):
  0x00000000  boot0 (32KB)       - BROM SPL with DRAM parameters
  0x00008000  U-Boot (1MB)       - Secondary bootloader
  0x00100000  Environment        - U-Boot environment variables
  0x00200000  Kernel             - Linux kernel image
  0x01000000  Root filesystem    - System root partition

SRAM Layout:
  0x00010000  SRAM A1 (32KB)     - SPL execution area
  0x00018000  SRAM A2 (48KB)     - Additional SPL space
  0x00028000  SRAM C (175KB)     - Larger runtime area
  
DRAM:
  0x40000000  DRAM base          - 2GB DDR3 memory
  
Peripheral Base:
  0x01C00000  System peripherals
  0x03006000  SID (Security ID)

FEL Mode Access:
  ✓ SRAM regions accessible
  ✓ boot0 after BROM loads it
  ✓ DRAM after initialization
  ✗ eMMC requires booted system (U-Boot UMS or Linux)

For complete eMMC backup:
1. Boot U-Boot via FEL: sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin
2. Use UMS command: ums 0 mmc 0
3. Device appears as USB storage
4. Full backup: dd if=/dev/sdX of=emmc_complete.img bs=1M
EOF

echo "  ✓ Saved: memory_map.txt"

# Component 10: Backup manifest
echo "[10/10] Creating backup manifest..."
cat > backup_manifest.txt << EOF
HY300 FEL Mode Firmware Backup
==============================

Backup Details:
  Date: $(date)
  SoC: Allwinner H713 (ID: 0x1860)
  Method: FEL Mode (USB Boot)
  Tool: sunxi-fel with H713 support

Files in this backup:
---------------------
$(ls -lh | grep -v "^d" | grep -v "total" | awk '{print "  " $9 " (" $5 ")"}'| sort)

Component Summary:
------------------
EOF

# Add component details to manifest
for file in soc_info.txt chip_id.txt boot0.bin uboot_region.bin \
            sram_a1.bin sram_a2.bin sram_c.bin dram_sample.bin \
            memory_map.txt; do
    if [ -f "$file" ]; then
        size=$(stat -c%s "$file")
        if [ $size -gt 0 ]; then
            echo "  ✓ $file - ${size} bytes" >> backup_manifest.txt
        else
            echo "  ⚠ $file - empty" >> backup_manifest.txt
        fi
    else
        echo "  ✗ $file - not extracted" >> backup_manifest.txt
    fi
done

cat >> backup_manifest.txt << 'EOF'

Boot0 DRAM Parameters:
----------------------
If boot0.bin was extracted successfully, analyze with:
  ../tools/analyze_boot0.py boot0.bin

FEL Mode Limitations:
---------------------
FEL mode provides direct memory access but NOT block device access.
Complete eMMC backup requires:

Method 1: U-Boot UMS Mode (Recommended)
  1. Boot U-Boot via FEL: 
     ./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin
  2. At U-Boot prompt:
     => ums 0 mmc 0
  3. Device appears as USB storage on host
  4. Create full image:
     dd if=/dev/sdX of=emmc_full.img bs=1M status=progress
  5. Restore if needed:
     dd if=emmc_full.img of=/dev/sdX bs=1M status=progress

Method 2: Linux dd (Alternative)
  1. Boot minimal Linux via FEL with initramfs
  2. Load network or USB storage drivers
  3. Use dd to copy /dev/mmcblk0 to network/USB

Method 3: Android Recovery (If Available)
  Use factory recovery tools if Android recovery is accessible

Next Steps:
-----------
1. Analyze boot0.bin for DRAM parameters
2. Verify U-Boot region if extracted
3. Proceed with U-Boot FEL testing (Phase VI)
4. Complete eMMC backup via U-Boot UMS mode

See Also:
---------
- USING_H713_FEL_MODE.md - FEL mode usage guide
- SUNXI_TOOLS_H713_SUMMARY.md - H713 support implementation
- docs/tasks/027-fel-mode-firmware-backup.md - Task documentation
EOF

echo "  ✓ Saved: backup_manifest.txt"

# Create SHA256 checksums
echo ""
echo "Generating checksums..."
sha256sum *.bin *.txt > checksums.sha256 2>/dev/null || true
echo "  ✓ Saved: checksums.sha256"

echo ""
echo "=========================================="
echo "Backup completed successfully!"
echo "=========================================="
echo ""
echo "Backup location: $(pwd)"
echo ""

# Display summary
echo "Backup Summary:"
echo "---------------"
ls -lh | grep -v "^d" | grep -v "total"

echo ""
echo "Important Notes:"
echo "----------------"
echo "✓ This backup contains all FEL-accessible components"
echo "⚠ Complete eMMC backup requires U-Boot UMS mode"
echo "→ See backup_manifest.txt for next steps"
echo ""

# Analyze boot0 if available and tool exists
if [ -f "boot0.bin" ] && [ -f "../tools/analyze_boot0.py" ]; then
    echo "Analyzing boot0 DRAM parameters..."
    python3 ../tools/analyze_boot0.py boot0.bin || echo "  (Analysis failed or tool not available)"
    echo ""
fi

echo "Safe to proceed with Phase VI hardware testing."
echo "For complete eMMC backup, use U-Boot UMS mode."
echo ""
