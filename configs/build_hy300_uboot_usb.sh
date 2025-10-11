#!/usr/bin/env bash
# Build U-Boot for HY300 with USB Serial Console Support

set -e

UBOOT_DIR="u-boot"
DEFCONFIG="hy300_h713_defconfig"
CROSS_COMPILE="aarch64-unknown-linux-gnu-"

echo "Building U-Boot for HY300 with USB serial gadget support..."

# Check if U-Boot directory exists
if [ ! -d "$UBOOT_DIR" ]; then
    echo "Error: U-Boot directory not found at $UBOOT_DIR"
    exit 1
fi

cd "$UBOOT_DIR"

# Clean previous build
echo "Cleaning previous build..."
make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE mrproper

# Copy defconfig to U-Boot configs directory
echo "Installing HY300 defconfig..."
cp ../configs/hy300_h713_defconfig configs/

# Configure U-Boot
echo "Configuring U-Boot with $DEFCONFIG..."
make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE $DEFCONFIG

# Verify USB gadget configs are enabled
echo "Verifying USB gadget configuration..."
grep -q "CONFIG_USB_GADGET=y" .config && echo "✓ USB_GADGET enabled"
grep -q "CONFIG_USB_FUNCTION_ACM=y" .config && echo "✓ USB_FUNCTION_ACM enabled"
grep -q "CONFIG_DM_USB_GADGET=y" .config && echo "✓ DM_USB_GADGET enabled"

# Build U-Boot
echo "Building U-Boot (this may take a few minutes)..."
make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE -j$(nproc)

# Check build output
if [ -f "u-boot-sunxi-with-spl.bin" ]; then
    echo "✅ Build successful!"
    ls -lh u-boot-sunxi-with-spl.bin
    
    # Copy to project root
    cp u-boot-sunxi-with-spl.bin ../u-boot-sunxi-with-spl-usb.bin
    echo "✅ Copied to: u-boot-sunxi-with-spl-usb.bin"
    
    echo ""
    echo "Next steps:"
    echo "1. Test via FEL mode:"
    echo "   sunxi-fel uboot u-boot-sunxi-with-spl-usb.bin"
    echo "2. Check host for /dev/ttyACM0"
    echo "3. Connect with: screen /dev/ttyACM0 115200"
else
    echo "❌ Build failed! Check errors above."
    exit 1
fi
