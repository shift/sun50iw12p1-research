# **Project HY300 Kodi Media Center: Requirements Document**

## **1\. Project Vision**

To transform the HY300 Android-based projector into a dedicated, high-performance Kodi media center by porting a minimal Linux OS. The primary goals are to create a seamless "10-foot UI" experience controlled entirely by a remote and to implement a true auto-keystone correction system using an external USB camera for a perfect picture regardless of projector placement.

## **2\. Core Kodi User Experience (Remote-Only) Requirements**

All functionality must be optimized for navigation using a standard D-pad-based remote control (Up, Down, Left, Right, OK/Select, Back, Home, Menu, Volume Up/Down).

### **2.1. Navigation & Interaction**

* **Complete Accessibility:** Every menu, setting, and interactive element within the Kodi interface must be selectable and controllable with the D-pad and OK/Select buttons. There should be no reliance on mouse or touch input.  
* **Visual Focus Indicator:** A clear, high-contrast visual indicator (e.g., highlighting, border) must always be present to show the currently selected UI element.  
* **On-Screen Keyboard:** Any text input field (e.g., search bars, password fields) must automatically trigger a remote-friendly on-screen keyboard.  
* **Context Menu:** The Menu button on the remote, or a long-press of the OK/Select button, must consistently open the context-sensitive menu for the currently selected item.  
* **Back Button Logic:** The Back button must logically navigate to the previous screen or level in the UI hierarchy. Pressing Back from the Home screen should have no effect.  
* **Home Button Logic:** The Home button must always return the user directly to the Kodi main menu screen from any location within the interface.

### **2.2. System & Playback Control**

* **Direct Boot:** The system must boot directly into the Kodi interface, which will act as the primary shell. No underlying desktop environment should be visible to the user.  
* **Playback Controls:** Standard media keys on the remote (Play/Pause, Stop, FFWD, RWD) must be correctly mapped to Kodi's playback functions.  
* **Volume Control:** Volume Up/Down buttons must control the system's master volume, with a clear on-screen display (OSD) volume indicator.  
* **Power Menu:** A long-press of the power button on the remote should bring up a clean, remote-navigable Kodi dialog with options for Power Off System, Reboot, and Quit Kodi.

### **2.3. Input & Source Management**

* **HDMI Input Integration:** An appropriate PVR client addon must be configured to represent the projector's HDMI input as a selectable "channel" within Kodi's Live TV or a similar menu. This will allow the user to switch to the HDMI source seamlessly from the remote without leaving the Kodi interface. The switch should be direct and not require navigating through system-level input menus.

## **3\. Automatic Keystone Correction (USB Camera) Requirements**

This feature aims to use a standard USB Video Class (UVC) camera to automate the geometric correction of the projected image.

### **3.1. Hardware & System**

* **Camera Detection:** The Linux OS must automatically detect and initialize any connected UVC-compliant USB camera on the device's USB-A port. Required v4l2 kernel modules must be included.  
* **Software Dependencies:** The system image must include the OpenCV library (or an equivalent computer vision library) for image analysis.

### **3.2. Calibration Process & UI Flow**

* **Initiation:** The keystone process shall be initiated from a dedicated menu item within Kodi, for example: Settings \> System \> Display \> Run Auto Keystone Calibration.  
* **User Prompt:** Before starting, a dialog box must instruct the user to position the camera so it can see the entire projected image and to press OK to begin.  
* **Calibration Pattern:** Upon initiation, the system will project a high-contrast calibration pattern (e.g., a grid of circles, a checkerboard) onto the display surface.  
* **Image Capture:** The system will capture one or more frames of the projected pattern from the connected USB camera.  
* **Distortion Analysis:**  
  * The computer vision software will analyze the captured image to detect the corners or key feature points of the projected pattern.  
  * It will calculate the perspective transformation (homography matrix) required to map the distorted quadrilateral corners to a perfect rectangle.  
* **Correction Application:**  
  * The calculated transformation matrix will be applied to the video output pipeline (e.g., via DRM/KMS or the display server) to warp the image in real-time, correcting the trapezoidal distortion.  
* **Confirmation & Saving:**  
  * After the correction is applied, a confirmation dialog will appear: "Is the image correctly aligned? \[Save\] \[Retry\] \[Cancel\]".  
  * Save: The new transformation settings are persisted across reboots.  
  * Retry: The calibration process runs again.  
  * Cancel: The changes are discarded, and the display reverts to its previous state.

### **3.3. Fallback**

* The existing manual keystone correction settings within Kodi should be retained as a fallback option in case the automatic calibration is unsuccessful or for fine-tuning.

## **4\. System-Level Requirements**

* **Base OS:** A minimal, embedded Linux distribution (e.g., built with Yocto or Buildroot) is required to ensure fast boot times and low resource overhead.  
* **Kernel:** The Linux kernel must be configured with all necessary drivers for the HY300's specific SoC, including graphics (Mali), networking, USB, and IR remote receiver.  
* **Graphics Stack:** A fully hardware-accelerated graphics stack (e.g., using Lima/Panfrost open-source drivers if compatible, or vendor-provided drivers) is essential for smooth 1080p video playback.  
* **Read-Only Root Filesystem:** To ensure system stability and prevent corruption from improper shutdowns, the root filesystem should be mounted as read-only. User data and Kodi settings will be stored on a separate, writable partition.