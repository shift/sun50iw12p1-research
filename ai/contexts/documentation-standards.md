# Documentation Standards for Delegated Agents

## Multi-Document Maintenance Philosophy
Key information is intentionally duplicated across multiple documents for different access patterns:
- **README.md**: High-level project status and quick orientation
- **PROJECT_OVERVIEW.md**: Technical details and phase completion status  
- **Hardware Status Matrix**: Component-specific implementation and driver information
- **Task Documentation**: Detailed implementation steps and external resources
- **Reference Documents**: Deep-dive topics like WiFi drivers or testing methodology

## Documentation Update Protocol
**Always keep documentation synchronized:**
- **After major milestones:** Update README.md, PROJECT_OVERVIEW.md, and instructions
- **Cross-reference updates:** When updating one document, check related documents
- **Status consistency:** Ensure all documents reflect current project phase
- **New findings:** Add external references and resources to appropriate documentation

## Cross-Reference Requirements
When updating documentation, always check and update related files:

### For Hardware Components:
- Update: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
- Cross-check: `docs/HY300_SPECIFIC_HARDWARE.md`
- Reference: Device tree files (`sun50i-h713-hy300.dts`)

### For Driver Development:
- Update specific driver reference docs (e.g., `docs/AIC8800_WIFI_DRIVER_REFERENCE.md`)
- Cross-check: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
- Update: Relevant task documentation

### For Task Updates:
- Update task file status and progress
- Cross-check: `docs/PROJECT_OVERVIEW.md` for phase status
- Update: README.md if major milestone reached

## External Resource Integration
**When valuable external resources are identified:**
- **Update task documentation** with specific references
- **Add to hardware status documents** for easy access
- **Create dedicated reference documents** for complex topics
- **Maintain links in multiple places** for different access patterns
- **Example:** AIC8800 driver references added to task, hardware status, and dedicated reference document

## Documentation Cross-Reference Map

### For Hardware Analysis:
- Start: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
- Details: `docs/HY300_SPECIFIC_HARDWARE.md`
- Testing: `docs/HY300_TESTING_METHODOLOGY.md`
- Device Tree: `sun50i-h713-hy300.dts`

### For Driver Development:
- WiFi: `docs/AIC8800_WIFI_DRIVER_REFERENCE.md`
- MIPS: `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`
- GPU: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` (Mali section)
- General: `docs/PROJECT_OVERVIEW.md` (Phase V planning)

### For Task Management:
- Active: Check `docs/tasks/` directory
- History: `docs/tasks/completed/` directory
- Overview: `docs/tasks/README.md`

## Evidence-Based Documentation
All documentation updates must be supported by:
- Specific file references and line numbers
- Concrete examples from firmware analysis or code
- Measurable success criteria and validation procedures
- Complete audit trail of discoveries and decisions
- Cross-references between related documentation

## Documentation Quality Standards
- Use clear, technical language
- Include specific file paths and line numbers for code references
- Provide concrete examples rather than generic descriptions
- Maintain consistent formatting and structure
- Include validation procedures for technical claims
- Document both successes and failures for future reference