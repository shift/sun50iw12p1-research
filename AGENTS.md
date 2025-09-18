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

## Task File Management

### Creating New Tasks
When tracking work, create new task files in `docs/tasks/` with format:
- **Filename**: `###-descriptive-name.md` (e.g., `022-device-tree-integration.md`)
- **Status field**: Use `pending`, `in_progress`, `completed`, or `blocked`
- **Move completed tasks**: To `docs/tasks/completed/` directory when finished

### Task Status Updates
- Edit task files directly to update status
- Only one task should have `in_progress` status at a time
- Document progress within the task file itself
- Reference task numbers in git commit messages

### Task Discovery and Management
Use the comprehensive `ai/tools/task-manager` tool for all task operations:

#### Core Task Management Commands
```bash
# Check for any in-progress tasks (CRITICAL - always run first)
ai/tools/task-manager find-inprogress

# Get next priority task recommendation
ai/tools/task-manager next

# List all tasks with status
ai/tools/task-manager list

# Show only active (in_progress + pending + blocked) tasks
ai/tools/task-manager active

# Create new task
ai/tools/task-manager create "descriptive-task-name"

# Update task status
ai/tools/task-manager start 015        # Mark as in_progress
ai/tools/task-manager complete 015     # Mark as completed and move to completed/
ai/tools/task-manager block 015 "Hardware access required"

# View specific task details
ai/tools/task-manager status 015

# Validate all task files
ai/tools/task-manager validate
```

#### Task Management Workflow
1. **Always start with**: `ai/tools/task-manager find-inprogress`
2. **If in-progress task found**: Continue working on it
3. **If no in-progress tasks**: Use `ai/tools/task-manager next` for recommendations
4. **When starting work**: `ai/tools/task-manager start <task_id>`
5. **When completing**: `ai/tools/task-manager complete <task_id>`

### Additional AI Tools Available
- **`ai/tools/context-manager`**: Context and session management
- **`ai/tools/git-manager`**: Git operations following project standards  
- **`ai/tools/test-suite`**: Testing framework for AI tools

### Tool Usage Policy
- **Never use bash find/grep** for task discovery - use the task-manager tool
- **Never use TodoRead/TodoWrite tools** - task files are the single source of truth
- **Always use task-manager** for task status operations
- **Create additional tools** as needed for specific workflows

### Agent Delegation Protocol
- **ATOMIC TASK DELEGATION**: Each delegation must be a single, atomic task to manage context limits effectively
- **ONE TASK PER DELEGATION**: Never combine multiple tasks in a single agent delegation - break complex work into separate atomic delegations
- **COMPLETE INITIAL CONTEXT**: All rules, standards, and required context files must be provided in the delegated agent's initial prompt
- **SELF-CONTAINED DELEGATIONS**: Each delegated agent must receive everything needed to complete their atomic task independently - they have NO ACCESS to AGENTS.md, project files, or any context not explicitly provided in the prompt
- **General agents act as coordinators**: The general agent should primarily delegate tasks to specialized agents rather than performing implementation work directly
- **Use Task tool for atomic operations**: When single-step tasks or specialized knowledge is required, use the Task tool to launch appropriate specialized agents
- **Examples of atomic delegation scenarios**:
  - Search for specific driver references → single research delegation with project context and search criteria
  - Analyze one firmware component → single analysis delegation with component file and analysis framework
  - Implement one specific function → single development delegation with coding standards and requirements
  - Update one documentation file → single coordination task with update requirements and cross-reference rules
- **General agent focuses on**: task coordination, documentation updates, high-level planning, and orchestrating work between multiple atomic specialized agent delegations
- **Reserve direct implementation for**: simple file edits, single-step operations, immediate responses to user questions
- **Context Management**: Each atomic delegation prevents context overflow and ensures focused, high-quality results

#### Required Delegation Context
When delegating atomic tasks, always include in the initial prompt:
1. **Project Overview**: Brief description of HY300 Linux porting project and current phase
2. **Specific Task**: Clear, atomic task definition with success criteria
3. **Technical Standards**: Relevant coding standards, documentation requirements, and quality guidelines
4. **File Context**: Any files the agent needs to read or modify, with specific paths
5. **Safety Protocols**: Hardware safety rules, git commit requirements, testing procedures
6. **Validation Requirements**: How to verify task completion and what deliverables are expected
7. **Cross-References**: Related documentation that must be updated or referenced
8. **Tool Usage**: Specific tools or commands required for the task
9. **Environment Context**: Nix development environment requirements if applicable
10. **Integration Points**: How this atomic task fits into the larger work stream

#### Critical File Editing Rules for Delegated Agents
**MANDATORY for all delegated agents:**
- **NEVER use Edit tool on .c files** - C files must use patch-based editing due to length limitations
- **Create patches instead** - Use bash commands to create patch files for C file modifications
- **Apply patches cleanly** - Verify patches apply without errors before proceeding
- **Test compilation** - Always verify changes compile successfully before completion
- **Commit modified files only** - Commit the updated .c files, never commit the patch files themselves
- **Document changes** - Include clear descriptions of what the modifications accomplish

#### Context Content Inclusion Strategy
**COPY RELEVANT CONTEXT DIRECTLY INTO PROMPT** - do not reference files the agent should read:
- **Extract and include** relevant sections from context files directly in the delegation prompt
- **Provide specific rules and standards** inline rather than referencing external files
- **Include file paths and examples** directly in the prompt text
- **Copy command examples** and validation procedures into the prompt
- **Never assume** the delegated agent can read any project files not explicitly provided

#### Example Delegation Prompt Template
```
You are a specialized agent working on the HY300 Linux porting project. 

**PROJECT CONTEXT:**
This is a hardware porting project to run mainline Linux on the HY300 Android projector with Allwinner H713 SoC. Current phase: Phase V Driver Integration Research.

**CODING STANDARDS:**
- NEVER mock, stub, or simulate code - implement complete solutions
- **CRITICAL: C FILES MUST USE PATCH-BASED EDITING** - Never use Edit tool on .c files, always create patches due to file length limitations
- Always run available tests before completion
- Follow existing codebase conventions

**MANDATORY C FILE EDITING RULES:**
- **NEVER use Edit tool on any .c files** - they are too large and will cause errors
- **ALWAYS create patches instead** - use bash commands like: diff -u original.c modified.c > changes.patch
- **Apply patches cleanly** - use: patch -p0 < changes.patch and verify no errors
- **Test compilation after changes** - verify your modifications compile successfully
- **Commit the modified files** - commit the updated .c files, NOT the patch files
- **Document what your changes do** - include clear descriptions in commit messages

**GIT REQUIREMENTS:**
- Commit all changes with format: [Task ###] Brief description
- Reference task numbers in commit messages
- Never commit binaries (*.img, *.bin files)
- Always include cross-reference updates in commits

**ENVIRONMENT:**
- Verify Nix environment with: echo $IN_NIX_SHELL
- Use: nix develop -c -- <command> if not in devShell
- Required tools: aarch64-unknown-linux-gnu-*, dtc, binwalk

**ATOMIC TASK:**
[Single, specific task description with clear success criteria]

**DELIVERABLES EXPECTED:**
- [Specific files to create/modify with full paths]
- [Documentation updates required with cross-references]
- [Git commits with proper task references]

**VALIDATION REQUIRED:**
[Specific steps to verify task completion]

Complete this atomic task following all standards above.
```

#### Post-Delegation Verification Protocol
After each atomic task delegation, the coordinating agent MUST:
1. **Verify task completion status**: Check that the delegated agent completed successfully without errors or context limits
2. **Validate deliverables**: Ensure all expected outputs (files, documentation, analysis) were produced
3. **Check git commits**: Verify that any file changes were properly committed with appropriate messages
4. **Update task status**: Use `ai/tools/task-manager` to update task progress if the delegated agent didn't
5. **Document results**: Synthesize agent results into project documentation if not completed
6. **Prepare next delegation**: Only proceed to next atomic task after verifying current delegation completed cleanly
7. **Handle incomplete delegations**: If agent hit context limits or errors, complete remaining work before next delegation

## Agent Workflow Rules
### 1. Task Continuity (CRITICAL)
**Always check for in-progress tasks first using the task-manager tool:**
```bash
# ALWAYS run this first when starting work
ai/tools/task-manager find-inprogress
```

**Workflow Priority Order:**
1. **Continue last worked task** - If `find-inprogress` returns a task, work on it
2. **Pick up pending tasks** - Use `ai/tools/task-manager next` for recommendations
3. **Create new task** - Use `ai/tools/task-manager create` if no pending tasks exist

**Never use TodoRead/TodoWrite tools** - task files are the single source of truth

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
- **C file editing**: Use patch-based approach for large C files to avoid edit tool limitations

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
1. **Use `ai/tools/task-manager find-inprogress`** to check for any in_progress tasks
2. **Continue in_progress task** if found
3. **Use `ai/tools/task-manager next`** to get next priority task if no in_progress work
4. **Review task dependencies** before starting
5. **Use `ai/tools/task-manager start <task_id>`** when beginning work

### When Creating New Tasks:
- **Break down complexity** - prefer atomic, testable tasks
- **Document prerequisites** - what must be completed first
- **Define success criteria** - clear, measurable outcomes
- **Estimate effort** - realistic time and resource requirements

### When Completing Tasks:
- **Validate all success criteria** met
- **Run tests and verification** procedures
- **Update documentation** with results
- **Use `ai/tools/task-manager complete <task_id>`** to mark completed and move to completed/
- **Use `ai/tools/task-manager next`** to identify next task priorities

## Technical Standards

### Code Quality
- Follow existing codebase conventions
- Use established libraries and utilities
- Implement complete, working solutions
- Include comprehensive error handling
- Document complex technical decisions
- **CRITICAL: C files MUST use patch-based editing** - Never use Edit tool on .c files due to length limitations

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
- **C code changes**: When modifying C files, document changes in patch format for clarity and reviewability

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
- Check for existing in-progress tasks using `ai/tools/task-manager find-inprogress`
- Use `ai/tools/task-manager next` to get task recommendations
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
- **Delegate atomic tasks to specialized agents** using the Task tool rather than implementing directly
- **Each delegation must be single, atomic task** to manage context limits effectively
- **Include complete context directly in delegation prompts**: Copy all necessary rules, standards, and requirements into the prompt text - delegated agents cannot access project files
- **Verify each delegation completes cleanly** before proceeding to next atomic task
- **Complete any unfinished work** from delegated agents (commits, task updates, documentation)
- Update task status using `ai/tools/task-manager start/complete/block` commands
- Update task progress directly in task files
- Validate each step before proceeding
- Maintain hardware safety protocols
- Document research findings as they're discovered
- Cross-reference new information with existing documentation
- **Act as coordinator**: orchestrate work between multiple atomic specialized agent delegations for complex multi-phase tasks

**When completing phases/milestones:**
- Update all primary documentation (README.md, PROJECT_OVERVIEW.md, instructions)
- Verify documentation consistency across all files
- Update AGENTS.md with new processes or resources discovered
- Create/update hardware status and testing documentation
- Prepare next phase prerequisites and planning

**After task completion:**
- Verify all success criteria met via defined validation procedures
- Update project documentation with results and cross-references
- Use `ai/tools/task-manager complete <task_id>` to mark complete and move to completed/
- Use `ai/tools/task-manager next` to identify next priorities and dependencies
- Commit all changes with clear messages referencing task numbers

**Research and Analysis Protocol:**
- **Delegate atomic research tasks to specialized agents** for focused analysis with context limit management
- Use Task tool to launch research agents for single, atomic tasks: individual firmware analysis, specific driver research, or targeted external resource discovery
- **ONE ATOMIC TASK PER DELEGATION**: Never combine multiple research areas in a single delegation
- **Copy relevant context directly into delegation prompts**: Extract and include needed information from context files directly in the prompt - delegated agents cannot read project files
- **Verify each research delegation completes successfully** before proceeding to next atomic delegation
- **Complete any unfinished research work** (documentation updates, file commits, task status) if delegated agent didn't finish cleanly
- **Coordinate multiple atomic research delegations** for comprehensive analysis across different specialized agents
- Maximize software analysis before requiring hardware access
- Extract comprehensive information from factory firmware components through focused atomic delegations
- Research external resources and community implementations via targeted single-focus delegations
- Document integration patterns and requirements thoroughly
- Create roadmaps that minimize hardware testing iterations
- **General agent synthesizes results** from multiple atomic specialized research agent delegations into project documentation

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