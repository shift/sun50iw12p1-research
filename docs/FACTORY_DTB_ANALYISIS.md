### **Executive Summary**

An in-depth analysis of the four provided Device Tree Blob (DTB) files from the HY300 projector's factory firmware provides a definitive and highly detailed hardware blueprint of the device. The consistency across the files confirms a standardized hardware platform, removing significant guesswork from any potential software porting effort.

The analysis confirms the central processor is the **Allwinner H713 System-on-Chip (SoC)**, an ARM Cortex-A53 based platform. The files provide precise pin mappings and configurations for nearly all essential peripherals, including the debug serial port, the WiFi/Bluetooth module, the infrared receiver, and even the motor controller for keystone/focus adjustment. This information represents a massive shortcut, effectively providing a pre-made hardware map that would otherwise require weeks of manual reverse-engineering.

However, the most critical finding is the revelation of a complex and proprietary display architecture. The DTBs show that the Allwinner H713 offloads all display management to a dedicated **MIPS co-processor**, which runs its own proprietary firmware (display.bin). This means that enabling video output is not a matter of writing a standard Linux display driver, but rather a formidable challenge of reverse-engineering the communication protocol between the main ARM cores and this secondary MIPS processor.

While the DTBs provide an invaluable roadmap for enabling most of the hardware, they also confirm that the display subsystem is the single greatest and most complex obstacle to a successful mainline Linux port.

---

### **1\. Core System Architecture and Component Identification**

The DTB files provide an unambiguous identification of the core components, confirming the device's foundation.1

* **System-on-Chip (SoC):** The root node's compatible string, "allwinner,tv303", "arm,sun50iw12p1", is the specific internal designator for the **Allwinner H713** SoC.1  
* **CPU Architecture:** The cpus node explicitly defines four CPU cores, each compatible with "arm,cortex-a53" and "arm,armv8". This confirms a quad-core 64-bit ARMv8-A architecture.1  
* **GPU Architecture:** The GPU is identified with the compatible string "arm,mali-midgard", which corresponds to the **Mali-G31** GPU family.1  
* **Video Processing Unit (VPU):** The presence of two distinct video engine nodes confirms a sophisticated VPU setup:  
  * ve@1c0e000: Compatible with "allwinner,sunxi-cedar-ve", this is the traditional CedarX engine for decoding standard codecs like H.264.1  
  * av1@1c0d000: Compatible with "allwinner,sunxi-google-ve", this indicates a dedicated hardware block for decoding modern, royalty-free codecs like AV1.1

---

### **2\. The Display Subsystem: A MIPS Co-Processor Architecture**

The most significant discovery from the DTBs is the unconventional architecture of the display subsystem. This design choice dramatically increases the complexity of enabling video output on a custom Linux system.

* **MIPS Co-Processor:** A node named mipsloader@3061000 is defined with a compatible string of "allwinner,sunxi-mipsloader".1 This node describes a secondary processor based on the MIPS architecture, which is entirely separate from the main ARM cores.  
* **Dedicated Firmware:** The firmware section of the DTB explicitly instructs the system to load a binary file named **display.bin** into a reserved memory region associated with the MIPS loader.1  
* **Implications for Development:** This architecture means the main ARM CPU does not directly control the LCD panel. Instead, all display functions are offloaded to this MIPS co-processor. To get any video output, a developer must reverse-engineer the proprietary communication protocol used by the ARM cores to send commands and framebuffer data to the MIPS processor. This is a far more complex task than writing a standard Linux Direct Rendering Manager (DRM) driver.  
* **Panel Electrical and Timing Data:** While the communication protocol is a black box, the tvtop@5700000 node provides a complete and detailed "recipe" for the electrical and timing characteristics of the LCD panel itself.1 This includes critical parameters such as:  
  * GPIOs for enabling panel power (panel\_power\_en) and backlight (panel\_bl\_en).  
  * The PWM channel and frequency for backlight control (panel\_pwm\_ch, panel\_pwm\_freq).  
  * Precise power-on and power-off timing delays to prevent panel damage.  
  * Low-level electrical values like the panel's dot clock frequency (panel\_dclk\_freq).

This data is invaluable, as it provides the exact parameters needed to drive the panel once the communication protocol with the MIPS co-processor is understood.

---

### **3\. Peripheral Hardware Mapping**

The DTBs provide a comprehensive and precise map of the various hardware peripherals and their connections to the SoC's GPIO pins. This information is a massive accelerator for driver development.

* **Debug Serial Port (UART):** The uart@2500000 node is enabled and its pins are explicitly mapped to PH0 and PH1.1 This provides the exact physical connection points needed to establish a serial console for debugging the boot process.  
* **Wireless Module (WiFi/Bluetooth):** The sdmmc@4021000 controller (sdc1) is configured for SDIO (cap-sdio-irq), confirming the interface for the wireless module. Furthermore, the rfkill node defines the specific GPIOs used for WLAN power (wlan\_regon on pin PL1) and Bluetooth reset (bt\_rst\_n on pin PL4), providing all necessary information to port the AIC8800 driver.1  
* **Motor Controller (Keystone/Focus):** A dedicated motor\_ctr node is present, compatible with "motor-control".1 It meticulously lays out:  
  * The four GPIOs for the motor phases: PH4, PH5, PH6, and PH7.  
  * The stepping sequences for clockwise (motor-cw-table) and counter-clockwise (motor-ccw-table) rotation.  
  * This detailed information allows a developer to write a Linux driver to control the electronic keystone/focus motors without any hardware-level probing.  
* **Infrared (IR) Receiver:** The s\_cir@7040000 node is mapped to pin PL9. Crucially, a separate cir\_param node contains a complete list of the remote control's address and key codes.1 This allows for a near-perfect configuration of a Linux IR driver (  
  lirc) with minimal effort.  
* **Accelerometers:** The twi@2502400 (I2C bus 1\) has two devices attached: stk8ba58@18 and kxttj3@18.1 These are accelerometers, which are almost certainly used for the projector's automatic vertical keystone correction feature.

---

### **4\. Legacy and Modern Configuration Corroboration**

The provided files include two distinct types of configuration, which together offer a complete and verified view of the hardware setup.

* **Modern Kernel DTBs:** Three of the files are standard Device Tree Blobs used by the modern Linux kernel. Their identical content confirms a stable hardware revision.1  
* **Legacy Bootloader Configuration:** The fourth file is in a legacy sys\_config.fex format, which is used by the early-stage U-Boot bootloader.1  
* **Data Consistency:** The configurations in the legacy file perfectly match those in the modern DTBs. For example, both formats specify that the IR receiver uses pin PL9 and both define the same GPIOs for panel power control.1 This cross-verification provides extremely high confidence in the accuracy of the hardware map.

#### **Works cited**

1. 16815c.dtb