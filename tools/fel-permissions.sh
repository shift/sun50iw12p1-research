#!/usr/bin/env bash
# Helper script to set FEL mode device permissions
# Usage: sudo ./tools/fel-permissions.sh

set -e

echo "Searching for Allwinner FEL mode device..."

# Find the device
DEVICE=$(lsusb | grep "1f3a:efe8" | awk '{print $2"/"$4}' | sed 's/://')

if [ -z "$DEVICE" ]; then
    echo "ERROR: No Allwinner FEL mode device found!"
    echo "Please put the HY300 into FEL mode by:"
    echo "1. Power off the device"
    echo "2. Press and hold the remote button"
    echo "3. Power on while holding the button"
    echo "4. Connect USB cable"
    exit 1
fi

# Extract bus and device numbers
BUS=$(echo "$DEVICE" | cut -d'/' -f1)
DEV=$(echo "$DEVICE" | cut -d'/' -f2)

DEVICE_PATH="/dev/bus/usb/$BUS/$DEV"

echo "Found device at: $DEVICE_PATH"
echo "Setting permissions..."

chmod 666 "$DEVICE_PATH"

echo "Permissions set successfully!"
echo "You can now run: sunxi-fel version"
