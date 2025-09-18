### **Executive Summary**

An in-depth analysis of the four provided Device Tree Blob (DTB) files from the HY300 projector's factory firmware provides a definitive and highly detailed hardware blueprint of the device. The consistency across the files, which include both modern kernel-level device trees and a legacy bootloader configuration, confirms a standardized hardware platform and removes significant guesswork from any potential software porting effort.1

The analysis confirms the central processor is the **Allwinner H713 System-on-Chip (SoC)**, an ARM Cortex-A53 based platform identified by the compatible string arm,sun50iw12p1.1 The files provide precise pin mappings and configurations for nearly all essential peripherals, including the debug serial port, the WiFi/Bluetooth module, the infrared receiver, and the motor controller for keystone/focus adjustment. This information represents a massive shortcut, effectively providing a pre-made hardware map that would otherwise require weeks of manual reverse-engineering.

However, the most critical finding is the revelation of a complex and proprietary display architecture. The DTBs show that the Allwinner H713 offloads all display management to a dedicated **MIPS co-processor**, which runs its own proprietary firmware (display.bin).1 This means that enabling video output is not a matter of writing a standard Linux display driver, but rather a formidable challenge of reverse-engineering the communication protocol between the main ARM cores and this secondary MIPS processor.

While the DTBs provide an invaluable roadmap for enabling most of the hardware, they also confirm that the display subsystem is the single greatest and most complex obstacle to a successful mainline Linux port. The overall mainline support for the H713 SoC is still in its infancy, with fundamental challenges like DRAM initialization currently blocking community development efforts.2

---

### **1\. SoC and Core System Identification**

The DTB files provide an unambiguous identification of the core components, confirming the device's foundation. The root node's compatible string, "allwinner,tv303", "arm,sun50iw12p1", definitively identifies the SoC as the **Allwinner H713** (sun50iw12p1) on a board or product family designated tv303.1

| Property | Value | Significance | Citation |
| :---- | :---- | :---- | :---- |
| compatible | "allwinner,tv303", "arm,sun50iw12p1" | Identifies the SoC as Allwinner H713 and the board family as tv303. | 1 |
| model | "sun50iw12" | Internal Allwinner model name for the SoC family. | 1 |
| cpus | 4x "arm,cortex-a53", "arm,armv8" | Confirms a quad-core 64-bit ARMv8-A CPU architecture. | 1 |
| gpu | "arm,mali-midgard" | Identifies the GPU as a **Mali-G31**. | 1 |
| ve@1c0e000 | "allwinner,sunxi-cedar-ve" | Confirms the CedarX engine for standard video decoding. | 1 |
| av1@1c0d000 | "allwinner,sunxi-google-ve" | Indicates a dedicated hardware block for decoding modern codecs like AV1. | 1 |

---

### **2\. Cross-File Comparison and Corroboration**

The provided firmware contains two distinct types of configuration files, which together offer a complete and verified view of the hardware setup.

* **Modern Kernel DTBs:** Three of the files (1eac00.dtb, c755.dtb, fc00.dtb) are standard Device Tree Blobs used by the modern Linux kernel.1 Their identical content confirms a stable hardware revision for different boot scenarios (e.g., normal boot, recovery).  
* **Legacy Bootloader Configuration:** The fourth file (16815c.dtb) is in a legacy sys\_config.fex format, which is used by the early-stage U-Boot bootloader to configure hardware before the kernel loads.1

The configurations in the legacy file perfectly match those in the modern DTBs. This cross-verification provides extremely high confidence in the accuracy of the hardware map.

| Peripheral | Legacy Config (16815c.dtb) | Modern DTB (system.dtb) | Status |
| :---- | :---- | :---- | :---- |
| **IR Receiver** | s\_cir0@2: allwinner,pins \= "PL9" 1 | s\_cir@7040000: pinctrl-0 \= \<\&s\_cir0\_pins\_a\> which maps to PL9 1 | **Confirmed** |
| **Panel Power** | tvtop@1: panel\_power\_en \= \<...\> 1 | tvtop@5700000: panel\_power\_en \= \<...\> 1 | **Confirmed** |
| **Panel PWM** | tvtop@1: panel\_pwm\_ch \= \<0x03\> 1 | tvtop@5700000: panel\_pwm\_ch \= \<0x03\> 1 | **Confirmed** |

---

### **3\. The Display Subsystem: A MIPS Co-Processor Architecture**

The most significant discovery from the DTBs is the unconventional architecture of the display subsystem. This design choice dramatically increases the complexity of enabling video output on a custom Linux system.

* **MIPS Co-Processor:** A node named mipsloader@3061000 is defined with a compatible string of "allwinner,sunxi-mipsloader".1 This node describes a secondary processor based on the MIPS architecture, which is entirely separate from the main ARM cores.  
* **Dedicated Firmware:** The firmware section of the DTB explicitly instructs the system to load a binary file named **display.bin** into a reserved memory region associated with the MIPS loader.1  
* **Implications for Development:** This architecture means the main ARM CPU does not directly control the LCD panel. To get any video output, a developer must reverse-engineer the proprietary communication protocol used by the ARM cores to send commands and framebuffer data to the MIPS processor. This is a far more complex task than writing a standard Linux Direct Rendering Manager (DRM) driver for other Allwinner SoCs that use the DE2/DE33 display engine.

#### **Display Property Analysis**

| Node | Property | Value | Significance | Citation |
| :---- | :---- | :---- | :---- | :---- |
| **mipsloader** | compatible | "allwinner,sunxi-mipsloader" | Identifies the node as a loader for a MIPS co-processor. | 1 |
|  | memory-region | \<\&mipsloader\> | Points to a reserved memory region for the MIPS firmware. | 1 |
| **firmware/mips** | file\_name | "display.bin" | Specifies the proprietary firmware file to be loaded onto the co-processor. | 1 |
| **tvtop** | panel\_power\_en | \<0x02 0x07 0x13 0x20\> | Defines the GPIO pin used to enable power to the LCD panel. | 1 |
|  | panel\_bl\_en | \<0x02 0x01 0x05 0x00\> | Defines the GPIO pin used to enable the panel's backlight. | 1 |
|  | panel\_pwm\_ch | \<0x03\> | Specifies PWM channel 3 for backlight brightness control. | 1 |
|  | panel\_dclk\_freq | \<0x8d9ee20\> (148.5 MHz) | Sets the panel's dot clock frequency, a critical timing parameter. | 1 |
|  | panel\_poweron\_delay | \<0x14\>, \<0x226\>, \<0x4b\> | Defines the precise power-on timing sequence in milliseconds. | 1 |

---

### **4\. Peripheral Hardware Mapping and Analysis**

The DTBs provide a comprehensive and precise map of various hardware peripherals and their connections to the SoC's GPIO pins. This information is a massive accelerator for driver development.

#### **Motor Controller (Keystone/Focus)**

A dedicated motor\_ctr node provides all necessary information to control the electronic keystone/focus motors.1

| Property | Value | Significance | Citation |
| :---- | :---- | :---- | :---- |
| compatible | "motor-control" | Generic compatible string for a motor controller. | 1 |
| motor-gpio-phaseX | \<0x02 0x07 0x04 0x00\> etc. | Defines the four GPIOs (PH4, PH5, PH6, PH7) for the motor phases. | 1 |
| motor-cw-table | \<0x9080c04 0x6020301\> | Provides the stepping sequence for clockwise rotation. | 1 |
| motor-ccw-table | \<0x1030206 0x40c0809\> | Provides the stepping sequence for counter-clockwise rotation. | 1 |

#### **Other Key Peripherals**

| Peripheral | Node | Key Property | Value / Pin | Significance | Citation |
| :---- | :---- | :---- | :---- | :---- | :---- |
| **Debug UART** | uart@2500000 | pinctrl-0 | PH0, PH1 | Provides exact pins for serial console debugging. | 1 |
| **WiFi/Bluetooth** | sdmmc@4021000 | cap-sdio-irq | (present) | Confirms SDIO interface for the wireless module. | 1 |
|  | rfkill | wlan\_regon | PL1 | Defines the GPIO to power on the WiFi chip. | 1 |
|  | rfkill | bt\_rst\_n | PL4 | Defines the GPIO to reset the Bluetooth chip. | 1 |
| **IR Receiver** | s\_cir@7040000 | pinctrl-0 | PL9 | Defines the GPIO for the infrared remote receiver. | 1 |
|  | cir\_param | ir\_power\_key\_code0 | \<0x14\> | Contains a full map of remote control key codes. | 1 |
| **Accelerometers** | twi@2502400 | compatible | "stk,stk83xx", "kxtj3" | Identifies two accelerometers on the I2C bus, likely for auto-keystone. | 1 |

---

### **5\. Mainline Linux Compatibility Assessment**

The Allwinner H713 SoC is not currently supported by the mainline Linux kernel or major community projects like Armbian.2 Development within the

linux-sunxi community is in a very early, experimental stage.2

* **SoC Support:** The H713 is considered architecturally closer to the newer A523 SoC family than older, well-supported chips.2 However, the entire A523 family is also under early development.5  
* **DRAM Initialization:** The single greatest blocker to any progress is the failure to initialize the DRAM controller in a mainline U-Boot environment. The boot process hangs definitively at the point of memory setup, a problem that expert developers are actively trying to solve through advanced reverse-engineering of the proprietary boot0 binary.2  
* **Display Engine:** The MIPS co-processor architecture is undocumented and has no existing mainline driver. Enabling it would require a significant reverse-engineering effort to understand the communication protocol between the ARM and MIPS cores.1  
* **VPU (CedarX):** While the mainline kernel has a driver (cedrus) for the Allwinner VPU, it would require significant adaptation and new code to support the specific version in the H713.2  
* **WiFi/Bluetooth (AIC8800):** The AW869A module is based on the AIC8800 chipset.8 This chipset does not have a mainline driver, but out-of-tree drivers exist and have been ported to other Allwinner devices, making this a feasible but non-trivial porting task.5

---

### **6\. Hardware Enablement Checklist**

This checklist summarizes the status and required effort for enabling key hardware components in a mainline Linux environment.

| Component | Mainline Status | Driver/Subsystem | Required Development Path |
| :---- | :---- | :---- | :---- |
| **SoC Boot (DRAM)** | **Unsupported** | U-Boot SPL | **Blocker:** Reverse-engineer boot0 to extract and implement correct DRAM parameters. This is a major, unsolved hurdle.3 |
| **CPU** | Supported | arm64 | Standard ARMv8-A support is present in the mainline kernel.1 |
| **eMMC Controller** | Well Supported | mmc\_sunxi | Enable node in Device Tree (.dts) with correct pinmux and clock settings from DTB.1 |
| **USB 2.0 Controller** | Well Supported | musb\_sunxi | Enable node in Device Tree (.dts) with correct PHY and power configuration from DTB.1 |
| **UART (Serial)** | Well Supported | 8250\_sunxi | Enable node in Device Tree (.dts) with pinmux settings from DTB (PH0, PH1).1 |
| **WiFi/BT (AIC8800)** | **Unsupported** | aic8800 (out-of-tree) | Adapt existing aic8800 driver patches from community builds. Extract firmware from stock Android. Add SDIO node to Device Tree.5 |
| **IR Receiver** | Supported | sunxi-ir / lirc | Enable node in Device Tree (.dts). Create a keymap file using the codes provided in the cir\_param node.1 |
| **Display Engine** | **Unsupported** | Custom Driver | **Major Hurdle:** Reverse-engineer the communication protocol between the ARM cores and the MIPS co-processor. Write a custom driver to send commands/framebuffers.1 |
| **Motor Controller** | **Unsupported** | gpio-motor / Custom | Write a new driver using the GPIO pins and stepping sequences provided in the motor\_ctr node of the DTB.1 |
| **GPU (Mali-G31)** | Supported (Panfrost) | panfrost (Mesa) | Enable node in Device Tree (.dts). Requires a working display driver first.1 |
| **VPU (CedarX)** | Partial Support | cedrus (V4L2) | Requires significant reverse-engineering and adaptation of the existing cedrus driver for the H713's VPU variant.2 |

#### **Works cited**

1. 16815c.dtb  
2. Magcubic HY300 Android 11 Projector Allwinner H713, 1GB/8GB ..., accessed on September 18, 2025, [https://forum.armbian.com/topic/42114-magcubic-hy300-android-11-projector-allwinner-h713-1gb8gb/](https://forum.armbian.com/topic/42114-magcubic-hy300-android-11-projector-allwinner-h713-1gb8gb/)  
3. \#linux-sunxi on 2024-03-29 — irc logs at oftc.catirclogs.org, accessed on September 18, 2025, [https://oftc.irclog.whitequark.org/linux-sunxi/2024-03-29](https://oftc.irclog.whitequark.org/linux-sunxi/2024-03-29)  
4. ARM Allwinner SoCs \- The Linux Kernel documentation, accessed on September 18, 2025, [https://docs.kernel.org/arch/arm/sunxi.html](https://docs.kernel.org/arch/arm/sunxi.html)  
5. Allwinner H728/A523/A527/T527 initial support thread \- ARM \- Manjaro Linux Forum, accessed on September 18, 2025, [https://forum.manjaro.org/t/allwinner-h728-a523-a527-t527-initial-support-thread/173654](https://forum.manjaro.org/t/allwinner-h728-a523-a527-t527-initial-support-thread/173654)  
6. Allwinner VPU support in mainline Linux status update (week 13\) \- Bootlin, accessed on September 18, 2025, [https://bootlin.com/blog/allwinner-vpu-support-in-mainline-linux-status-update-week-13/](https://bootlin.com/blog/allwinner-vpu-support-in-mainline-linux-status-update-week-13/)  
7. Allwinner VPU support in mainline Linux status update (week 26\) \- Bootlin, accessed on September 18, 2025, [https://bootlin.com/blog/allwinner-vpu-support-in-mainline-linux-status-update-week-26/](https://bootlin.com/blog/allwinner-vpu-support-in-mainline-linux-status-update-week-26/)  
8. HY350 ANDROID 11.0V (CPU ALLWINNER H713) SMART PROJECTOR 2GB+32GB DUAL SPEAKER 1080P WITH DUAL BAND WIFI6 AND BLUETOOTH 5.0 BRIGHTNESS (LUMENS) 580 ANSI SUPPORT 4K | Daraz.pk, accessed on September 18, 2025, [https://www.daraz.pk/products/hy350-android-110v-cpu-allwinner-h713-smart-projector-2gb32gb-dual-speaker-1080p-with-dual-band-wifi6-and-bluetooth-50-brightness-lumens-580-ansi-support-4k-i494588070.html](https://www.daraz.pk/products/hy350-android-110v-cpu-allwinner-h713-smart-projector-2gb32gb-dual-speaker-1080p-with-dual-band-wifi6-and-bluetooth-50-brightness-lumens-580-ansi-support-4k-i494588070.html)  
9. 4K Android 11 Dual Wifi6 230 ANSI Allwinner H713 BT5.0 1080P 1980\*1080P Home Cinema Outdoor Projetor \- AliExpress, accessed on September 18, 2025, [https://www.aliexpress.com/i/1005009111220214.html](https://www.aliexpress.com/i/1005009111220214.html)  
10. How to install armbian in h618? \- Page 18 \- Allwinner CPU Boxes, accessed on September 18, 2025, [https://forum.armbian.com/topic/29794-how-to-install-armbian-in-h618/page/18/](https://forum.armbian.com/topic/29794-how-to-install-armbian-in-h618/page/18/)  
11. net-wireless/aic8800 \- Gentoo Packages, accessed on September 18, 2025, [https://packages.gentoo.org/packages/net-wireless/aic8800](https://packages.gentoo.org/packages/net-wireless/aic8800)  
12. Aicsemi aic8800 Wi-Fi driver. Official support for Radxa devices only. \- GitHub, accessed on September 18, 2025, [https://github.com/radxa-pkg/aic8800](https://github.com/radxa-pkg/aic8800)  
13. USB Adapter Wifi 6 AIC8800D80 Drivers for ubuntu : r/linux4noobs \- Reddit, accessed on September 18, 2025, [https://www.reddit.com/r/linux4noobs/comments/1ehpblo/usb\_adapter\_wifi\_6\_aic8800d80\_drivers\_for\_ubuntu/](https://www.reddit.com/r/linux4noobs/comments/1ehpblo/usb_adapter_wifi_6_aic8800d80_drivers_for_ubuntu/)  
14. goecho/aic8800\_linux\_drvier: Aicsemi aic8800 Wi-Fi driver \- GitHub, accessed on September 18, 2025, [https://github.com/goecho/aic8800\_linux\_drvier](https://github.com/goecho/aic8800_linux_drvier)  
15. AIC8800 wifi sdio module not working with kernel 6.12+ \- Rockchip \- Armbian forum, accessed on September 18, 2025, [https://forum.armbian.com/topic/50332-aic8800-wifi-sdio-module-not-working-with-kernel-612/](https://forum.armbian.com/topic/50332-aic8800-wifi-sdio-module-not-working-with-kernel-612/)  
16. Need help getting AIC8800 based USB WiFi adapter working again : r/raspberry\_pi \- Reddit, accessed on September 18, 2025, [https://www.reddit.com/r/raspberry\_pi/comments/1jtqecl/need\_help\_getting\_aic8800\_based\_usb\_wifi\_adapter/](https://www.reddit.com/r/raspberry_pi/comments/1jtqecl/need_help_getting_aic8800_based_usb_wifi_adapter/)  
17. U-Boot \- linux-sunxi.org, accessed on September 18, 2025, [https://linux-sunxi.org/U-Boot](https://linux-sunxi.org/U-Boot)