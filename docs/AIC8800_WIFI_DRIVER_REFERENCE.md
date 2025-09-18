# AIC8800 WiFi Driver Integration Reference

## Overview
The HY300 projector uses an AIC8800 WiFi module (identified as AW869A in factory analysis). This module requires proprietary drivers that are not yet mainlined in the Linux kernel.

## Driver Source Repositories

### Community Driver Implementations
1. **geniuskidkanyi/aic8800**
   - **URL:** https://github.com/geniuskidkanyi/aic8800
   - **Type:** Community driver port
   - **Status:** Active development
   - **Notes:** General-purpose AIC8800 driver implementation

2. **radxa-pkg/aic8800** 
   - **URL:** https://github.com/radxa-pkg/aic8800
   - **Type:** Radxa's packaging and integration
   - **Status:** Package maintained
   - **Notes:** Radxa's approach to AIC8800 integration for their boards

3. **goecho/aic8800_linux_drvier**
   - **URL:** https://github.com/goecho/aic8800_linux_drvier
   - **Type:** Alternative Linux driver implementation
   - **Status:** Community maintained
   - **Notes:** Another community effort for AIC8800 Linux support

## Integration Strategy

### Phase V Research Tasks
- [ ] Compare driver implementations for feature completeness
- [ ] Analyze hardware interface compatibility with H713/H6 platform
- [ ] Evaluate driver quality and maintenance status
- [ ] Test driver compilation against mainline kernel versions
- [ ] Document any HY300-specific configuration requirements

### Device Tree Integration
The HY300 device tree includes basic AIC8800 configuration:
```dts
wifi: wifi@1 {
    compatible = "aicsemi,aic8800";
    reg = <1>;
    interrupt-parent = <&pio>;
    interrupts = <6 10 IRQ_TYPE_LEVEL_HIGH>; /* PG10 */
    interrupt-names = "host-wake";
    
    aicsemi,wakeup-gpio = <&pio 6 11 GPIO_ACTIVE_HIGH>; /* PG11 */
    aicsemi,reset-gpio = <&pio 6 12 GPIO_ACTIVE_HIGH>;  /* PG12 */
};
```

### Testing Requirements
- **Hardware Access:** WiFi testing requires physical device
- **FEL Mode:** Driver loading can be tested via FEL mode
- **Network Validation:** Full functionality requires network connectivity testing

## Factory Analysis Context
- **Module ID:** AW869A (identified in ROM analysis)
- **Interface:** SDIO via MMC1 controller
- **Power Management:** GPIO-controlled reset and wake pins
- **Interrupt Handling:** Host wake interrupt on PG10

## Integration Priority
- **Phase V.4:** Medium priority (after core system drivers)
- **Dependencies:** Basic MMC/SDIO functionality must work first
- **Testing:** Can be deferred until basic system is stable

## Notes
- All current drivers appear to be out-of-tree (not mainlined)
- Driver selection will depend on code quality and maintenance status
- May require customization for HY300-specific hardware configuration
- Consider contributing improvements back to selected driver project

## Related Documentation
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` - Hardware component status
- `sun50i-h713-hy300.dts` - Device tree WiFi configuration
- `firmware/ROM_ANALYSIS.md` - Original WiFi module identification