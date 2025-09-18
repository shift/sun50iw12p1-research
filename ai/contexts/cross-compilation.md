# Cross-Compilation Context

## Environment Setup
The Nix flake provides a complete cross-compilation environment for ARM64.

## Environment Variables
```bash
export CROSS_COMPILE=aarch64-unknown-linux-gnu-
export ARCH=arm64
export CC=aarch64-unknown-linux-gnu-gcc
export CXX=aarch64-unknown-linux-gnu-g++
```

## Toolchain Components
- **GCC:** aarch64-unknown-linux-gnu-gcc
- **Binutils:** aarch64-unknown-linux-gnu-{as,ld,objdump,etc}
- **GDB:** aarch64-unknown-linux-gnu-gdb

## U-Boot Cross-Compilation
```bash
# Clean previous builds
make clean

# Configure for sunxi platform
make CROSS_COMPILE=aarch64-unknown-linux-gnu- sunxi_defconfig

# Customize configuration
make CROSS_COMPILE=aarch64-unknown-linux-gnu- menuconfig

# Build
make CROSS_COMPILE=aarch64-unknown-linux-gnu- -j$(nproc)

# Output: u-boot-sunxi-with-spl.bin
```

## Kernel Cross-Compilation
```bash
# Clean
make ARCH=arm64 clean

# Default sunxi configuration
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- defconfig

# Sunxi-specific config
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- sunxi_defconfig

# Customize
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- menuconfig

# Build kernel
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- -j$(nproc)

# Build device tree
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- dtbs

# Build modules
make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- modules
```

## Device Tree Compilation
```bash
# Compile DTS to DTB
dtc -I dts -O dtb -o sun50i-h713-hy300.dtb sun50i-h713-hy300.dts

# Decompile DTB to DTS
dtc -I dtb -O dts -o output.dts input.dtb
```

## Common Issues
- **Missing headers:** Install kernel headers for target
- **Wrong endianness:** Ensure little-endian for ARM64
- **Library paths:** Use target libraries, not host
- **Binary format:** Verify ELF format with `file` command

## Verification
```bash
# Check binary architecture
file u-boot-sunxi-with-spl.bin
readelf -h vmlinux

# Verify cross-compilation
aarch64-unknown-linux-gnu-objdump -f binary.elf
```