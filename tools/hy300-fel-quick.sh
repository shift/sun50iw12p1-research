#!/usr/bin/env bash
# Quick FEL backup script for rapid testing
# Extracts essential components only for fast iteration
# Usage: sudo ./tools/hy300-fel-quick.sh [output_dir]

set -e

SUNXI_FEL="./sunxi-fel-h713"
OUTPUT_DIR="${1:-fel_quick_$(date +%Y%m%d_%H%M%S)}"
FEL_DEVICE_ID="1f3a:efe8"

echo "HY300 Quick FEL Backup"
echo "======================"

# Check binary
if [ ! -f "$SUNXI_FEL" ]; then
    echo "ERROR: sunxi-fel-h713 not found"
    exit 1
fi

# Check root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: Run as root (sudo)"
    exit 1
fi

# Set permissions
FEL_DEV=$(lsusb | grep "$FEL_DEVICE_ID" | awk '{print $2"/"$4}' | tr -d ':')
if [ -n "$FEL_DEV" ]; then
    chmod 666 /dev/bus/usb/$FEL_DEV 2>/dev/null
fi

# Test connection
echo "Testing FEL connection..."
if ! $SUNXI_FEL version >/dev/null 2>&1; then
    echo "ERROR: Cannot connect to FEL device"
    exit 1
fi

echo "✓ Connected"

# Create output
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

echo "Extracting to: $(pwd)"
echo ""

# Essential components only
echo "[1/4] SoC info..."
$SUNXI_FEL version > soc_info.txt 2>&1
echo "  ✓ soc_info.txt"

echo "[2/4] Chip ID..."
$SUNXI_FEL sid > chip_id.txt 2>&1 || echo "N/A" > chip_id.txt
echo "  ✓ chip_id.txt"

echo "[3/4] boot0..."
$SUNXI_FEL read 0x0 32768 boot0.bin >/dev/null 2>&1
if [ -f boot0.bin ]; then
    echo "  ✓ boot0.bin ($(stat -c%s boot0.bin) bytes)"
else
    echo "  ✗ boot0.bin failed"
fi

echo "[4/4] Manifest..."
cat > manifest.txt << EOF
Quick FEL Backup - $(date)
SoC: H713 (0x1860)
Files: soc_info.txt, chip_id.txt, boot0.bin
Use hy300-fel-backup.sh for complete backup
EOF
echo "  ✓ manifest.txt"

echo ""
echo "Quick backup complete!"
ls -lh

# Return to original directory
cd - >/dev/null
echo ""
echo "Backup saved to: $(pwd)/$OUTPUT_DIR"
