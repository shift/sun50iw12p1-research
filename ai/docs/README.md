# AI Agent Documentation

This directory contains documentation for AI agents working on the HY300 Android projector mainline Linux porting project.

## Project Overview

The goal is to perform a complete hardware bring-up and port of a modern, mainline-based Linux distribution to the HY300 consumer projector with Allwinner H713 SoC.

## AI Agent Guidelines

### Task Management

- All tasks are tracked in `docs/tasks/` with incrementing numbers
- Tasks should be atomic and well-defined
- Completed tasks are moved to `docs/tasks/completed/`
- Context files in `ai/contexts/` provide background information for task completion

### Development Environment

- Use the Nix flake development environment
- All tools for cross-compilation, firmware analysis, and hardware debugging are provided
- Check `IN_NIX_SHELL` environment variable before running commands

### Key Phases

1. **Phase I: Low-Level Device Access & Reconnaissance**
   - Establish serial console access
   - Achieve FEL mode access
   - Backup firmware
   - Analyze firmware components

2. **Phase II: U-Boot Porting & Bring-up**
   - Extract DRAM parameters from boot0.bin
   - Create U-Boot configuration for HY300
   - Achieve stable U-Boot prompt

3. **Phase III: Mainline Kernel Boot**
   - Port device tree
   - Configure kernel for sun50i platform
   - Boot to command line

4. **Phase IV: Driver Enablement**
   - Wireless connectivity (AIC8800)
   - Optical engine drivers

### Safety Guidelines

- ALWAYS backup firmware before modifications
- Never commit ROM images or binaries to git
- Verify cross-compilation environment before building
- Test changes incrementally

### Communication

- Update task status after each significant change
- Document discoveries in appropriate context files
- Reference specific file locations with `file:line` format