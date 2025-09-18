# Development Environment Standards for Delegated Agents

## Nix Development Environment
This project uses NixOS with a flake.nix development shell for consistent tooling.

### Environment Verification
**Always verify Nix environment before starting work:**
```bash
# Check if in devShell
echo $IN_NIX_SHELL

# If not in devShell, use:
nix develop -c -- <command>
```

### Required Tools Available in devShell
- **Cross-compilation toolchain**: `aarch64-unknown-linux-gnu-*` (GCC, binutils, etc.)
- **sunxi-tools**: FEL mode operations and Allwinner-specific utilities
- **Firmware analysis tools**: binwalk, strings, hexdump, file
- **Device tree compiler**: dtc for compiling device tree sources
- **Build tools**: make, cmake, pkg-config
- **Development utilities**: git, python3, ripgrep (rg)

### Project Structure
```
/home/shift/code/android_projector/
├── flake.nix              # Development environment configuration
├── .envrc                 # direnv auto-activation
├── tools/                 # Custom analysis tools
├── firmware/              # ROM analysis and extracted components
├── docs/                  # Documentation and task management
├── ai/                    # AI agent tools and contexts
└── [device tree files]    # Build outputs
```

### Environment Setup Commands
```bash
# Enter development shell (if not auto-activated)
nix develop

# Build project components
nix build

# Run project checks/tests
nix check

# Run commands with environment (if outside devShell)
nix develop -c -- <command>
```

### Custom Tools Usage
Available custom Python tools in `tools/` directory:
- `tools/analyze_boot0.py` - DRAM parameter extraction from boot0.bin
- `tools/compare_dram_params.py` - Parameter validation against U-Boot defaults
- `tools/hex_viewer.py` - Interactive hex viewer for firmware analysis

All tools are designed to work within the Nix development environment.

### File Management Rules
- **ROM files excluded**: `*.img`, `boot0.bin` files are in .gitignore
- **Commit standards**: Always commit documentation and code changes
- **Binary exclusion**: Never commit binaries unless part of testing framework
- **Task references**: Include task numbers in commit messages

### Build and Testing
- **Cross-compilation**: All builds target aarch64 architecture
- **Device tree compilation**: Use `dtc` to compile .dts to .dtb
- **Testing**: Run available tests before marking tasks complete
- **Validation**: Verify builds succeed without errors/warnings

### Environment Troubleshooting
Common issues and solutions:
1. **Missing tools**: Ensure in Nix devShell (`echo $IN_NIX_SHELL`)
2. **Cross-compilation failures**: Verify toolchain with `aarch64-unknown-linux-gnu-gcc --version`
3. **Python tool errors**: Ensure running within devShell environment
4. **Build failures**: Use `nix develop -c -- <build-command>` format

### Integration with Task Management
- Use `ai/tools/task-manager` for all task operations
- Update task status after environment setup validation
- Document environment-specific issues in task files
- Reference environment requirements in task prerequisites