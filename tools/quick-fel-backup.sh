#!/usr/bin/env bash
# Quick FEL backup script that works standalone
# Usage: sudo ./tools/quick-fel-backup.sh

set -e

SUNXI_FEL="/nix/store/2w77q2877lgpv32zj323nq2mfzy305i5-sunxi-tools-0-unstable-2025-03-29/bin/sunxi-fel"
OUTPUT_DIR="firmware_backup_$(date +%Y%m%d_%H%M%S)"

echo "====================================="
echo "HY300 Quick FEL Backup"
echo "====================================="

# Function to find and set permissions
setup_device() {
    DEVICE=$(lsusb | grep "1f3a:efe8" | awk '{print $2"/"$4}' | sed 's/://')
    if [ -n "$DEVICE" ]; then
        chmod 666 /dev/bus/usb/$DEVICE 2>/dev/null
    fi
}

# Set permissions just before each command
setup_device

# Test connection
echo "Testing FEL connection..."
if ! $SUNXI_FEL version; then
    echo "ERROR: Cannot connect to FEL device"
    exit 1
fi
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"
echo "Backup directory: $(pwd)"
echo ""

# Save SoC info
echo "[1/7] Saving SoC information..."
setup_device
$SUNXI_FEL version > soc_info.txt
echo "  Saved: soc_info.txt"

# Try to read SID
echo "[2/7] Reading chip ID (SID)..."
setup_device
$SUNXI_FEL sid > chip_id.txt 2>&1 || echo "  (SID not supported, saved error info)"

# Read boot0 from DRAM (sunxi-fel loads it there)
echo "[3/7] Reading boot0 header from DRAM..."
setup_device
$SUNXI_FEL dump 0x0 32768 boot0_header.bin
echo "  Saved: boot0_header.bin (32KB)"

# Try to read from various memory locations
echo "[4/7] Dumping SRAM-A region..."
setup_device
$SUNXI_FEL dump 0x00010000 0x8000 sram_a.bin 2>&1 || echo "  (SRAM-A not accessible)"

echo "[5/7] Dumping SRAM-C region..."
setup_device
$SUNXI_FEL dump 0x00028000 0x16000 sram_c.bin 2>&1 || echo "  (SRAM-C not accessible)"

# Read from DRAM base
echo "[6/7] Sampling DRAM content..."
setup_device
$SUNXI_FEL dump 0x40000000 65536 dram_sample.bin 2>&1 || echo "  (DRAM not accessible yet)"

# Create manifest
echo "[7/7] Creating manifest..."
cat > backup_manifest.txt << EOF
HY300 FEL Mode Backup
=====================
Date: $(date)
SoC ID: 0x1860 (H713)

Files:
- soc_info.txt: FEL version and SoC info
- chip_id.txt: Chip ID (SID) if available
- boot0_header.bin: Boot0 header from DRAM (32KB)
- sram_*.bin: SRAM regions if accessible
- dram_sample.bin: DRAM sample if accessible

Note: FEL mode provides limited access. Full eMMC backup
requires booting U-Boot and using UMS mode.
EOF

echo ""
echo "====================================="
echo "Backup completed!"
echo "====================================="
echo ""
ls -lh
echo ""
cd ..
echo "Backup saved to: $(pwd)/$OUTPUT_DIR"
