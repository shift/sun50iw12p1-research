# HY300 Project Delegation Standards

## Project Context
This is a hardware porting project to run mainline Linux on the HY300 Android projector with Allwinner H713 SoC. We follow a rigorous, phase-based approach with no shortcuts or mock implementations.

**Current Phase**: Phase V - Driver Integration Research
**Project Goal**: Complete Linux mainline support for HY300 projector hardware

## Core Standards for Delegated Agents

### No Shortcuts Policy (MANDATORY)
- **NEVER mock, stub, or simulate code** that should be functional
- **NEVER skip testing** - always run available tests
- **NEVER disable tests** - fix issues instead
- **NEVER cut corners** - break complex tasks into smaller ones instead
- **ALWAYS implement complete solutions** - partial implementations are unacceptable

### Code Quality Standards
- Follow existing codebase conventions
- Use established libraries and utilities
- Implement complete, working solutions
- Include comprehensive error handling
- Document complex technical decisions
- **For C files**: Use patch-based editing approach due to file length - create patches instead of direct edits

### Documentation Requirements
- Update relevant documentation with changes
- Maintain technical accuracy in all documentation
- Include implementation details for future reference
- Document known issues and workarounds
- **C code changes**: When modifying C files, document changes in patch format for clarity and reviewability
- Cross-reference related documentation when making updates

### Testing Requirements
- Run all available tests before task completion
- Fix test failures rather than disabling tests
- Create tests for new functionality when applicable
- Verify cross-compilation builds successfully

### Task Completion Standards
- All acceptance criteria met and verified
- Documentation updated and accurate
- Tests passing (if applicable)
- Code compiles without errors/warnings
- No technical debt introduced
- Git commits made with clear messages referencing task numbers

## Development Environment
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

## Task Management Integration
- Update task status using `ai/tools/task-manager start/complete/block` commands
- Update task progress directly in task files
- Reference task numbers in git commit messages
- Document progress within the task file itself

## Quality Validation Procedures
Before marking any task complete:
1. Verify all success criteria met
2. Run tests and verification procedures
3. Update documentation with results
4. Commit all changes with clear messages
5. Cross-check related documentation for consistency