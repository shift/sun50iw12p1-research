# AI Agent Guidelines for HY300 Linux Porting Project

## Project Overview
This is a hardware porting project to run mainline Linux on the HY300 Android projector with Allwinner H713 SoC. We follow a rigorous, phase-based approach with no shortcuts or mock implementations.

## Documentation Structure

### Primary Documentation
- `README.md` - Project overview and current status
- `docs/instructions/README.md` - Comprehensive development guide
- `firmware/ROM_ANALYSIS.md` - Complete ROM analysis results
- `AGENTS.md` - This file (AI agent guidelines)

### Task Management System
- `docs/tasks/` - Active tasks directory
- `docs/tasks/completed/` - Completed tasks archive
- `docs/tasks/README.md` - Task management overview

### Analysis and Context
- `firmware/` - ROM analysis results and extracted components
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

## Current Project Status

### Phase I: Firmware Analysis ✅ COMPLETED
- ROM structure analyzed and documented
- Boot0 bootloader extracted with DRAM parameters
- Device trees identified and extracted
- Development environment established

### Phase II: U-Boot Porting 🔄 IN PROGRESS
**Current Focus:** DRAM parameter extraction from boot0.bin
**Critical Path:** This is the main blocker for bootloader compilation

### Known Tasks
1. **Task 001:** ✅ Setup ROM Analysis Workspace - COMPLETED
2. **Task 002:** ⏳ Extract DRAM parameters from boot0.bin - NEXT PRIORITY

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

## Agent Responsibilities

**Before starting any work:**
- Read current project status
- Check for existing in_progress tasks
- Verify development environment ready
- Understand current phase objectives

**During task execution:**
- Follow no-shortcuts policy strictly
- Update task status and documentation
- Validate each step before proceeding
- Maintain hardware safety protocols

**After task completion:**
- Verify all success criteria met
- Update project documentation
- Prepare next task dependencies
- Commit all changes with clear messages

This project requires precision, patience, and methodical progress. Quality and completeness are more important than speed.