#!/usr/bin/env python3
"""
MIPS Firmware Disassembly Tool
Comprehensive analysis of MIPS instruction patterns and control flow in display.bin
"""

import struct
import sys
from pathlib import Path

class MIPSDisassembler:
    def __init__(self):
        # MIPS instruction formats and opcodes
        self.opcodes = {
            0x00: 'SPECIAL',
            0x01: 'REGIMM',
            0x02: 'J',
            0x03: 'JAL',
            0x04: 'BEQ',
            0x05: 'BNE',
            0x06: 'BLEZ',
            0x07: 'BGTZ',
            0x08: 'ADDI',
            0x09: 'ADDIU',
            0x0a: 'SLTI',
            0x0b: 'SLTIU',
            0x0c: 'ANDI',
            0x0d: 'ORI',
            0x0e: 'XORI',
            0x0f: 'LUI',
            0x20: 'LB',
            0x21: 'LH',
            0x22: 'LWL',
            0x23: 'LW',
            0x24: 'LBU',
            0x25: 'LHU',
            0x26: 'LWR',
            0x28: 'SB',
            0x29: 'SH',
            0x2a: 'SWL',
            0x2b: 'SW',
            0x2e: 'SWR',
            0x30: 'LL',
            0x38: 'SC',
            0x3c: 'LUI',
        }
        
        self.special_functions = {
            0x00: 'SLL',
            0x02: 'SRL',
            0x03: 'SRA',
            0x04: 'SLLV',
            0x06: 'SRLV',
            0x07: 'SRAV',
            0x08: 'JR',
            0x09: 'JALR',
            0x0c: 'SYSCALL',
            0x0d: 'BREAK',
            0x10: 'MFHI',
            0x11: 'MTHI',
            0x12: 'MFLO',
            0x13: 'MTLO',
            0x18: 'MULT',
            0x19: 'MULTU',
            0x1a: 'DIV',
            0x1b: 'DIVU',
            0x20: 'ADD',
            0x21: 'ADDU',
            0x22: 'SUB',
            0x23: 'SUBU',
            0x24: 'AND',
            0x25: 'OR',
            0x26: 'XOR',
            0x27: 'NOR',
            0x2a: 'SLT',
            0x2b: 'SLTU',
        }
        
        # MIPS register names
        self.registers = [
            '$zero', '$at', '$v0', '$v1', '$a0', '$a1', '$a2', '$a3',
            '$t0', '$t1', '$t2', '$t3', '$t4', '$t5', '$t6', '$t7',
            '$s0', '$s1', '$s2', '$s3', '$s4', '$s5', '$s6', '$s7',
            '$t8', '$t9', '$k0', '$k1', '$gp', '$sp', '$fp', '$ra'
        ]
        
    def disassemble_instruction(self, word, address):
        """Disassemble a single MIPS instruction"""
        opcode = (word >> 26) & 0x3f
        rs = (word >> 21) & 0x1f
        rt = (word >> 16) & 0x1f
        rd = (word >> 11) & 0x1f
        shamt = (word >> 6) & 0x1f
        funct = word & 0x3f
        immediate = word & 0xffff
        target = word & 0x3ffffff
        
        if opcode == 0x00:  # SPECIAL
            if funct in self.special_functions:
                func_name = self.special_functions[funct]
                if func_name in ['SLL', 'SRL', 'SRA']:
                    return f"{func_name} {self.registers[rd]}, {self.registers[rt]}, {shamt}"
                elif func_name in ['ADD', 'ADDU', 'SUB', 'SUBU', 'AND', 'OR', 'XOR', 'NOR', 'SLT', 'SLTU']:
                    return f"{func_name} {self.registers[rd]}, {self.registers[rs]}, {self.registers[rt]}"
                elif func_name in ['JR', 'JALR']:
                    return f"{func_name} {self.registers[rs]}"
                elif func_name in ['MFHI', 'MFLO']:
                    return f"{func_name} {self.registers[rd]}"
                elif func_name in ['MTHI', 'MTLO']:
                    return f"{func_name} {self.registers[rs]}"
                else:
                    return f"{func_name}"
            else:
                return f"UNKNOWN_SPECIAL(0x{funct:02x})"
                
        elif opcode in self.opcodes:
            op_name = self.opcodes[opcode]
            if op_name in ['ADDI', 'ADDIU', 'SLTI', 'SLTIU', 'ANDI', 'ORI', 'XORI']:
                return f"{op_name} {self.registers[rt]}, {self.registers[rs]}, 0x{immediate:x}"
            elif op_name == 'LUI':
                return f"{op_name} {self.registers[rt]}, 0x{immediate:x}"
            elif op_name in ['LW', 'LH', 'LB', 'LBU', 'LHU', 'SW', 'SH', 'SB']:
                offset = immediate if immediate < 32768 else immediate - 65536
                return f"{op_name} {self.registers[rt]}, {offset}({self.registers[rs]})"
            elif op_name in ['BEQ', 'BNE']:
                offset = immediate if immediate < 32768 else immediate - 65536
                target_addr = address + 4 + (offset << 2)
                return f"{op_name} {self.registers[rs]}, {self.registers[rt]}, 0x{target_addr:08x}"
            elif op_name in ['J', 'JAL']:
                jump_target = (address & 0xf0000000) | (target << 2)
                return f"{op_name} 0x{jump_target:08x}"
            else:
                return f"{op_name} (0x{word:08x})"
        else:
            return f"UNKNOWN(0x{word:08x})"
            
    def analyze_firmware_entry_point(self, firmware_data):
        """Analyze the firmware entry point and initial instructions"""
        print("=== MIPS Firmware Entry Point Analysis ===")
        
        # MIPS typically starts at offset 0 or has a specific entry point
        entry_points = [0x0, 0x1000, 0x2000]  # Common entry points
        
        for entry_offset in entry_points:
            if entry_offset >= len(firmware_data):
                continue
                
            print(f"\nAnalyzing potential entry point at offset 0x{entry_offset:x}:")
            
            # Disassemble first 16 instructions
            for i in range(16):
                offset = entry_offset + (i * 4)
                if offset + 4 > len(firmware_data):
                    break
                    
                word = struct.unpack('>I', firmware_data[offset:offset+4])[0]
                address = 0xbfc00000 + offset  # MIPS reset vector base
                
                instruction = self.disassemble_instruction(word, address)
                print(f"  0x{address:08x}: 0x{word:08x}  {instruction}")
                
    def find_hardware_access_patterns(self, firmware_data):
        """Find patterns indicating hardware register access"""
        print("\n=== Hardware Access Pattern Analysis ===")
        
        # Look for LUI/ORI pairs that load hardware addresses
        hardware_addresses = []
        
        for i in range(0, len(firmware_data) - 8, 4):
            # Check for LUI followed by ORI pattern
            word1 = struct.unpack('>I', firmware_data[i:i+4])[0]
            word2 = struct.unpack('>I', firmware_data[i+4:i+8])[0]
            
            opcode1 = (word1 >> 26) & 0x3f
            opcode2 = (word2 >> 26) & 0x3f
            
            if opcode1 == 0x0f and opcode2 == 0x0d:  # LUI followed by ORI
                rt1 = (word1 >> 16) & 0x1f
                rt2 = (word2 >> 16) & 0x1f
                
                if rt1 == rt2:  # Same register
                    upper = word1 & 0xffff
                    lower = word2 & 0xffff
                    full_address = (upper << 16) | lower
                    
                    # Check if this looks like a hardware address
                    if 0x30000000 <= full_address <= 0x50000000:
                        hardware_addresses.append({
                            'offset': i,
                            'address': full_address,
                            'register': self.registers[rt1]
                        })
                        
        print(f"Found {len(hardware_addresses)} potential hardware register accesses:")
        for hw in hardware_addresses[:10]:  # Limit output
            print(f"  Offset 0x{hw['offset']:x}: Loading 0x{hw['address']:08x} into {hw['register']}")
            
    def analyze_communication_protocols(self, firmware_data):
        """Analyze communication protocol patterns"""
        print("\n=== Communication Protocol Analysis ===")
        
        # Look for specific register access patterns that indicate communication
        comm_patterns = []
        
        # Known communication register base: 0x3061000
        comm_base = 0x3061000
        
        for i in range(0, len(firmware_data) - 4, 4):
            word = struct.unpack('>I', firmware_data[i:i+4])[0]
            
            # Look for immediate values that might be register offsets
            immediate = word & 0xffff
            
            # Check for addresses near communication base
            if (comm_base - 0x1000) <= immediate <= (comm_base + 0x1000):
                opcode = (word >> 26) & 0x3f
                if opcode in [0x23, 0x2b]:  # LW or SW
                    comm_patterns.append({
                        'offset': i,
                        'instruction': 'LW' if opcode == 0x23 else 'SW',
                        'register_offset': immediate,
                        'word': f"0x{word:08x}"
                    })
                    
        print(f"Found {len(comm_patterns)} potential communication register accesses:")
        for pattern in comm_patterns[:10]:
            print(f"  Offset 0x{pattern['offset']:x}: {pattern['instruction']} with offset 0x{pattern['register_offset']:x}")
            
    def analyze_timing_critical_sections(self, firmware_data):
        """Look for timing-critical code sections"""
        print("\n=== Timing Critical Section Analysis ===")
        
        # Look for delay loops and timing-sensitive operations
        timing_patterns = []
        
        for i in range(0, len(firmware_data) - 12, 4):
            # Look for loop patterns (BNE back to earlier address)
            word = struct.unpack('>I', firmware_data[i:i+4])[0]
            opcode = (word >> 26) & 0x3f
            
            if opcode == 0x05:  # BNE
                immediate = word & 0xffff
                # Sign extend the immediate
                offset = immediate if immediate < 32768 else immediate - 65536
                
                # If it's a backward branch (negative offset), it might be a loop
                if offset < 0:
                    timing_patterns.append({
                        'offset': i,
                        'branch_offset': offset,
                        'instruction': f"BNE (loop offset {offset})"
                    })
                    
        print(f"Found {len(timing_patterns)} potential timing loops:")
        for pattern in timing_patterns[:5]:
            print(f"  Offset 0x{pattern['offset']:x}: {pattern['instruction']}")

def main():
    firmware_path = Path("firmware/display.bin")
    if not firmware_path.exists():
        print(f"ERROR: {firmware_path} not found")
        return
        
    with open(firmware_path, 'rb') as f:
        firmware_data = f.read()
        
    print("MIPS Firmware Disassembly Analysis")
    print("=" * 60)
    print(f"Firmware size: {len(firmware_data)} bytes")
    
    disassembler = MIPSDisassembler()
    disassembler.analyze_firmware_entry_point(firmware_data)
    disassembler.find_hardware_access_patterns(firmware_data)
    disassembler.analyze_communication_protocols(firmware_data)
    disassembler.analyze_timing_critical_sections(firmware_data)
    
    print("\n" + "=" * 60)
    print("MIPS DISASSEMBLY ANALYSIS COMPLETE")

if __name__ == "__main__":
    main()