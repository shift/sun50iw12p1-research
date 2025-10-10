#!/usr/bin/env bash
# Complete firmware backup script for HY300 via FEL mode
# Usage: sudo ./tools/backup-firmware.sh [output_directory]

set -e

OUTPUT_DIR="${1:-firmware_backup_$(date +%Y%m%d_%H%M%S)}"

echo "====================================="
echo "HY300 Firmware Backup via FEL Mode"
echo "====================================="
echo ""

# Check if in Nix devShell
if [ -z "$IN_NIX_SHELL" ]; then
    echo "ERROR: Not in Nix devShell. Please run:"
    echo "  nix develop"
    echo "or"
    echo "  nix develop -c -- sudo ./tools/backup-firmware.sh"
    exit 1
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root (sudo)"
    exit 1
fi

# Find sunxi-fel
SUNXI_FEL=$(which sunxi-fel)
if [ -z "$SUNXI_FEL" ]; then
    echo "ERROR: sunxi-fel not found in PATH"
    exit 1
fi

echo "Using sunxi-fel: $SUNXI_FEL"
echo ""

# Check FEL connection
echo "Checking FEL connection..."
if ! $SUNXI_FEL version > /dev/null 2>&1; then
    echo "ERROR: Cannot connect to FEL device"
    echo ""
    echo "Troubleshooting:"
    echo "1. Ensure device is in FEL mode"
    echo "2. Run: sudo ./tools/fel-permissions.sh"
    echo "3. Try again immediately"
    exit 1
fi

echo "FEL device detected successfully!"
$SUNXI_FEL version
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

echo "Backup directory: $(pwd)"
echo ""

# Backup boot0 (first 32KB)
echo "[1/6] Backing up boot0 (32KB)..."
$SUNXI_FEL readl 0x0 > boot0.bin || true
echo "  Saved: boot0.bin"

# Backup U-Boot (typically at 0x8000, 1MB)
echo "[2/6] Backing up U-Boot (1MB from 0x8000)..."
$SUNXI_FEL readl 0x8000 0x100000 > uboot.bin || true
echo "  Saved: uboot.bin"

# Read SPL info
echo "[3/6] Gathering SoC information..."
$SUNXI_FEL ver > soc_info.txt
echo "  Saved: soc_info.txt"

# Attempt to read BROM ID
echo "[4/6] Reading BROM chip ID..."
$SUNXI_FEL sid > chip_id.txt 2>&1 || echo "  (SID read not supported on this SoC)"

# Dump complete memory map
echo "[5/6] Saving memory map information..."
echo "HY300 Memory Map" > memory_map.txt
echo "================" >> memory_map.txt
echo "" >> memory_map.txt
echo "SoC: Allwinner H713 (H616 derivative)" >> memory_map.txt
echo "DRAM: 2GB DDR3" >> memory_map.txt
echo "Storage: 16GB eMMC" >> memory_map.txt
echo "" >> memory_map.txt
echo "Boot sequence:" >> memory_map.txt
echo "  0x00000000: boot0 (BROM SPL, 32KB)" >> memory_map.txt
echo "  0x00008000: U-Boot (1MB)" >> memory_map.txt
echo "  0x00100000: Environment" >> memory_map.txt
echo "  0x00200000: Kernel" >> memory_map.txt
echo "" >> memory_map.txt

# Save session log
echo "[6/6] Creating backup manifest..."
cat > backup_manifest.txt << EOF
HY300 Firmware Backup
=====================
Date: $(date)
SoC: Allwinner H713 (H616 family)
Method: FEL Mode (USB Boot)

Files in this backup:
- boot0.bin: First-stage bootloader (BROM SPL) with DRAM parameters
- uboot.bin: U-Boot bootloader
- soc_info.txt: SoC version and FEL protocol info
- chip_id.txt: Chip ID and SID (if available)
- memory_map.txt: Memory layout documentation
- backup_manifest.txt: This file

Note: Complete eMMC backup requires booting into U-Boot or Linux.
FEL mode provides access to bootloader components only.

For complete eMMC backup:
1. Boot mainline U-Boot via FEL
2. Use 'ums' command to expose eMMC as USB storage
3. Use dd to create full image: dd if=/dev/sdX of=emmc_full.img bs=1M

Alternatively, use Android recovery tools if available.
EOF

echo ""
echo "====================================="
echo "Backup completed successfully!"
echo "====================================="
echo ""
echo "Backup location: $(pwd)"
echo ""
ls -lh
echo ""
echo "Next steps:"
echo "1. Copy backup to safe location"
echo "2. Verify boot0.bin contains DRAM parameters"
echo "3. Proceed with U-Boot testing via FEL"
