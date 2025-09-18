# Hardware Safety Protocols for Delegated Agents

## Critical Safety Rules
**Always follow safe development practices:**
- Test via FEL mode first (USB recovery)
- Maintain complete backups before modifications
- Document hardware access requirements
- Never make irreversible changes without explicit approval

## FEL Mode Recovery
The HY300 projector supports FEL (Flashing/Emergency Loading) mode for safe recovery:
- USB-based recovery mechanism
- Allows testing without permanent modifications
- Required for all hardware validation phases

## Testing Methodology
Reference: `docs/HY300_TESTING_METHODOLOGY.md`

### Safe Testing Sequence
1. **Preparation**: Verify FEL mode access and recovery procedures
2. **Backup**: Create complete firmware backups before modifications
3. **FEL Testing**: Test via FEL mode before permanent installation
4. **Validation**: Verify functionality in controlled environment
5. **Documentation**: Record all testing procedures and results

## Hardware Access Requirements
- Serial console access for debugging
- FEL mode USB connection for safe testing
- Complete firmware backups for recovery
- Documented rollback procedures

## Emergency Procedures
### If Hardware Access Required:
1. **Document hardware access requirements** clearly
2. **Identify safe testing procedures** (FEL mode, etc.)
3. **Plan backup and recovery strategies**
4. **Never proceed without proper safety measures**

### If Testing Fails:
1. Use FEL mode recovery immediately
2. Restore from verified backups
3. Document failure conditions thoroughly
4. Analyze root cause before retry

## Development vs Testing Phases
- **Development Phase**: Software analysis, no hardware required
- **Testing Phase**: Requires hardware access with full safety protocols
- **Validation Phase**: Production testing with complete safety framework

## Safety Validation Checklist
Before any hardware interaction:
- [ ] FEL mode recovery verified
- [ ] Complete backups created
- [ ] Testing procedures documented
- [ ] Rollback plan established
- [ ] Safety protocols reviewed