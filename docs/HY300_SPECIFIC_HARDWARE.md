### **Executive Summary**

A detailed analysis of the provided Device Tree Blobs (DTBs) offers a remarkably clear blueprint of the HY300's projector-specific hardware systems. Beyond the generic System-on-Chip (SoC) components, the files meticulously document the proprietary hardware that defines the device's function as a projector. This includes the complete control scheme for the electronic keystone motor, the sensors used for automatic keystone correction, the full electrical configuration for the LCD panel and its LED backlight, and the thermal management system.

This information provides a significant advantage for any custom software development. Instead of requiring extensive and difficult hardware probing with tools like logic analyzers, the DTBs serve as a pre-made map, detailing the exact GPIO pins, I2C devices, and PWM channels used to control these custom components. While significant challenges remain in interfacing with the display co-processor, the control mechanisms for nearly every other piece of projector-specific hardware are explicitly defined, dramatically lowering the barrier to creating custom drivers.

---

### **1\. Motor Control System for Electronic Keystone Correction**

The DTBs contain a dedicated node, motor\_ctr, that fully describes the electronic motor system.1 Given that the focus mechanism on this projector is manual, this system is definitively for controlling the electronic keystone correction.1 This node provides a complete software-level guide to operating the motor without needing to probe the physical hardware.

The configuration is exceptionally detailed, specifying:

* **Control Pins:** The four GPIOs that drive the stepper motor phases are explicitly mapped to pins PH4, PH5, PH6, and PH7.1  
* **Stepping Sequence:** The precise bit patterns required to drive the motor in both clockwise (motor-cw-table) and counter-clockwise (motor-ccw-table) directions are provided as tables of values.1  
* **Timing:** The delay between individual phase changes (motor-phase-udelay) and between full steps (motor-step-mdelay) are defined, ensuring smooth motor operation.1  
* **Limit Switch:** A limiter-gpio is mapped to pin PH14, likely connected to a limit switch to detect the motor's end-of-travel position.1

This level of detail allows a developer to write a complete Linux driver to control the projector's physical lens adjustments with high precision, based solely on the information in the DTB.

---

### **2\. Sensors for Automatic Keystone Correction**

To enable the "automatic keystone correction" feature, the projector is equipped with orientation sensors to detect its tilt angle. The DTBs identify these sensors and their connection interface.

* **I2C Bus:** The sensors are connected to the I2C bus at twi@2502400 (twi1).1  
* **Accelerometer Chips:** Two distinct accelerometer devices are defined on this bus:  
  * stk8ba58@18: Compatible with "stk,stk83xx".1  
  * kxttj3@18: Compatible with "kxtj3".1

The presence of these sensors confirms the hardware basis for the auto-keystone feature. A custom Linux system could read data from these I2C devices to determine the projector's physical orientation and apply digital image correction accordingly.

---

### **3\. Optical Engine: LCD Panel and LED Light Source Control**

While the display data is processed by the H713's MIPS co-processor, the electrical control and power sequencing for the physical LCD panel and the white LED light source are specific to the projector's design. The tvtop@5700000 node in the DTB provides a complete electrical "recipe" for these components.1

* **Panel and Backlight Power:** The DTB specifies the exact GPIOs used to control power:  
  * panel\_power\_en: Pin PH19 is used to enable power to the LCD panel itself.1  
  * panel\_bl\_en: Pin PB5 is used to enable the backlight (the main LED light source).1  
* **Backlight Brightness Control:** The brightness of the LED light source is managed via Pulse-Width Modulation (PWM):  
  * panel\_pwm\_ch: Specifies that PWM channel 3 is used for this purpose.1  
  * panel\_pwm\_freq: Sets the PWM frequency to 25,000 Hz (0x61a8).1  
  * panel\_pwm\_pol: Defines the PWM polarity as active high (0x01).1  
* **Power Sequencing:** The DTB defines precise, multi-stage delays for powering the panel on and off (panel\_poweron\_delay0/1/2 and panel\_powerdown\_delay0/1/2) to prevent damage to the LCD.1

This information is critical for safely initializing and controlling the projector's core optical components in a custom software environment.

---

### **4\. Thermal Management and User Interface Elements**

The DTBs also provide clear definitions for other hardware specific to the projector's physical enclosure and operation.

* **Cooling Fan Control:** The thermal management system is described across two nodes:  
  * A fan node with a compatible string of "pwm-fan" indicates the primary cooling fan is speed-controlled via PWM.1  
  * A fan\_ctrl node defines a GPIO on pin PH17 for additional fan control, possibly as a simple on/off switch.1  
* **Status LEDs:** A leds node maps the user-facing status lights to specific GPIOs, allowing for direct control over system status indicators 1:  
  * led-r (pwr-r): Mapped to pin PL0.  
  * led-g (status-g): Mapped to pin PL1.  
  * led-b (led3-b): Mapped to pin PL5.

These definitions provide all the necessary information to manage the device's cooling and display its operational status through the built-in LEDs.

#### **Works cited**

1. fc00.dtb