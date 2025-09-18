# Git Standards for Delegated Agents

## Git Commit Requirements
All delegated agents must follow these git standards for consistency and traceability.

### Commit Message Format
```
[Task ###] Brief description of change

Detailed explanation of what was changed and why.
Include references to specific files modified.

- Specific change 1
- Specific change 2
- Cross-reference updates made
```

### Required Git Operations
1. **Always commit changes**: Never leave uncommitted modifications
2. **Reference task numbers**: Include `[Task ###]` in commit messages
3. **Atomic commits**: One logical change per commit
4. **Descriptive messages**: Explain both what and why
5. **File validation**: Verify all expected files are committed

### File Management Rules
**Critical exclusions (already in .gitignore):**
- ROM files: `*.img`, `boot0.bin`, factory firmware binaries
- Temporary files: Build artifacts, editor backups
- Test outputs: Unless part of official testing framework

**Always commit:**
- Documentation updates (.md files)
- Source code changes (.c, .h, .dts files)
- Configuration updates (flake.nix, Makefiles)
- Task file updates
- Analysis results and reference documents

### Pre-Commit Validation
Before each commit, verify:
```bash
# Check git status
git status

# Verify expected files are staged
git diff --cached

# Check for any excluded files accidentally staged
git ls-files | grep -E '\.(img|bin)$' || echo "No binaries staged (good)"
```

### Cross-Reference Commit Protocol
When updating documentation that affects multiple files:
1. **Single commit approach**: Include all related changes in one commit
2. **Reference all files**: List all modified files in commit message
3. **Explain relationships**: Document why files were updated together
4. **Validate consistency**: Ensure information is consistent across files

### Example Commit Messages

**Good commit message:**
```
[Task 009] Add AIC8800 WiFi driver reference documentation

Created dedicated reference document for AIC8800 WiFi drivers with
3 community implementations. Updated hardware status matrix and
task documentation with cross-references.

- docs/AIC8800_WIFI_DRIVER_REFERENCE.md: New reference document
- docs/HY300_HARDWARE_ENABLEMENT_STATUS.md: Added WiFi driver links
- docs/tasks/009-phase5-driver-research.md: Updated with driver resources
```

**Bad commit message:**
```
Update docs

Fixed some stuff in the documentation.
```

### Branch Management
- **Main branch**: All work committed to main branch
- **Clean history**: Use meaningful commit messages for project history
- **No force pushes**: Maintain complete commit history
- **Linear history**: Avoid unnecessary merge commits

### File Path Standards
Always use full paths from project root in commit messages:
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` (not just "hardware status")
- `firmware/FIRMWARE_COMPONENTS_ANALYSIS.md` (not just "firmware analysis")
- `ai/contexts/delegation-standards.md` (not just "standards file")

### Git Integration with Task Management
- **Task start**: Commit any setup or preparation work
- **Task progress**: Commit incremental progress with task references
- **Task completion**: Final commit includes all deliverables and documentation
- **Cross-validation**: Use `ai/tools/task-manager` after commits to update task status

### Recovery and Validation
```bash
# View recent commits for validation
git log --oneline -5

# Check for uncommitted changes
git status

# Verify no binaries committed
git log --name-only | grep -E '\.(img|bin)$' || echo "No binaries in history (good)"
```