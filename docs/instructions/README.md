### **Project Title: Mainline Linux Port for the Allwinner H713-based HY300 Projector**

---

### **1\. Primary Objective**

To perform a complete hardware bring-up and port of a modern, mainline-based Linux distribution to the HY300 consumer projector. The final deliverable should be a set of source code patches and a reproducible build process for a functional Linux image that supports the device's core hardware components.

---

### **2\. Target Hardware Profile: HY300 Projector**

The agent must operate on the following hardware specifications:

* **System-on-Chip (SoC):** Allwinner H713 (Quad-core ARM Cortex-A53, Mali-G31 GPU).1  
* **Memory (RAM):** 1GB or 2GB DDR3 (exact parameters are unknown and must be determined).4  
* **Internal Storage:** 8GB, 16GB, or 32GB eMMC.6  
* **Wireless Module:** AW869A, identified as being based on the Aicsemi (AIC) 8800 chipset, providing WiFi 6 and Bluetooth 5.0.7  
* **Optical Engine (Proprietary):**  
  * **LCD Panel:** Single 1280x720 native resolution panel. Known models include FPC-C269XHD0204-V, ZTW269HD720P-V01, and FPC-FJ035FHD01-V3\_A.9 The interface is likely MIPI-DSI.  
  * **Light Source:** White LED module controlled via GPIO or PWM.  
  * **Keystone:** Electronic keystone correction system.  
* **I/O Ports:** HDMI input, USB 2.0 Type-A, 3.5mm audio jack, DC power input.11  
* **User Input:** Infrared (IR) remote control.12

---

### **3\. Prerequisites & Required Tools**

The agent must be provisioned with the following software and hardware capabilities:

* **Host Environment:** A Linux-based development system (e.g., Debian/Ubuntu).  
* **Toolchain:** An aarch64 cross-compiler toolchain.  
* **Core Software:**  
  * sunxi-tools: For interaction with the SoC's FEL recovery mode.14  
  * Git: For cloning U-Boot, Linux kernel, and other necessary repositories.  
  * Standard build utilities: make, gcc, ncurses-dev, etc.16  
* **Hardware Tools:**  
  * USB-A to USB-A cable.  
  * USB-to-TTL Serial Adapter for debugging via the UART console.  
  * Soldering equipment to attach wires to the PCB's UART pads.  
  * (Optional but Recommended) A logic analyzer for snooping protocols on I2C, SPI, or MIPI-DSI buses.

---

### **4\. Phased Execution Plan**

The project must be executed in the following strict sequence. Failure or incomplete data from an earlier phase prevents subsequent phases from commencing.

#### **Phase I: Low-Level Device Access & Reconnaissance**

1. **Establish Serial Console:** Physically disassemble the HY300 projector, identify the UART TX, RX, and GND pads on the mainboard, and establish a stable serial console connection.17  
2. **Achieve FEL Access:** Determine the specific button combination or procedure to force the H713 SoC into FEL (Firmware Execute and Load) mode. Verify the connection using sunxi-fel version.20  
3. **Mandatory Firmware Backup:** Before making any modifications, perform a complete, bit-for-bit backup of the internal eMMC using sunxi-fel. This backup is the sole recovery mechanism and a critical source of information.  
4. **Firmware Analysis:**  
   * Unpack the stock firmware image using tools like sunxi-fw or imgrepacker.  
   * Isolate the boot0.bin binary. This file contains the proprietary DRAM initialization code.  
   * Decompile the stock Android kernel's device tree blob (.dtb) to identify hardware addresses, pin configurations, and peripheral nodes.

#### **Phase II: U-Boot Porting & Bring-up**

1. **Source & Configure:** Clone the latest mainline U-Boot source code.23 Create a new board configuration (  
   hy300\_h713\_defconfig) and device tree (sun50i-h713-hy300.dts) based on the most similar supported sun50i platform.24  
2. **DRAM Parameter Extraction (Critical Task):** This is the primary known obstacle. Manually reverse-engineer the boot0.bin binary to extract the precise DRAM timing, clock, and impedance (ZQ) values. These values must be correctly translated into the CONFIG\_DRAM\_\* variables in the new defconfig file.26  
3. **Iterative Debugging:** Cross-compile u-boot-sunxi-with-spl.bin. Load and execute this binary in RAM using sunxi-fel spl....27 Monitor the serial console output. The objective is to resolve the known DRAM initialization hang and achieve a stable U-Boot command prompt. This will require an iterative process of adjusting DRAM parameters, recompiling, and re-testing.

#### **Phase III: Mainline Kernel Boot**

1. **Kernel Preparation:** Clone the mainline Linux kernel source. Expand the device tree from Phase II to include nodes for all necessary peripherals (eMMC, USB, I2C, etc.).16  
2. **Minimal System:** Configure the kernel for the sun50i platform and create a minimal root filesystem using Buildroot or a similar tool.  
3. **SD Card Boot:** Prepare a bootable microSD card. Write the compiled u-boot-sunxi-with-spl.bin to the correct offset (8KB).28 Create a boot partition with the kernel image (  
   uImage/Image), the compiled device tree blob (.dtb), and a boot.scr script to automate the boot process.30  
4. **Boot to Shell:** Boot the device from the SD card and debug via the serial console until a command-line shell is accessible.

#### **Phase IV: Post-Boot Driver Enablement**

1. **Wireless Connectivity:**  
   * Source the aic8800 driver, likely requiring adaptation of existing community patches.8  
   * Extract the necessary firmware blob from the stock Android system.img partition.31  
   * Configure the device tree to describe the SDIO bus for the module and load the driver.  
2. **Optical Engine (Reverse Engineering Task):**  
   * Analyze the stock device tree and kernel modules for clues on how the LCD panel and LED are controlled.  
   * If necessary, use a logic analyzer to capture the MIPI-DSI and I2C/SPI communication between the H713 and the optical engine components during the stock Android boot process.  
   * Based on the captured protocol, develop a custom DRM/KMS panel driver from scratch for the LCD and a simple GPIO/PWM driver for the LED light source.

---

### **5\. Deliverables**

1. **Source Code:** A complete set of patches against the mainline U-Boot and Linux kernel repositories that add support for the HY300 projector.  
2. **Configuration Files:** The final, working defconfig files for U-Boot and the kernel, and the complete device tree source (.dts) file.  
3. **Build Script:** An automated script to reproduce the bootable Linux image from source.  
4. **Documentation:** A detailed report on the reverse-engineering process for the DRAM parameters and the proprietary optical engine components.

#### **Works cited**

1. Projectors Android11 4K Wifi6 300ANSI HD Allwinner H713 BT5.0 1080P 1280\*720P | eBay, accessed on September 18, 2025, [https://www.ebay.com/itm/405918711478](https://www.ebay.com/itm/405918711478)  
2. New Projector Android 11.0 160ANSI WiFi6 \+ BT5.0 US Plug Allwinner H713 Quad-core ARM Cortex-A53 Dual Wifi HD 1280\*720p 4K \- Coolbe, accessed on September 18, 2025, [https://www.coolbe.com/en/products/669e18c44b8ae101ee8fd708](https://www.coolbe.com/en/products/669e18c44b8ae101ee8fd708)  
3. 580ANSI Projector Android 11 4K 1080P Wifi6 Voice Control Allwinner H713 Electro | eBay, accessed on September 18, 2025, [https://www.ebay.com/itm/226716697130](https://www.ebay.com/itm/226716697130)  
4. Magcubic HY300 Android 11 Projector Allwinner H713, 1GB/8GB ..., accessed on September 18, 2025, [https://forum.armbian.com/topic/42114-magcubic-hy300-android-11-projector-allwinner-h713-1gb8gb/](https://forum.armbian.com/topic/42114-magcubic-hy300-android-11-projector-allwinner-h713-1gb8gb/)  
5. H713 Chip P30max Gaming Projection 36000 Games Android 4K Smart Gaming Projector, accessed on September 18, 2025, [https://www.alebdaa.ae/en/products/%D8%AC%D9%87%D8%A7%D8%B2-%D8%B9%D8%B1%D8%B6-%D8%A7%D9%84%D8%A3%D9%84%D8%B9%D8%A7%D8%A8-%D8%A7%D9%84%D8%B0%D9%83%D9%8A-h713-chip-p30max-gaming-projection-36000-games-android-4k](https://www.alebdaa.ae/en/products/%D8%AC%D9%87%D8%A7%D8%B2-%D8%B9%D8%B1%D8%B6-%D8%A7%D9%84%D8%A3%D9%84%D8%B9%D8%A7%D8%A8-%D8%A7%D9%84%D8%B0%D9%83%D9%8A-h713-chip-p30max-gaming-projection-36000-games-android-4k)  
6. 4K Android 11 Dual Wifi6 230 ANSI Allwinner H713 BT5.0 1080P 1980\*1080P Home Cinema Outdoor Projetor \- AliExpress, accessed on September 18, 2025, [https://www.aliexpress.com/i/1005009111220214.html](https://www.aliexpress.com/i/1005009111220214.html)  
7. HY350 ANDROID 11.0V (CPU ALLWINNER H713) SMART PROJECTOR 2GB+32GB DUAL SPEAKER 1080P WITH DUAL BAND WIFI6 AND BLUETOOTH 5.0 BRIGHTNESS (LUMENS) 580 ANSI SUPPORT 4K | Daraz.pk, accessed on September 18, 2025, [https://www.daraz.pk/products/hy350-android-110v-cpu-allwinner-h713-smart-projector-2gb32gb-dual-speaker-1080p-with-dual-band-wifi6-and-bluetooth-50-brightness-lumens-580-ansi-support-4k-i494588070.html](https://www.daraz.pk/products/hy350-android-110v-cpu-allwinner-h713-smart-projector-2gb32gb-dual-speaker-1080p-with-dual-band-wifi6-and-bluetooth-50-brightness-lumens-580-ansi-support-4k-i494588070.html)  
8. How to install armbian in h618? \- Page 18 \- Allwinner CPU Boxes, accessed on September 18, 2025, [https://forum.armbian.com/topic/29794-how-to-install-armbian-in-h618/page/18/](https://forum.armbian.com/topic/29794-how-to-install-armbian-in-h618/page/18/)  
9. 2.69Inch FPC-C269XHD0204-V LCD Screen hy300 Projector Display hy350 | eBay, accessed on September 18, 2025, [https://www.ebay.com/itm/226702924162](https://www.ebay.com/itm/226702924162)  
10. New HY300 HY320 HY350 Projector FPC-FJ035FHD01-V3\_A FPC-FJ035FHD05-V6-V7 FPC-CXHD006-V1 FPC-QY27003-A HP-445001-V2display screen \- AliExpress, accessed on September 18, 2025, [https://www.aliexpress.com/item/1005005026693706.html](https://www.aliexpress.com/item/1005005026693706.html)  
11. Magcubic HY300 $47 Portable LED Android Projector: Should you buy it? \- AndroidPIMP, accessed on September 18, 2025, [https://www.androidpimp.com/projectors/hy300-android-projector/](https://www.androidpimp.com/projectors/hy300-android-projector/)  
12. MAGCUBIC HY300 Portable Projector User Manual, accessed on September 18, 2025, [https://manuals.plus/magcubic/hy300-portable-projector-manual](https://manuals.plus/magcubic/hy300-portable-projector-manual)  
13. Smart Projector HY300 Quick Installation Guide \- Manuals.plus, accessed on September 18, 2025, [https://manuals.plus/m/ac1b1852e2b058266295106e490e977e152146b5766152be71e770026fa9787b](https://manuals.plus/m/ac1b1852e2b058266295106e490e977e152146b5766152be71e770026fa9787b)  
14. linux-sunxi/sunxi-tools: A collection of command line tools for ARM devices with Allwinner SoCs. \- GitHub, accessed on September 18, 2025, [https://github.com/linux-sunxi/sunxi-tools](https://github.com/linux-sunxi/sunxi-tools)  
15. Sunxi-tools \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/Sunxi-tools](https://linux-sunxi.org/Sunxi-tools)  
16. Linux Kernel \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/Linux\_Kernel](https://linux-sunxi.org/Linux_Kernel)  
17. Bring Your HY300 Pro Android Projector Back to Life: Ultimate DIY Cleaning Guide\!, accessed on September 18, 2025, [https://www.youtube.com/watch?v=bFQbnpkX3eY](https://www.youtube.com/watch?v=bFQbnpkX3eY)  
18. Side of HY300 projector coming apart \- Reddit, accessed on September 18, 2025, [https://www.reddit.com/r/projectors/comments/1hnjq5a/side\_of\_hy300\_projector\_coming\_apart/](https://www.reddit.com/r/projectors/comments/1hnjq5a/side_of_hy300_projector_coming_apart/)  
19. Disassemble and Assemble Your HY300 Projector WITHOUT ERROR \- YouTube, accessed on September 18, 2025, [https://www.youtube.com/watch?v=HmJ3HIn9oqw](https://www.youtube.com/watch?v=HmJ3HIn9oqw)  
20. FEL mode \- Neutis docs, accessed on September 18, 2025, [https://docs.neutis.io/hardware-integration/fel/](https://docs.neutis.io/hardware-integration/fel/)  
21. Allwinner Boot / FEL / FES / NAND Dump \- \- XOR, accessed on September 18, 2025, [https://xor.co.za/post/2018-12-01-fel-bootprocess/](https://xor.co.za/post/2018-12-01-fel-bootprocess/)  
22. FEL \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/FEL](https://linux-sunxi.org/FEL)  
23. OpenIPC/u-boot-allwinner: U-Boot for Allwinner SoC's \- GitHub, accessed on September 18, 2025, [https://github.com/OpenIPC/u-boot-allwinner](https://github.com/OpenIPC/u-boot-allwinner)  
24. U-Boot \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/U-Boot](https://linux-sunxi.org/U-Boot)  
25. Porting U-Boot and Linux on new ARM boards: a step-by-step guide, accessed on September 18, 2025, [https://bootlin.com/pub/conferences/2017/elce/schulz-how-to-support-new-board-u-boot-linux/schulz-how-to-support-new-board-u-boot-linux.pdf](https://bootlin.com/pub/conferences/2017/elce/schulz-how-to-support-new-board-u-boot-linux/schulz-how-to-support-new-board-u-boot-linux.pdf)  
26. \#linux-sunxi on 2024-03-29 — irc logs at oftc.catirclogs.org, accessed on September 18, 2025, [https://oftc.irclog.whitequark.org/linux-sunxi/2024-03-29](https://oftc.irclog.whitequark.org/linux-sunxi/2024-03-29)  
27. sunxi-fel(1) — sunxi-tools — Debian unstable, accessed on September 18, 2025, [https://manpages.debian.org/unstable/sunxi-tools/sunxi-fel.1](https://manpages.debian.org/unstable/sunxi-tools/sunxi-fel.1)  
28. Allwinner SoC based boards \- The U-Boot Documentation, accessed on September 18, 2025, [https://docs.u-boot.org/en/stable/board/allwinner/sunxi.html](https://docs.u-boot.org/en/stable/board/allwinner/sunxi.html)  
29. Bootable SD card \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/Bootable\_SD\_card](https://linux-sunxi.org/Bootable_SD_card)  
30. \[OpenWrt Wiki\] Allwinner Sun4i/5i/6i/7i/9i (sunxi), accessed on September 18, 2025, [https://openwrt.org/docs/techref/hardware/soc/soc.allwinner.sunxi](https://openwrt.org/docs/techref/hardware/soc/soc.allwinner.sunxi)  
31. How to install unsupported Bluetooth 5.0 Dongle on Linux | by Filipe Pires | Nerd For Tech, accessed on September 18, 2025, [https://medium.com/nerd-for-tech/how-to-install-unsupported-bluetooth-5-0-dongle-on-linux-4bf34aa99fed](https://medium.com/nerd-for-tech/how-to-install-unsupported-bluetooth-5-0-dongle-on-linux-4bf34aa99fed)