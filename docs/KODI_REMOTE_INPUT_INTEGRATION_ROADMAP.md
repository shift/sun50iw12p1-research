# HY300 Remote Input System - Integration Roadmap

**Task:** 025 - Kodi Remote Input System Design  
**Date:** September 18, 2025  
**Phase:** VIII - Kodi Media Center Development  

## Implementation Overview

This roadmap provides step-by-step integration instructions for implementing the complete remote input system on the HY300 projector, ensuring 100% remote-only navigation capability in Kodi.

## Critical Hardware Finding

**🚨 IMPORTANT:** The IR receiver hardware is present in the HY300 but **disabled** in the current device tree. The integration has already been added to `sun50i-h713-hy300.dts` but requires compilation and testing.

---

## Phase 1: Hardware Enablement (Week 1)

### Task 1.1: Device Tree Compilation and Testing

**Status:** Device tree modifications complete, requires testing

**Actions Required:**
```bash
# 1. Compile updated device tree
nix develop -c -- dtc -I dts -O dtb -o sun50i-h713-hy300.dtb sun50i-h713-hy300.dts

# 2. Verify compilation
ls -la sun50i-h713-hy300.dtb
file sun50i-h713-hy300.dtb

# 3. Test via FEL mode (safe testing)
nix develop -c -- sunxi-fel version
# Load and test new DTB (detailed testing protocol in HY300_TESTING_METHODOLOGY.md)
```

**Device Tree Changes Made:**
- ✅ Added IR receiver configuration (`&r_ir` section)
- ✅ Added GPIO PL9 pinctrl configuration (`&r_pio` section)  
- ✅ Referenced factory DTB analysis (register 0x7040000)

**Verification Steps:**
1. **DTB Size Check:** New DTB should be ~11KB (increased from 10.5KB)
2. **Hardware Detection:** Check `/sys/class/rc/` for IR receiver after boot
3. **GPIO Verification:** Confirm PL9 pin assignment in `/sys/kernel/debug/pinctrl/`

### Task 1.2: Kernel Configuration Validation

**Required Kernel Modules:**
```bash
# Check current kernel config
nix develop -c -- zcat /proc/config.gz | grep -E "IR|RC_"

# Required configurations:
CONFIG_IR_SUNXI=y                    # Allwinner IR receiver driver  
CONFIG_RC_CORE=y                     # Remote control core
CONFIG_RC_DECODERS=y                 # IR protocol decoders
CONFIG_IR_NEC_DECODER=y              # NEC protocol decoder
CONFIG_INPUT_EVDEV=y                 # Event device interface
CONFIG_INPUT_UINPUT=y                # User-space input driver
```

**If Missing:** Rebuild kernel with IR support enabled

### Task 1.3: Basic IR Hardware Testing

**Hardware Detection Tests:**
```bash
# 1. Check IR receiver device
ls -la /dev/lirc*
ls -la /sys/class/rc/

# 2. Verify GPIO configuration  
cat /sys/kernel/debug/pinctrl/pio/pins | grep PL9

# 3. Test raw IR reception
cat /dev/lirc0  # Should show data when pressing remote buttons

# 4. Check interrupts
watch -n 1 'cat /proc/interrupts | grep ir'
```

**Expected Results:**
- `/dev/lirc0` device should exist
- `/sys/class/rc/rc0/` directory should be present
- Raw IR data should appear when pressing buttons
- IR interrupts should increment with button presses

---

## Phase 2: LIRC Integration (Week 2)

### Task 2.1: LIRC Installation and Configuration

**Install LIRC Package:**
```bash
# Add LIRC to NixOS configuration
# File: /etc/nixos/configuration.nix
services.lirc = {
  enable = true;
  configs = [ ./docs/templates/hy300-lirc.conf ];
  options = ./docs/templates/lirc_options.conf;
};
```

**Manual Installation Alternative:**
```bash
# Copy configuration files
sudo mkdir -p /etc/lirc/lircd.conf.d/
sudo cp docs/templates/hy300-lirc.conf /etc/lirc/lircd.conf.d/
sudo cp docs/templates/lirc_options.conf /etc/lirc/

# Create LIRC user and permissions
sudo groupadd lirc
sudo usermod -a -G lirc kodi
```

### Task 2.2: Remote Control Code Capture

**Critical Step:** The template configuration contains estimated IR codes. **Actual codes must be captured from the real remote.**

**Code Capture Process:**
```bash
# 1. Start LIRC in learning mode
sudo systemctl stop lircd
sudo irrecord -d /dev/lirc0 -H default /tmp/hy300-learned.conf

# 2. Follow prompts to capture each button
# Press each button when prompted:
# - POWER, UP, DOWN, LEFT, RIGHT, OK, BACK, HOME
# - VOL+, VOL-, PLAY, STOP, FFWD, REW, MENU

# 3. Validate captured configuration
irw --config /tmp/hy300-learned.conf

# 4. Replace template with learned codes
sudo cp /tmp/hy300-learned.conf /etc/lirc/lircd.conf.d/hy300.conf
```

### Task 2.3: LIRC Service Testing

**Service Configuration:**
```bash
# 1. Enable and start LIRC daemon
sudo systemctl enable lircd
sudo systemctl start lircd

# 2. Check service status
sudo systemctl status lircd
journalctl -u lircd -f

# 3. Test button recognition
irw  # Should show button names when pressed

# 4. Test input event generation
evtest /dev/input/by-path/*ir*
```

**Troubleshooting Commands:**
```bash
# Debug LIRC configuration
lircd --nodaemon --device=/dev/lirc0 --listen=8765

# Test specific remote
irsend LIST HY300_REMOTE ""
irsend SEND_ONCE HY300_REMOTE KEY_POWER

# Check device permissions
ls -la /dev/lirc* /dev/input/event*
```

---

## Phase 3: Kodi Integration (Week 3)

### Task 3.1: Kodi Keymap Installation

**Install Custom Keymap:**
```bash
# 1. Create Kodi keymap directory
mkdir -p ~/.kodi/userdata/keymaps/

# 2. Install HY300 keymap
cp docs/templates/hy300-kodi-keymap.xml ~/.kodi/userdata/keymaps/

# 3. Restart Kodi to load keymap
systemctl restart kodi

# 4. Verify keymap loading
grep -i "keymap" ~/.kodi/temp/kodi.log
```

**Keymap Testing Protocol:**
```bash
# Test each navigation context:
# 1. Main menu navigation (all directions)
# 2. Settings menu access and navigation  
# 3. Media library browsing
# 4. Video playback controls
# 5. Virtual keyboard operation
# 6. Context menus
# 7. Power management menu
```

### Task 3.2: Input Event Integration

**Configure Input Device Recognition:**
```bash
# 1. Create udev rule for IR input
cat > /etc/udev/rules.d/71-lirc-hy300.rules << 'EOF'
# IR receiver device permissions
SUBSYSTEM=="rc", ATTRS{name}=="sunxi-ir", MODE="0664", GROUP="input"

# LIRC socket permissions  
SUBSYSTEM=="misc", KERNEL=="lirc*", MODE="0664", GROUP="lirc"

# Input event device for remote
SUBSYSTEM=="input", ATTRS{name}=="HY300_REMOTE", MODE="0644", GROUP="input"
EOF

# 2. Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# 3. Verify input device creation
ls -la /dev/input/by-path/*ir*
```

### Task 3.3: Kodi Input Configuration

**Advanced Settings Configuration:**
```bash
# Create Kodi advanced settings
cat > ~/.kodi/userdata/advancedsettings.xml << 'EOF'
<advancedsettings>
  <input>
    <enableremotecontrol>true</enableremotecontrol>
    <remoterepeat>25</remoterepeat>
    <remotedelay>1</remotedelay>
  </input>
  <gui>
    <algorithmdirtyregions>3</algorithmdirtyregions>
    <nofliptimeout>0</nofliptimeout>
  </gui>
</advancedsettings>
EOF
```

**Test Remote Integration:**
```bash
# 1. Monitor Kodi input events
tail -f ~/.kodi/temp/kodi.log | grep -i "input\|remote\|key"

# 2. Test navigation responsiveness
# Measure button press to UI response time
# Target: <100ms for navigation

# 3. Test all button functions
# Verify every button performs expected action
```

---

## Phase 4: Optimization and Testing (Week 4)

### Task 4.1: Performance Optimization

**Latency Optimization:**
```bash
# 1. Optimize IR receiver settings
echo 1 > /sys/class/rc/rc0/protocols/nec
echo 0 > /sys/class/rc/rc0/protocols/rc-5  # Disable unused protocols

# 2. Optimize LIRC timing
# Edit /etc/lirc/lirc_options.conf:
# resolution = 1000  # 1ms resolution
# repeat-max = 600   # Fast repeat rate

# 3. Test input latency
python3 << 'EOF'
import time, subprocess
start = time.time()
proc = subprocess.Popen(['evtest', '/dev/input/event0'], stdout=subprocess.PIPE, text=True)
for line in proc.stdout:
    if 'EV_KEY' in line and 'value 1' in line:
        latency = (time.time() - start) * 1000
        print(f"Latency: {latency:.1f}ms")
        break
EOF
```

### Task 4.2: Comprehensive Navigation Testing

**100% Remote Accessibility Audit:**
```bash
#!/bin/bash
# comprehensive_navigation_test.sh

echo "HY300 Remote Navigation Comprehensive Test"
echo "Test each function using ONLY the remote control"
echo ""

FUNCTIONS=(
    "Boot to Kodi home screen"
    "Navigate all main menu items"
    "Access and navigate Settings menu"
    "Browse Movies/TV library"
    "Play video content"
    "Control volume during playback"
    "Access context menus"
    "Use virtual keyboard for text input"
    "Configure WiFi settings"
    "Install add-ons"
    "Access power menu and shutdown"
)

for func in "${FUNCTIONS[@]}"; do
    echo "Testing: $func"
    echo "Can this be completed with remote only? (y/n): "
    read -r response
    if [[ "$response" != "y" ]]; then
        echo "❌ FAIL: $func requires non-remote input"
        exit 1
    fi
done

echo "✅ All functions accessible via remote control"
```

### Task 4.3: User Experience Validation

**10-Foot UI Testing:**
```bash
# 1. Distance Testing
# Test remote functionality from 10+ feet away
# Verify IR range and responsiveness

# 2. Visual Feedback Testing  
# Ensure focus indicators are clearly visible
# Test in various lighting conditions

# 3. Navigation Speed Testing
# Time typical user workflows:
# - Home to movie selection: <10 seconds
# - Volume adjustment: <2 seconds  
# - Context menu access: <3 seconds
```

**Elderly-Friendly Operation Validation:**
- [ ] All buttons provide immediate visual feedback
- [ ] Navigation paths are consistent and predictable
- [ ] No complex button combinations required
- [ ] Clear error recovery (Back button always works)
- [ ] Large, high-contrast visual elements
- [ ] Simple power on/off sequence

---

## Integration Checklist

### Hardware Layer ✅
- [x] Device tree IR receiver configuration added
- [ ] DTB compiled and tested via FEL mode
- [ ] IR hardware detection verified
- [ ] GPIO PL9 pin configuration confirmed

### Driver Layer
- [ ] Kernel IR support modules loaded
- [ ] `/dev/lirc0` device created and accessible  
- [ ] Raw IR signal reception confirmed
- [ ] Linux input events generated

### LIRC Layer
- [ ] LIRC daemon installed and configured
- [ ] Remote control codes captured and configured
- [ ] Button recognition verified via `irw`
- [ ] Input event generation confirmed

### Kodi Layer
- [ ] Custom keymap installed and loaded
- [ ] All navigation contexts tested
- [ ] Input latency optimized (<100ms)
- [ ] Advanced settings configured

### System Integration
- [ ] udev rules configured for permissions
- [ ] Power management (sleep/wake) functional
- [ ] Auto-repeat and acceleration tuned
- [ ] Emergency fallback methods tested

### User Experience
- [ ] 100% remote-only navigation confirmed
- [ ] 10-foot UI distance testing passed
- [ ] Elderly-friendly operation validated
- [ ] Performance targets met
- [ ] Documentation completed

---

## Success Criteria

**Functional Requirements:**
1. ✅ **IR Hardware:** Device tree enables IR receiver on GPIO PL9
2. ⏳ **Signal Reception:** All remote buttons recognized by LIRC
3. ⏳ **Input Events:** Button presses generate Linux input events
4. ⏳ **Kodi Integration:** All navigation possible via remote only
5. ⏳ **Response Time:** <100ms latency button-to-UI response

**User Experience Requirements:**
1. ⏳ **Complete Accessibility:** Every Kodi function accessible via remote
2. ⏳ **Intuitive Navigation:** Consistent, predictable button behaviors  
3. ⏳ **Visual Feedback:** Clear focus indicators and status overlays
4. ⏳ **Error Recovery:** Back button provides consistent escape path
5. ⏳ **Power Management:** Remote wake/sleep functionality

**Performance Requirements:**
1. ⏳ **Latency:** Button press to UI response <100ms
2. ⏳ **Range:** 10+ foot operation distance
3. ⏳ **Reliability:** 99%+ button recognition accuracy
4. ⏳ **Battery Life:** Remote battery indicator if available

---

## Next Steps

1. **Immediate:** Test compiled device tree via FEL mode
2. **Priority:** Capture actual remote control IR codes  
3. **Critical:** Validate 100% remote-only navigation
4. **Integration:** Connect with Task 026 (HDMI input) and Task 027 (keystone control)

**Dependencies:**
- **Hardware Access:** Serial console for testing and debugging
- **Remote Control:** Factory HY300 remote for code capture
- **Testing Environment:** FEL mode setup for safe DTB testing

This integration roadmap ensures systematic implementation and validation of the complete remote input system, meeting all requirements for seamless 10-foot UI operation.