# AI Agent Guidelines for HY300 Linux Porting Project

## Project Overview
This is a hardware porting project to run mainline Linux on the HY300 Android projector with Allwinner H713 SoC. We follow a rigorous, phase-based approach with no shortcuts or mock implementations.

## Documentation Structure

## Documentation Structure

### Primary Documentation
- `README.md` - Project overview and current status
- `docs/instructions/README.md` - Comprehensive development guide
- `docs/PROJECT_OVERVIEW.md` - Technical overview and phase completion status
- `firmware/ROM_ANALYSIS.md` - Complete ROM analysis results
- `AGENTS.md` - This file (AI agent guidelines)

### Task Management System
- `docs/tasks/` - Active tasks directory
- `docs/tasks/completed/` - Completed tasks archive
- `docs/tasks/README.md` - Task management overview

### Hardware and Device Tree Documentation
- `sun50i-h713-hy300.dts` - **MAIN DELIVERABLE**: Complete mainline device tree
- `sun50i-h713-hy300.dtb` - Compiled device tree blob (10.5KB)
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` - Hardware component status matrix
- `docs/HY300_TESTING_METHODOLOGY.md` - Safe testing procedures with FEL recovery
- `docs/HY300_SPECIFIC_HARDWARE.md` - Projector-specific hardware documentation
- `docs/FACTORY_DTB_ANALYSIS.md` - Detailed DTB hardware analysis
- `docs/DTB_ANALYSIS_COMPARISON.md` - Previous analysis error corrections

### Driver Integration Documentation
- `docs/AIC8800_WIFI_DRIVER_REFERENCE.md` - WiFi driver implementation references
- `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md` - Complete firmware component analysis
- `firmware/DRAM_ANALYSIS.md` - DRAM parameter analysis

### Analysis and Context
- `firmware/` - ROM analysis results and extracted components
- `firmware/extracted_components/` - Kernel, initramfs, and MIPS firmware
- `flake.nix` - Development environment configuration
- `.envrc` - direnv auto-activation

### Custom Analysis Tools
- `tools/analyze_boot0.py` - Python script to extract DRAM parameters from boot0.bin
- `tools/compare_dram_params.py` - Tool to compare extracted DRAM parameters with U-Boot defaults
- `tools/hex_viewer.py` - Interactive hex viewer for firmware analysis

## Agent Workflow Rules

### 1. Task Continuity (CRITICAL)
**Always check for in-progress tasks first:**
```bash
# Check for any in-progress or pending tasks
find docs/tasks/ -name "*.md" -exec grep -l "Status.*in_progress\|Status.*pending" {} \;
```

**Priority Order:**
1. **Continue last worked task** - Check `docs/tasks/` for in_progress status
2. **Pick up pending tasks** - Next highest priority pending task
3. **Create new task** - Only if no pending tasks exist

### 2. No Shortcuts Policy (MANDATORY)
- **NEVER mock, stub, or simulate code** that should be functional
- **NEVER skip testing** - always run available tests
- **NEVER disable tests** - fix issues instead
- **NEVER cut corners** - break complex tasks into smaller ones instead
- **ALWAYS implement complete solutions** - partial implementations are unacceptable

### 3. Development Environment
**Always verify Nix environment:**
```bash
# Check if in devShell
echo $IN_NIX_SHELL

# If not in devShell, use:
nix develop -c -- <command>
```

**Required for all development:**
- Cross-compilation toolchain (aarch64-unknown-linux-gnu-*)
- sunxi-tools for FEL mode operations
- Firmware analysis tools (binwalk, strings, hexdump)
- Device tree compiler (dtc)

### 4. Task Documentation Standards
**Every task must have:**
- Clear objective and success criteria
- Prerequisites and dependencies
- Implementation steps with verification
- Quality validation procedures
- Next task preparation

**Task Status Lifecycle:**
- `pending` - Task defined, not started
- `in_progress` - Currently being worked on (ONLY ONE AT A TIME)
- `completed` - Finished with validation
- `blocked` - Cannot proceed (document blocker)

### 5. Git and File Management
**Critical Rules:**
- ROM files (`*.img`, `boot0.bin`) are excluded from git
- Always commit documentation and code changes
- Reference task numbers in commit messages
- Never commit binaries or test scripts unless part of testing framework

### 6. Hardware Safety Protocol
**Always follow safe development practices:**
- Test via FEL mode first (USB recovery)
- Maintain complete backups before modifications
- Document hardware access requirements
- Never make irreversible changes without explicit approval

### 7. Documentation Update Protocol
**Always keep documentation synchronized:**
- **After major milestones:** Update README.md, PROJECT_OVERVIEW.md, and instructions
- **Cross-reference updates:** When updating one document, check related documents
- **Status consistency:** Ensure all documents reflect current project phase
- **New findings:** Add external references and resources to appropriate documentation
- **Example:** When WiFi driver references are found, update task documentation, hardware status matrix, and create dedicated reference documents

### 8. Research Phase Protocol
**For tasks that can be done without hardware:**
- **Maximize software analysis** before requiring hardware access
- **Extract maximum information** from existing firmware components
- **Document driver patterns** and integration requirements
- **Create integration roadmaps** to minimize hardware testing iterations
- **Research external resources** like community driver implementations

### 9. External Resource Integration
**When valuable external resources are identified:**
- **Update task documentation** with specific references
- **Add to hardware status documents** for easy access
- **Create dedicated reference documents** for complex topics
- **Maintain links in multiple places** for different access patterns
- **Example:** AIC8800 driver references added to task, hardware status, and dedicated reference document

## Current Project Status

### Phase I: Firmware Analysis ✅ COMPLETED
- ROM structure analyzed and documented
- Boot0 bootloader extracted with DRAM parameters
- Device trees identified and extracted
- Development environment established

### Phase II: U-Boot Porting ✅ COMPLETED
- DRAM parameter extraction from boot0.bin
- U-Boot configuration and compilation for H713
- Complete bootloader binaries built and ready

### Phase III: Additional Firmware Analysis ✅ COMPLETED
- MIPS co-processor firmware (display.bin) extracted
- Android kernel and initramfs analysis completed
- Complete hardware component inventory documented

### Phase IV: Mainline Device Tree Creation ✅ COMPLETED
- Complete mainline device tree created (`sun50i-h713-hy300.dts`)
- All hardware components configured and validated
- Device tree compilation verified (10.5KB DTB)

### Phase V: Driver Integration 🎯 CURRENT PHASE
**Current Status:** Research phase active (Task 009), hardware testing phase ready
**Research Focus:** Reverse engineering factory firmware for driver patterns
**Hardware Dependencies:** Serial console access, FEL mode testing for validation phase

### Current Active Tasks
- **Task 009:** 🔄 Phase V Driver Integration Research (pending - ready to start)

### Known Completed Tasks
1. **Task 001:** ✅ Setup ROM Analysis Workspace
2. **Task 002:** ✅ Extract DRAM parameters from boot0.bin
3. **Task 003:** ✅ U-Boot Integration and Compilation
4. **Task 004:** ✅ Complete DTB Analysis Revision
5. **Task 006:** ✅ Extract Additional Firmware Components
6. **Task 007:** ✅ Extract MIPS Firmware and Complete Phase III
7. **Task 008:** ✅ Phase IV - Mainline Device Tree Creation

## Agent Decision Framework

### When Starting Work:
1. **Check `docs/tasks/`** for any in_progress tasks
2. **Continue in_progress task** if found
3. **Check for pending tasks** if no in_progress work
4. **Review task dependencies** before starting
5. **Update task status** to in_progress when beginning

### When Creating New Tasks:
- **Break down complexity** - prefer atomic, testable tasks
- **Document prerequisites** - what must be completed first
- **Define success criteria** - clear, measurable outcomes
- **Estimate effort** - realistic time and resource requirements

### When Completing Tasks:
- **Validate all success criteria** met
- **Run tests and verification** procedures
- **Update documentation** with results
- **Mark task completed** and move to completed/ directory
- **Identify next task** dependencies and priorities

## Technical Standards

### Code Quality
- Follow existing codebase conventions
- Use established libraries and utilities
- Implement complete, working solutions
- Include comprehensive error handling
- Document complex technical decisions

### Testing Requirements
- Run all available tests before task completion
- Fix test failures rather than disabling tests
- Create tests for new functionality when applicable
- Verify cross-compilation builds successfully

### Custom Tool Usage
- Use `tools/analyze_boot0.py` for DRAM parameter extraction
- Use `tools/compare_dram_params.py` for parameter validation
- Use `tools/hex_viewer.py` for interactive firmware analysis
- Tools are designed to work within the Nix development environment
- All tools include comprehensive error handling and validation

### Documentation Requirements
- Update relevant documentation with changes
- Maintain technical accuracy in all documentation
- Include implementation details for future reference
- Document known issues and workarounds

## Emergency Procedures

### If Stuck on Technical Issues:
1. **Document the specific problem** in task notes
2. **Research alternative approaches** thoroughly
3. **Break problem into smaller sub-tasks** if needed
4. **Never skip or mock the problematic component**
5. **Update task status to blocked** with clear blocker description

### If Hardware Access Required:
1. **Document hardware access requirements** clearly
2. **Identify safe testing procedures** (FEL mode, etc.)
3. **Plan backup and recovery strategies**
4. **Never proceed without proper safety measures**

## Success Metrics

### Task Completion Standards:
- All acceptance criteria met and verified
- Documentation updated and accurate
- Tests passing (if applicable)
- Code compiles without errors/warnings
- No technical debt introduced

### Project Progress Indicators:
- Phase milestones achieved with validation
- Critical path blockers resolved
- Hardware safety maintained throughout
- Complete audit trail of decisions and changes

## Key Project Deliverables

### **Phase I-IV Completed Deliverables**
- ✅ **U-Boot Bootloader**: `u-boot-sunxi-with-spl.bin` (657.5 KB) - Ready for FEL testing
- ✅ **Mainline Device Tree**: `sun50i-h713-hy300.dts` (791 lines) → `sun50i-h713-hy300.dtb` (10.5 KB)
- ✅ **Hardware Documentation**: Complete component analysis and enablement status
- ✅ **Development Environment**: Nix-based cross-compilation toolchain
- ✅ **Analysis Tools**: Custom Python tools for firmware analysis
- ✅ **Safety Framework**: FEL recovery procedures and testing methodology

### **Current Phase V Focus**
- 🔄 **Driver Integration Research**: Factory firmware analysis for driver patterns
- 🎯 **Next**: Hardware validation and driver testing via FEL mode

### **Critical External Resources Identified**
- **AIC8800 WiFi Drivers**: 3 community implementations documented
- **Sunxi Tools**: FEL mode recovery and testing utilities
- **Mali-Midgard**: GPU driver options (Panfrost vs proprietary)
- **Device Tree References**: H6 compatibility layer for H713

## Documentation Cross-Reference Map

### **For Hardware Analysis:**
- Start: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
- Details: `docs/HY300_SPECIFIC_HARDWARE.md`
- Testing: `docs/HY300_TESTING_METHODOLOGY.md`
- Device Tree: `sun50i-h713-hy300.dts`

### **For Driver Development:**
- WiFi: `docs/AIC8800_WIFI_DRIVER_REFERENCE.md`
- MIPS: `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`
- GPU: `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` (Mali section)
- General: `docs/PROJECT_OVERVIEW.md` (Phase V planning)

### **For Task Management:**
- Active: `docs/tasks/009-phase5-driver-research.md`
- History: `docs/tasks/completed/` (Tasks 001-008)
- Overview: `docs/tasks/README.md`

### **For Development Setup:**
- Environment: `flake.nix` + `.envrc`
- Tools: `tools/` directory
- Build Results: Root directory (DTB, U-Boot binaries)

## Agent Responsibilities

**Before starting any work:**
- Read current project status from `docs/PROJECT_OVERVIEW.md`
- Check for existing in_progress tasks via TodoRead tool
- Check `docs/tasks/` directory for task status
- Verify development environment ready (Nix devShell)
- Understand current phase objectives and dependencies

**When user provides external resources:**
- Update relevant task documentation immediately
- Add to appropriate hardware/component status documents
- Create dedicated reference documents for complex resources
- Ensure resources are accessible from multiple documentation entry points
- Example workflow: WiFi driver links → update task + hardware status + create reference doc

**During task execution:**
- Follow no-shortcuts policy strictly
- Update task status via TodoWrite tool and file updates
- Validate each step before proceeding
- Maintain hardware safety protocols
- Document research findings as they're discovered
- Cross-reference new information with existing documentation

**When completing phases/milestones:**
- Update all primary documentation (README.md, PROJECT_OVERVIEW.md, instructions)
- Verify documentation consistency across all files
- Update AGENTS.md with new processes or resources discovered
- Create/update hardware status and testing documentation
- Prepare next phase prerequisites and planning

**After task completion:**
- Verify all success criteria met via defined validation procedures
- Update project documentation with results and cross-references
- Move completed tasks to `docs/tasks/completed/` directory
- Prepare next task dependencies and identify blockers
- Commit all changes with clear messages referencing task numbers

**Research and Analysis Protocol:**
- Maximize software analysis before requiring hardware access
- Extract comprehensive information from factory firmware components
- Research external resources and community implementations
- Document integration patterns and requirements thoroughly
- Create roadmaps that minimize hardware testing iterations

This project requires precision, patience, and methodical progress. Quality and completeness are more important than speed.

## Current Development Approach

### **Phase V Strategy: Research-First Development**
The project has evolved to maximize software analysis before hardware access requirements:

1. **Factory Firmware Mining**: Extract all possible information from existing Android firmware
2. **Community Resource Integration**: Identify and document external driver implementations
3. **Pattern Analysis**: Understand driver loading, configuration, and hardware interface patterns
4. **Integration Planning**: Create detailed roadmaps for efficient hardware testing phases
5. **Documentation Completeness**: Ensure all analysis is thoroughly documented for future reference

### **Multi-Document Maintenance Philosophy**
Key information is intentionally duplicated across multiple documents for different access patterns:
- **README.md**: High-level project status and quick orientation
- **PROJECT_OVERVIEW.md**: Technical details and phase completion status  
- **Hardware Status Matrix**: Component-specific implementation and driver information
- **Task Documentation**: Detailed implementation steps and external resources
- **Reference Documents**: Deep-dive topics like WiFi drivers or testing methodology

This redundancy ensures information is accessible regardless of entry point while maintaining consistency through systematic update protocols.

### **Evidence-Based Development**
All decisions and analysis are supported by:
- Specific file references and line numbers
- Concrete examples from factory firmware analysis
- Measurable success criteria and validation procedures
- Complete audit trail of discoveries and decisions
- Cross-references between related documentation and analysis

This approach ensures reproducibility and enables efficient knowledge transfer for hardware testing phases.