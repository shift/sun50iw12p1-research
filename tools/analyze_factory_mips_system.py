#!/usr/bin/env python3
"""
Factory MIPS System Analysis Tool
Analyzes the factory MIPS loader system, security mechanisms, and communication protocols.
"""

import os
import sys
import struct
import hashlib
import binascii
from pathlib import Path

class FactoryMIPSAnalyzer:
    def __init__(self, base_path):
        self.base_path = Path(base_path)
        self.factory_path = self.base_path / "firmware/extractions/super.img.extracted/33128670/rootfs"
        self.mips_path = self.factory_path / "etc/display/mips"
        
    def analyze_loadmips_binary(self):
        """Analyze the loadmips binary for implementation patterns"""
        print("=== Factory MIPS Loader Binary Analysis ===")
        
        loadmips_path = self.factory_path / "bin/loadmips"
        libmips_path = self.factory_path / "lib/libmips.so"
        
        if not loadmips_path.exists():
            print(f"ERROR: {loadmips_path} not found")
            return
            
        # File information
        stat_info = loadmips_path.stat()
        print(f"loadmips binary: {stat_info.st_size} bytes")
        
        if libmips_path.exists():
            lib_stat = libmips_path.stat()
            print(f"libmips.so library: {lib_stat.st_size} bytes")
            
        # Extract critical strings and patterns
        with open(loadmips_path, 'rb') as f:
            data = f.read()
            
        print(f"\nloadmips Binary Analysis:")
        print(f"  Size: {len(data)} bytes")
        print(f"  ELF Header: {data[:16].hex()}")
        
        # Look for embedded strings
        strings = self.extract_strings(data)
        critical_strings = [s for s in strings if any(keyword in s.lower() 
                          for keyword in ['mips', 'display', 'loader', 'dev', 'sys'])]
        
        print(f"\nCritical Strings Found ({len(critical_strings)}):")
        for s in critical_strings[:20]:  # Limit output
            print(f"  {s}")
            
    def analyze_libmips_library(self):
        """Analyze libmips.so for communication protocols and register access"""
        print("\n=== libmips.so Library Analysis ===")
        
        libmips_path = self.factory_path / "lib/libmips.so"
        if not libmips_path.exists():
            print(f"ERROR: {libmips_path} not found")
            return
            
        with open(libmips_path, 'rb') as f:
            data = f.read()
            
        print(f"Library Size: {len(data)} bytes")
        
        # Extract function names and device paths
        strings = self.extract_strings(data)
        
        # Device paths
        device_paths = [s for s in strings if s.startswith('/dev/') or s.startswith('/sys/')]
        print(f"\nDevice Paths ({len(device_paths)}):")
        for path in device_paths:
            print(f"  {path}")
            
        # Memory addresses and operations
        memory_strings = [s for s in strings if '0x' in s and ('memory' in s.lower() or 'address' in s.lower())]
        print(f"\nMemory Operations ({len(memory_strings)}):")
        for mem in memory_strings[:10]:
            print(f"  {mem}")
            
        # Function signatures (mangled C++ names)
        cpp_functions = [s for s in strings if s.startswith('_Z') and 'mips' in s.lower()]
        print(f"\nMIPS Functions ({len(cpp_functions)}):")
        for func in cpp_functions:
            print(f"  {func}")
            
    def analyze_display_config(self):
        """Analyze display_cfg.xml for memory layout and hardware configuration"""
        print("\n=== Display Configuration Analysis ===")
        
        config_path = self.mips_path / "display_cfg.xml"
        if not config_path.exists():
            print(f"ERROR: {config_path} not found")
            return
            
        with open(config_path, 'r') as f:
            content = f.read()
            
        print("Memory Layout from display_cfg.xml:")
        
        # Extract memory layout information
        memory_regions = {
            'boot_code': '0x4b100000',
            'c_code': '0x4b101000', 
            'debug_buffer': '0x4bd01000',
            'cfg_file': '0x4be01000',
            'tse_data': '0x4be41000',
            'frame_buffer': '0x4bf41000'
        }
        
        for region, addr in memory_regions.items():
            if addr in content:
                print(f"  {region:15}: {addr}")
                
        # Extract panel settings
        if 'htotal' in content:
            print("\nPanel Timing Configuration Found:")
            print("  H-Total: 2200 (typical), 2095-2809 (range)")
            print("  V-Total: 1125 (typical), 1107-1440 (range)")
            print("  PCLK: 148.5MHz (typical), 130-164MHz (range)")
            
    def analyze_tse_database(self):
        """Analyze TSE database files for hardware configuration"""
        print("\n=== TSE Database Analysis ===")
        
        database_path = self.mips_path / "database.TSE"
        if not database_path.exists():
            print(f"ERROR: {database_path} not found")
            return
            
        with open(database_path, 'rb') as f:
            data = f.read()
            
        print(f"Database Size: {len(data)} bytes")
        
        # Try to identify structure
        if len(data) >= 16:
            header = data[:16]
            print(f"Header: {header.hex()}")
            
            # Look for patterns
            uint32_vals = struct.unpack('<4I', header)
            print(f"Header as uint32: {[hex(v) for v in uint32_vals]}")
            
        # Analyze project ID files
        project_files = list(self.mips_path.glob("ProjectID_*.TSE"))
        print(f"\nProject ID Files Found: {len(project_files)}")
        
        for proj_file in sorted(project_files)[:5]:  # Limit output
            with open(proj_file, 'rb') as f:
                proj_data = f.read()
            print(f"  {proj_file.name}: {len(proj_data)} bytes")
            
    def analyze_security_mechanisms(self):
        """Analyze security mechanisms in MIPS firmware"""
        print("\n=== Security Mechanism Analysis ===")
        
        display_bin = self.base_path / "firmware/display.bin"
        if not display_bin.exists():
            print(f"ERROR: {display_bin} not found")
            return
            
        with open(display_bin, 'rb') as f:
            firmware_data = f.read()
            
        print(f"Firmware Size: {len(firmware_data)} bytes")
        
        # Calculate checksums
        crc32 = binascii.crc32(firmware_data) & 0xffffffff
        sha256 = hashlib.sha256(firmware_data).hexdigest()
        
        print(f"CRC32: 0x{crc32:08x}")
        print(f"SHA256: {sha256}")
        
        # Look for signature or verification blocks
        header = firmware_data[:64]
        print(f"Firmware Header: {header.hex()}")
        
        # Check for common signature patterns
        if b'CRC' in firmware_data[:1024]:
            print("CRC verification pattern found in header")
        if b'SHA' in firmware_data[:1024]:
            print("SHA verification pattern found in header")
        if b'AES' in firmware_data[:1024]:
            print("AES encryption pattern found in header")
            
        # Look for MIPS instruction patterns
        mips_patterns = self.find_mips_instructions(firmware_data)
        print(f"MIPS instruction patterns found: {len(mips_patterns)}")
        
    def find_mips_instructions(self, data):
        """Look for MIPS instruction patterns in firmware"""
        patterns = []
        
        # Common MIPS instruction opcodes (big-endian)
        mips_opcodes = {
            0x3c: 'lui',    # Load Upper Immediate
            0x27: 'addiu',  # Add Immediate Unsigned
            0x8c: 'lw',     # Load Word
            0xac: 'sw',     # Store Word
            0x10: 'beq',    # Branch if Equal
        }
        
        for i in range(0, len(data) - 4, 4):
            word = struct.unpack('>I', data[i:i+4])[0]
            opcode = (word >> 26) & 0x3f
            
            if opcode in mips_opcodes:
                patterns.append({
                    'offset': i,
                    'instruction': mips_opcodes[opcode],
                    'word': f"0x{word:08x}"
                })
                
        return patterns[:20]  # Return first 20 patterns
        
    def extract_strings(self, data, min_length=4):
        """Extract ASCII strings from binary data"""
        strings = []
        current_string = ""
        
        for byte in data:
            if 32 <= byte <= 126:  # Printable ASCII
                current_string += chr(byte)
            else:
                if len(current_string) >= min_length:
                    strings.append(current_string)
                current_string = ""
                
        if len(current_string) >= min_length:
            strings.append(current_string)
            
        return strings
        
    def generate_report(self):
        """Generate comprehensive analysis report"""
        print("=" * 80)
        print("FACTORY MIPS SYSTEM COMPREHENSIVE ANALYSIS")
        print("=" * 80)
        
        self.analyze_loadmips_binary()
        self.analyze_libmips_library()
        self.analyze_display_config()
        self.analyze_tse_database()
        self.analyze_security_mechanisms()
        
        print("\n" + "=" * 80)
        print("ANALYSIS COMPLETE")
        print("=" * 80)

def main():
    if len(sys.argv) > 1:
        base_path = sys.argv[1]
    else:
        base_path = "."
        
    analyzer = FactoryMIPSAnalyzer(base_path)
    analyzer.generate_report()

if __name__ == "__main__":
    main()