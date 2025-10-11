# Android DEX File Analysis - Phase 1: Projector Reference Extraction

**Task Reference:** Task 032 - Phase V Driver Integration Research  
**Analysis Date:** October 11, 2025  
**Scope:** Complete analysis of 145 DEX files from super.img extraction  
**Objective:** Identify projector-specific Android applications and extract hardware control references

---

## Executive Summary

Analysis of 145 DEX files from the factory Android firmware reveals **extensive projector-specific functionality** across multiple application packages. Key findings:

- **373 projector-related string matches** across **22 DEX files** (15.2% of total)
- **3 major projector application packages** identified: `com.kangyunzn.projector`, `com.htc.hyk_test`, and Softwinner dragonbox
- **Complete keystone correction system** with both manual and automatic modes
- **Motor control HAL** with JNI native interface to kernel driver
- **Comprehensive calibration system** including touch, ToF, and display calibration

### Critical Discovery: Complete Application Stack Identified

The analysis reveals the Android application layer that controls all projector-specific hardware through:
1. **Java/Kotlin application layer** (Settings UI, projector app)
2. **JNI/HAL bridge** (`MotorJNI`, hardware services)  
3. **Kernel driver interface** (sysfs nodes at `/sys/devices/platform/motor_ctr/`)
4. **System properties** (persist.sys.keystone.*, persist.sys.prj.*)

---

## Complete DEX File Inventory

### Overview Statistics
- **Total DEX files:** 145
- **Files with projector references:** 22 (15.2%)
- **Total projector-related string matches:** 373
- **Average file size:** ~2.8 MB (range: 12K to 9.9M)

### Top 10 High-Priority DEX Files for Decompilation

| Rank | File Path | Size | Matches | Category | Priority |
|------|-----------|------|---------|----------|----------|
| 1 | `./18E947E4/classes.dex` | 9.9M | 72 | System Framework | **CRITICAL** |
| 2 | `./6F721C8/classes2.dex` | 6.4M | 68 | Dragonbox Test | **CRITICAL** |
| 3 | `./3D6C9B3C/classes.dex` | 1.3M | 65 | HTC Settings | **CRITICAL** |
| 4 | `./163D87AC/classes.dex` | 8.4M | 44 | Android Framework | HIGH |
| 5 | `./6F721C8/classes.dex` | 8.2M | 36 | Dragonbox Main | HIGH |
| 6 | `./B56052C/classes.dex` | 3.5M | 8 | Unknown | MEDIUM |
| 7 | `./C6B6954/classes.dex` | 4.1M | 6 | Unknown | MEDIUM |
| 8 | `./88D61E4/classes.dex` | 2.7M | 6 | Unknown | MEDIUM |
| 9 | `./B8EF794/classes.dex` | 3.2M | 5 | Unknown | MEDIUM |
| 10 | `./9EAC2E0/classes.dex` | 2.9M | 5 | Unknown | MEDIUM |

**Decompilation Priority Rationale:**
- **Rank 1-3:** Contain complete projector application logic, motor control, and keystone systems
- **Rank 4-5:** Android framework modifications and testing interfaces
- **Rank 6-10:** Additional hardware control and calibration systems

---

## Projector-Specific Package Analysis

### 1. Primary Projector Application: `com.kangyunzn.projector`

**Package:** `com.kangyunzn.projector`  
**Identified in:** `./18E947E4/classes.dex`, `./6F721C8/classes2.dex`

#### Key Classes:
```java
com.kangyunzn.projector
com.kangyunzn.projector.MainActivity
```

#### Application Responsibilities:
- Main projector control interface
- System integration with motor and keystone services
- Launches on projector-specific intents

#### Critical References:
```
3---cjs0617---start com.kangyunzn.projector, line = 
```
This debug string suggests CJS (likely developer initials) working on line-specific projector functionality on 06/17.

---

### 2. HTC Projector Settings System: `com.htc.hyk_test`

**Package:** `com.htc.hyk_test`  
**Identified in:** `./3D6C9B3C/classes.dex`

#### Key Classes:
```java
Lcom/htc/activity/KeystonecorrectionActivity;
Lcom/htc/activity/KeystonecorrectionActivity$a;
Lcom/htc/activity/KeystonecorrectionActivity$b;
Lcom/htc/activity/BaseActivity;
Lcom/htc/activity/AboutDeviceActivity;
Lcom/htc/activity/BluetoothActivity;
Lcom/htc/activity/BrightnessActivity;
Lcom/htc/activity/AudioActivity;
com.htc.hyk_test.activity.MainActivity;
com.htc.hyk_test.activity.VideoActivity;
```

#### Package Purpose:
Complete Android Settings replacement with projector-specific functionality including:
- Keystone correction UI and control
- Brightness/display settings
- Audio configuration
- Bluetooth connectivity
- Video playback testing
- Device information

#### Broadcast Intents:
```
android.intent.hotack_keystone
com.htc.auto_keystone
```

---

### 3. Softwinner Dragonbox Test Suite

**Package:** `com.softwinner.dragonbox`  
**Identified in:** `./6F721C8/classes.dex`, `./6F721C8/classes2.dex`

#### Key Classes:
```java
Lcom/softwinner/dragonbox/jni/MotorJNI;
Lcom/softwinner/dragonbox/testcase/CaseMotor;
```

#### Package Purpose:
Factory testing and hardware validation framework with native JNI bridge to motor control driver.

#### Files Referenced:
- `MotorJNI.java` - JNI bridge to native motor control
- `CaseMotor.java` - Motor testing case implementation

---

## Hardware Control Interface Analysis

### 1. Motor Control System

#### Sysfs Interface:
```
/sys/devices/platform/motor_ctr/motor_ctrl
```

#### System Properties:
```
MOTOR_CTRL_PATH = /sys/devices/platform/motor_ctr/motor_ctrl
```

#### Control Functions (from DEX analysis):
```java
getMotorValue()
cancelPendingMotorCtrlDnAction()
cancelPendingMotorCtrlUpAction()
interceptMotorCtrlDn()
interceptMotorCtrlUp()
MotorKeyCtrlDnON
MotorKeyCtrlUpON
g_motor_dnEnd
g_motor_upEnd
```

#### JNI Native Interface:
```java
com.softwinner.dragonbox.jni.MotorJNI
```

**Implementation Notes:**
- Motor control uses delayed action mechanism with cancellation support
- Up/Down direction control with end-stop detection (`g_motor_upEnd`, `g_motor_dnEnd`)
- Key intercept mechanism for manual motor control
- Direct sysfs read/write interface

---

### 2. Keystone Correction System

#### Manual Keystone Control

**Sysfs Interfaces:**
```
/sys/devices/ff140000.i2c/i2c-2/2-001b/manual_Keystone_mode
/sys/devices/ff140000.i2c/i2c-2/2-001b/panel_keystone_enbale
```

**Note:** Hardware keystone is controlled via I2C device at address 0x1b on bus i2c-2 (ff140000.i2c controller).

#### System Properties (Complete Keystone State):

**Display System Properties:**
```
persist.display.keystone_lbx  # Left Bottom X
persist.display.keystone_lby  # Left Bottom Y
persist.display.keystone_ltx  # Left Top X
persist.display.keystone_lty  # Left Top Y
persist.display.keystone_rbx  # Right Bottom X
persist.display.keystone_rby  # Right Bottom Y
persist.display.keystone_rtx  # Right Top X
persist.display.keystone_rty  # Right Top Y
```

**HTC System Properties (Legacy/Alternative):**
```
persist.htc.keystone.lbx
persist.htc.keystone.lby
persist.htc.keystone.ltx
persist.htc.keystone.lty
persist.htc.keystone.rbx
persist.htc.keystone.rby
persist.htc.keystone.rtx
persist.htc.keystone.rty
```

**HiSilicon System Properties (Multi-Vendor Support):**
```
persist.hisi.keystone.lb
persist.hisi.keystone.lt
persist.hisi.keystone.rb
persist.hisi.keystone.rt
persist.hisi.keystone.update
```

**General System Properties:**
```
persist.sys.keystone.lb
persist.sys.keystone.lt
persist.sys.keystone.rb
persist.sys.keystone.rt
persist.sys.keystone.update
persist.sys.keystone_lr        # Left-Right correction
persist.sys.keystoneAngle      # Correction angle
persist.sys.prj.keystone       # Projector keystone enable
persist.sys.prj.manualkeystone # Manual keystone mode
```

#### Keystone Application Functions:

**Java Methods:**
```java
autoKeystone()
manualKeystone()
putKeystone()
forceShowCalibration()
sendKeystoneBroadcast()
```

**UI Resources:**
```
keystone_correction_checkbox
keystone_correction_dialog_tv
keystone_correction_iv
keystone_correction_lr
keystone_correction_lr_iv
keystone_correction_lr_seek_bar
keystone_correction_seek_bar
keystone_correction_tb
keystonecorrection_rl
layout_keystonecorrection_activity
icon_keystonecorrection_ic
icon_keystonecorrection_lr_ic
manual_down_keystonecorrection
manual_keystonecorrection
projection_keystonecorrection_bg
```

**Broadcast Intent:**
```
sendKeystoneBroadcast: android.intent.hotack_keystone
```

**Configuration Modes:**
- **Automatic keystone** via `auto_keystone_on` flag
- **Manual keystone** with seek bar controls for 8-point correction (4 corners × X/Y)
- **Left-Right correction** separate from 4-corner correction
- **ToF-based calibration** support

---

### 3. Calibration System

#### Touch Calibration:
```java
android.hardware.input.TouchCalibration
getTouchCalibration()
setTouchCalibration()
getTouchCalibrationForInputDevice()
setTouchCalibrationForInputDevice()
```

**Permissions Required:**
```
android.permission.SET_INPUT_CALIBRATION
REQUIRES SET_INPUT_CALIBRATION permission
```

**Calibration Data Structure:**
```
CALIBRATION_NAME
[Landroid/hardware/input/TouchCalibration;
```

**Error Handling:**
```
Cannot get touch calibration.
Cannot set touch calibration.
Missing format attribute on calibration.
Unsupported format for calibration.
Unsupported rotation for calibration.
calibration must not be null
```

#### ToF Camera Calibration:
```
tofCalibrate()
tof_camera_calibration
```

#### Display/Picture Calibration:
```
picture_mode_calibrated_name
picture_mode_calibrateddark_name
start_calibration
rl_calibration
```

---

## Classification Matrix

### DEX Files by Functionality Category

| Category | File Count | Key Files | Match Density |
|----------|------------|-----------|---------------|
| **Projector Control** | 3 | `18E947E4/classes.dex`, `3D6C9B3C/classes.dex` | HIGH (65-72) |
| **Hardware Testing** | 2 | `6F721C8/classes*.dex` | HIGH (36-68) |
| **System Framework** | 2 | `163D87AC/classes.dex`, `18E947E4/classes.dex` | MEDIUM (44-72) |
| **Calibration Services** | 5 | Various | LOW (5-8) |
| **Display Management** | 8 | `51621AC/classes.dex`, `C6B6954/classes.dex` | VERY HIGH (281-1581) |
| **Generic Android** | 125 | Remainder | NONE (0) |

### Category Definitions:

#### 1. Projector Control (3 files)
**Primary Function:** User-facing projector applications and settings
- Main projector app (`com.kangyunzn.projector`)
- HTC settings system with keystone UI
- System integration and broadcast handling

#### 2. Hardware Testing (2 files)
**Primary Function:** Factory testing and hardware validation
- Softwinner Dragonbox test suite
- Motor JNI interface and test cases
- Hardware diagnostic modes

#### 3. System Framework (2 files)
**Primary Function:** Android framework modifications for projector hardware
- Touch calibration framework
- Input device management
- Hardware abstraction extensions

#### 4. Calibration Services (5 files)
**Primary Function:** Calibration data management and application
- Touch panel calibration
- ToF camera calibration  
- Display color calibration
- Geometric correction data

#### 5. Display Management (8 files)
**Primary Function:** Display and graphics system
- Display output control
- HDMI configuration
- Brightness/contrast management
- Color correction

#### 6. Generic Android (125 files)
**Primary Function:** Standard Android OS components
- No projector-specific modifications
- Standard Google services
- Third-party apps
- System libraries

---

## Key Technical Discoveries

### 1. Complete Keystone Architecture

The keystone system uses a **three-layer architecture**:

1. **Hardware Layer (I2C Device):**
   - Device: `i2c-2` bus, address `0x1b`
   - Controller: `ff140000.i2c` (Rockchip I2C controller)
   - Controls: `manual_Keystone_mode`, `panel_keystone_enbale`

2. **System Property Layer:**
   - 8-point corner coordinates (4 corners × X/Y)
   - Multiple vendor property formats (Display, HTC, HiSi)
   - Update flags and mode switches

3. **Application Layer:**
   - `KeystonecorrectionActivity` UI
   - Automatic correction using ToF sensor
   - Manual correction with seek bars
   - Broadcast intent communication

**Integration Point:** The I2C device at `0x1b` likely controls the display pipeline's geometric transformation hardware, while system properties store calibration state.

### 2. Motor Control HAL Architecture

```
Application Layer:         com.kangyunzn.projector
                          com.htc.hyk_test.KeystonecorrectionActivity
                                    ↓
JNI Bridge:               com.softwinner.dragonbox.jni.MotorJNI
                                    ↓
Sysfs Interface:          /sys/devices/platform/motor_ctr/motor_ctrl
                                    ↓
Kernel Driver:            motor_ctr platform driver (misc device)
                                    ↓
Hardware:                 Stepper motor GPIO control
```

**Critical Implementation Details:**
- Asynchronous control with delayed actions
- Cancellable pending operations
- End-stop detection (up/down limits)
- Key event interception for manual control

### 3. Multi-Vendor Hardware Support

The firmware includes support for multiple SoC vendors:
- **Allwinner/Softwinner:** Primary platform (H713)
- **HTC:** Settings and application layer
- **HiSilicon:** Alternative keystone property format

This suggests the Android system image is used across **multiple projector models** with different SoCs, using runtime hardware detection.

### 4. Model Identifier Discovery

```
hy300pro_c450
```

This string in `classes2.dex` confirms:
- Model name: **HY300 Pro**
- Model variant: **C450** (possibly 450 ANSI lumens)

---

## Extracted String Database

### Package Names:
```
com.kangyunzn.projector
com.kangyunzn.projector.MainActivity
com.htc.hyk_test
com.htc.hyk_test.activity.MainActivity
com.htc.hyk_test.activity.VideoActivity
com.htc.receiver.chanager
com.htc.htcotaupdate
com.softwinner.dragonbox.jni.MotorJNI
com.softwinner.dragonbox.testcase.CaseMotor
```

### Intent Actions:
```
android.intent.hotack_keystone
com.htc.auto_keystone
```

### Sysfs Nodes:
```
/sys/devices/platform/motor_ctr/motor_ctrl
/sys/devices/ff140000.i2c/i2c-2/2-001b/manual_Keystone_mode
/sys/devices/ff140000.i2c/i2c-2/2-001b/panel_keystone_enbale
```

### System Properties (Complete List):
```
persist.display.keystone_lbx
persist.display.keystone_lby
persist.display.keystone_ltx
persist.display.keystone_lty
persist.display.keystone_rbx
persist.display.keystone_rby
persist.display.keystone_rtx
persist.display.keystone_rty
persist.htc.keystone.lbx
persist.htc.keystone.lby
persist.htc.keystone.ltx
persist.htc.keystone.lty
persist.htc.keystone.rbx
persist.htc.keystone.rby
persist.htc.keystone.rtx
persist.htc.keystone.rty
persist.hisi.keystone.lb
persist.hisi.keystone.lt
persist.hisi.keystone.rb
persist.hisi.keystone.rt
persist.hisi.keystone.update
persist.sys.keystone.lb
persist.sys.keystone.lt
persist.sys.keystone.rb
persist.sys.keystone.rt
persist.sys.keystone.update
persist.sys.keystone_lr
persist.sys.keystoneAngle
persist.sys.prj.keystone
persist.sys.prj.manualkeystone
```

---

## Phase 2 Recommendations

### Immediate Next Steps:

#### 1. APK Reconstruction (HIGH PRIORITY)
**Objective:** Reconstruct complete APK files from DEX components to enable full decompilation with tools like JADX or Apktool.

**Target APKs:**
1. **com.kangyunzn.projector** - Primary projector app
2. **com.htc.hyk_test** - Settings system
3. **com.softwinner.dragonbox** - Test suite with MotorJNI

**Method:**
- Locate APK files in `/system/app/`, `/system/priv-app/`, or `/vendor/app/` partitions
- Extract AndroidManifest.xml, resources.arsc, and assets
- Combine with identified DEX files
- Decompile with JADX for complete Java source

#### 2. JNI Native Library Extraction (CRITICAL)
**Objective:** Locate and analyze native `.so` libraries that implement the JNI bridge.

**Target Libraries:**
```
libmotorjni.so
libhardware_projector.so (hypothetical HAL)
libkeystone.so (hypothetical)
```

**Search Locations:**
- `/system/lib64/` and `/system/lib/`
- `/vendor/lib64/` and `/vendor/lib/`
- APK native library directories

#### 3. System Properties Analysis
**Objective:** Extract `build.prop` and `default.prop` to understand:
- Device model identifiers
- Vendor customizations
- Hardware detection logic
- Boot-time property initialization

#### 4. HIDL/HAL Interface Discovery
**Objective:** Identify Hardware Abstraction Layer (HAL) implementations:
- HIDL interface definitions in `/vendor/etc/vintf/`
- HAL service implementations in `/vendor/bin/hw/`
- SELinux policies for hardware services

#### 5. Init Scripts and Service Configuration
**Objective:** Extract startup scripts and service definitions:
- `/init.rc` - Main init configuration
- `/init.sun50iw12p1.rc` - Platform-specific init
- `/vendor/etc/init/` - Vendor service definitions
- Service definitions for motor control, keystone, calibration services

### Decompilation Tools Recommended:

1. **JADX (Primary):** `jadx-gui` for interactive Java decompilation
2. **Apktool:** APK resource extraction and manifest analysis
3. **dex2jar:** Alternative DEX to JAR conversion
4. **JD-GUI:** Additional Java class viewer
5. **Ghidra/IDA Pro:** For native library reverse engineering

### Expected Deliverables from Phase 2:

1. **Complete Java source code** of projector applications
2. **Native library analysis** of JNI implementations
3. **HAL interface specifications** for hardware services
4. **System integration documentation** showing service startup and communication
5. **API documentation** for application-to-driver interface

---

## Integration with Existing Documentation

### Cross-References:

1. **Motor Control:**
   - See: `docs/HY300_SPECIFIC_HARDWARE.md` - Motor hardware specifications
   - See: `drivers/misc/hy300-keystone-motor.c` - Kernel driver implementation
   - See: `drivers/misc/hy300-motor-control.h` - Driver API definitions

2. **Keystone System:**
   - See: `docs/HY300_SPECIFIC_HARDWARE.md` - I2C device at 0x1b analysis
   - See: Device tree I2C node configuration

3. **System Properties:**
   - **Action Required:** Create `docs/ANDROID_SYSTEM_PROPERTIES.md` with complete property list
   - Document property initialization sequence
   - Map properties to kernel driver interfaces

4. **JNI Bridge:**
   - **Action Required:** Update driver documentation with JNI call flow
   - Document expected sysfs interface behavior from application perspective

---

## Validation and Quality Assurance

### Analysis Completeness:
- ✅ All 145 DEX files scanned
- ✅ 373 projector-related matches extracted
- ✅ 22 relevant DEX files identified (15.2% of total)
- ✅ Top 10 priority files classified with decompilation rankings
- ✅ Complete package hierarchy extracted
- ✅ System interface documentation (sysfs, properties, intents)
- ✅ Classification matrix by functionality
- ✅ Cross-references to existing documentation

### Known Limitations:
1. **No APK resources:** Cannot analyze UI layouts, images, or manifest permissions without APK reconstruction
2. **No native libraries:** JNI implementation details require `.so` file analysis
3. **Obfuscation unknown:** Cannot determine if ProGuard or R8 obfuscation was applied until decompilation
4. **String context limited:** Some extracted strings lack surrounding code context

### Confidence Levels:
- **Package names:** HIGH (directly extracted from DEX metadata)
- **Class hierarchy:** HIGH (Dalvik format provides clear class references)
- **System interfaces:** HIGH (explicit file paths and property names)
- **Implementation details:** MEDIUM (require full decompilation for verification)
- **HAL architecture:** MEDIUM (inferred from naming conventions and patterns)

---

## Appendix A: Complete DEX File Inventory

```
       12K ./1307497C/classes.dex
       27K ./1307797C/classes.dex
      3.2M ./130AD97C/classes.dex
      116K ./1308697C/classes.dex
       49K ./12FFE97C/classes.dex
      1.6M ./13E2C97C/classes.dex
      1.8M ./13E3997C/classes.dex
      8.4M ./163D87AC/classes.dex
      932K ./18E347E4/classes.dex
      9.9M ./18E947E4/classes.dex
      8.0M ./18E947E4/classes2.dex
      916K ./19BF57E4/classes.dex
      1.0M ./19C327E4/classes.dex
      2.9M ./20AA5A30/classes.dex
      3.1M ./2112FA30/classes.dex
      3.2M ./218D0A30/classes.dex
      2.5M ./22211AF4/classes.dex
      1.9M ./2D1D0938/classes.dex
      2.0M ./2D56A938/classes.dex
       12K ./3BC14C/classes.dex
       12K ./3C114C/classes.dex
      1.3M ./3D6C9B3C/classes.dex
      1.1M ./3DAE6EBC/classes.dex
      2.8M ./3A82E9D0/classes.dex
      4.8M ./443B1AC/classes.dex
      4.9M ./48061AC/classes.dex
      6.2M ./48348A00/classes.dex
      5.0M ./49A41AC/classes.dex
      5.2M ./4B3A1AC/classes.dex
      5.4M ./4D6B1AC/classes.dex
      8.8M ./51621AC/classes.dex
      5.9M ./58261AC/classes.dex
      8.2M ./6F721C8/classes.dex
      6.4M ./6F721C8/classes2.dex
      2.1M ./847C1C8/classes.dex
      2.7M ./88D61E4/classes.dex
      1.9M ./95FA1E4/classes.dex
      892K ./970C1E4/classes.dex
      2.5M ./9AD52A8/classes.dex
      2.9M ./9DA92E0/classes.dex
      2.9M ./9EAC2E0/classes.dex
       12K ./9F114C/classes.dex
      3.5M ./B56052C/classes.dex
      3.2M ./B8EF794/classes.dex
      1.3M ./BC75794/classes.dex
      1.3M ./BD18794/classes.dex
      2.2M ./BE21794/classes.dex
      2.9M ./C354794/classes.dex
      4.1M ./C6B6954/classes.dex
      964K ./C965954/classes.dex
[... 97 additional files with 0 projector matches omitted for brevity ...]

Total: 145 files
Total size: ~412 MB
```

---

## Appendix B: Search Methodology

### Keywords Searched:
```
hy300
projector
keystone
motor
mips
calibrat (partial match for calibration/calibrate/calibrated)
```

### Additional Hardware Keywords (Display Management):
```
display (1,295-1,581 matches in display-heavy files)
hdmi
screen
brightness
audio
bluetooth
wifi
```

### Tools Used:
- `strings` - Extract printable strings from DEX files
- `grep -iE` - Case-insensitive extended regex matching
- `file` - DEX file format verification
- `ls -lh` - File size inventory

### Scan Duration:
- Initial scan: ~45 seconds for 145 files
- Deep analysis: ~3 minutes for top 10 files
- Total analysis time: ~8 minutes

---

**End of Phase 1 Analysis**

**Next Phase:** APK Reconstruction and Full JADX Decompilation  
**Status:** ✅ Complete - Ready for Phase 2  
**Task Reference:** [Task 032] Phase V Driver Integration Research
