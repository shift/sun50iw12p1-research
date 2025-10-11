# Task 027 Delegation Prompt Templates

## Quick Reference for Atomic Task Delegations

Each prompt below is ready-to-use for delegating to specialized agents via the Task tool. Copy the relevant prompt and use with Task tool.

---

## 027a: boot0 BROM Analysis Delegation Prompt

```
You are a specialized binary analysis agent working on the HY300 Linux porting project.

**PROJECT:** HY300 Android Projector - Port mainline Linux to Allwinner H713 SoC
**PHASE:** Phase II - U-Boot Porting
**CURRENT ISSUE:** H713 FEL mode detected but SPL upload fails with timeout
**ROOT CAUSE:** Using H616 memory map on H713 (incompatible SRAM layout)

**YOUR ATOMIC TASK:** Extract H713 BROM SRAM memory layout from boot0.bin

**CRITICAL FILE EDITING RULE - PREVENT SESSION CRASHES:**
- This task uses binary analysis only, no C file editing
- If you need to create code examples, use markdown code blocks
- Never attempt to use Edit tool on any .c files (will crash session)

**INPUT FILES:**
- /home/shift/code/android_projector/boot0.bin (first-stage bootloader)
- /home/shift/code/android_projector/build/sunxi-tools/soc_info.c (H616 reference)

**OBJECTIVE:**
Extract SRAM addresses from boot0.bin through static binary analysis:
- SRAM A1 base address and size
- ARM reset vectors and entry points
- Memory region definitions
- FEL handler locations (if present)

**ANALYSIS METHODS:**
1. Binary structure analysis: `binwalk boot0.bin`
2. ARM reset vector search: `grep -abo $'\xEA\x00\x00\x00' boot0.bin`
3. String extraction: `strings -t x boot0.bin | grep -iE "sram|0x0002"`
4. Memory pattern search: `hexdump -C boot0.bin | grep -E "00 00 02 00"`
5. Disassembly (if needed): `aarch64-unknown-linux-gnu-objdump -D -b binary -m aarch64 boot0.bin`

**H616 REFERENCE (for comparison):**
```c
.spl_addr = 0x20000,      // SRAM A1 + offset
.scratch_addr = 0x108000, // Scratchpad region
.thunk_addr = 0x118000,   // Code execution area
.thunk_size = 0x8000,     // 32KB
```

**EVIDENCE STANDARDS:**
Every finding MUST include:
- Hex offset in boot0.bin (e.g., "offset 0x1234")
- Hex pattern/bytes found
- Analysis method used
- Cross-validation from multiple methods when possible
- Confidence level: High/Medium/Low

**DELIVERABLE:**
Create file: `/home/shift/code/android_projector/docs/H713_BROM_MEMORY_MAP.md`

Required sections:
1. **Analysis Summary** - Key findings
2. **SRAM Memory Regions Table**:
   | Address | Purpose | Size | Evidence | Confidence |
   |---------|---------|------|----------|------------|
3. **Evidence Details** - Hex dumps and command outputs
4. **Cross-Validation** - Multiple method confirmation
5. **Uncertainties** - What needs further research

**SUCCESS CRITERIA:**
- [ ] SRAM A1 base address identified with evidence
- [ ] At least 3 memory addresses found
- [ ] All findings include hex offset + pattern
- [ ] Multiple analysis methods used
- [ ] Document created following template

**TIME LIMIT:** 1-2 hours
**SAFETY:** Read-only analysis, no hardware required
**TOOLS:** binwalk, strings, hexdump, grep, objdump (standard Linux)

Complete this atomic task independently. Provide complete deliverable document.
```

---

## 027c: Factory Firmware Mining Delegation Prompt

```
You are a specialized firmware analysis agent working on the HY300 Linux porting project.

**PROJECT:** HY300 Android Projector - Port mainline Linux to Allwinner H713 SoC
**PHASE:** Phase II - U-Boot Porting  
**CURRENT ISSUE:** Need to validate H713 SRAM addresses from boot0 analysis

**YOUR ATOMIC TASK:** Extract FEL-related addresses from factory Android firmware

**CRITICAL FILE EDITING RULE - PREVENT SESSION CRASHES:**
- This task uses text/binary search only, no C file editing
- Never attempt to use Edit tool on any .c files (will crash session)
- Use bash commands for all file operations

**INPUT DIRECTORY:**
- /home/shift/code/android_projector/firmware/extracted_components/
  - Device tree files (*.dts, *.dtsi)
  - boot.img (Android kernel)
  - System/vendor partitions

**OBJECTIVE:**
Find documented SRAM/FEL addresses in factory firmware to:
- Validate boot0.bin analysis findings
- Identify memory regions in device trees
- Find kernel FEL driver references
- Cross-reference with H713 BROM analysis

**SEARCH METHODS:**
1. Device tree SRAM search:
   ```bash
   cd firmware/extracted_components/
   find . -name "*.dts*" | xargs grep -i "sram"
   find . -name "*.dts*" | xargs grep -E "0x0002|0x0001"
   ```

2. Memory regions:
   ```bash
   grep -r "memory@\|reserved-memory" . --include="*.dts"
   ```

3. Kernel FEL drivers:
   ```bash
   find . -name "*.c" | xargs grep -l "FEL\|AWUSBFEX"
   strings boot.img | grep -iE "fel|sram|0x0002"
   ```

**CROSS-VALIDATION:**
Compare findings with Task 027a boot0 analysis results:
- Do addresses match?
- Are sizes consistent?
- Any conflicts or additional regions?

**EVIDENCE STANDARDS:**
Every finding MUST include:
- Source file path and line number
- Context (DTS node, driver code, log entry)
- Match with boot0 analysis? (yes/no/partial)
- Confidence level: High/Medium/Low

**DELIVERABLE:**
Create file: `/home/shift/code/android_projector/docs/FACTORY_FEL_ADDRESSES.md`

Required sections:
1. **Summary** - Key findings
2. **Address Comparison Table**:
   | Address | Source | Purpose | Match boot0? | Confidence |
   |---------|--------|---------|--------------|------------|
3. **Device Tree Findings** - SRAM regions from DTS
4. **Kernel References** - FEL driver data
5. **Cross-Validation Notes** - Agreements/conflicts with boot0
6. **Recommendations** - Addresses to prioritize for testing

**SUCCESS CRITERIA:**
- [ ] All device trees searched for SRAM references
- [ ] Kernel analyzed for FEL-related addresses
- [ ] Findings cross-referenced with boot0 analysis
- [ ] All addresses have source file:line references
- [ ] Document created following template

**TIME LIMIT:** 1-2 hours
**SAFETY:** Read-only analysis, no hardware required
**TOOLS:** grep, find, strings (standard Linux)

Complete this atomic task independently. Provide complete deliverable document.
```

---

## Usage Instructions

1. **Select appropriate prompt** for the task you're delegating
2. **Copy entire prompt** including all context sections
3. **Use Task tool** to delegate to specialized agent:
   ```
   Task(
     description="Boot0 BROM memory analysis",
     prompt="[paste full prompt here]",
     subagent_type="general"
   )
   ```
4. **Wait for completion** - Agent will work autonomously
5. **Verify deliverable** - Check that expected output file was created
6. **Commit results** - If agent didn't commit, do it manually
7. **Update task status** - Use `ai/tools/task-manager complete 027a`

## Prompt Customization Notes

- All prompts are self-contained (no external file dependencies)
- Include complete project context
- Specify exact file paths
- Provide command examples
- Define evidence standards
- Include success criteria
- Set time limits

## Critical Reminders for All Delegations

**SESSION-CRITICAL RULES:**
- ❌ NEVER use Edit tool on .c files (causes session crashes)
- ✅ ALWAYS use patches for C file modifications
- ✅ All analysis tasks use bash commands only
- ✅ Commit deliverables with [Task 027X] prefix

**Evidence Requirements:**
- Every finding needs source reference
- Cross-validate when possible
- Document negative results
- Include confidence levels

**Time Management:**
- Analysis tasks: 1-2 hours each
- Testing tasks: 2-3 hours
- Don't exceed time limits
- Document blockers if stuck

---

**Document Version:** 1.0
**Created:** 2025-10-11
**Purpose:** Quick delegation reference for Task 027 atomic subtasks
