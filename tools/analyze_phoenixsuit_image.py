#!/usr/bin/env python3
"""
PhoenixSuit Image Analyzer for HY300 Project

Analyzes and extracts Allwinner PhoenixSuit/LiveSuit firmware images.
Based on reverse engineering of PhoenixSuit V1.10.

Image Format:
- 96-byte header with magic, version, partition count
- Variable-size item table describing partitions
- Partition data (optionally encrypted)

Usage:
    python3 analyze_phoenixsuit_image.py <image_file>
    python3 analyze_phoenixsuit_image.py extract <image_file> <item_name> <output>
"""

import struct
import sys
import os
from pathlib import Path

class PhoenixSuitImage:
    """Parser for PhoenixSuit/LiveSuit firmware images"""
    
    # Image header constants (from imgdec_fun.lua)
    IMAGE_HEAD_SIZE = 96
    IMAGE_ITEM_SIZE_V1 = 1024  # Corrected from reverse engineering actual image
    IMAGE_ITEM_SIZE_V2 = 1024  # PhoenixSuit uses 1024-byte items
    
    # Magic signatures
    MAGIC_SIGNATURES = [
        b'IMAGEWTY',  # Standard signature
        b'ANDROID!',  # Android boot image
        b'\x69\x6d\x67',  # "img" signature
    ]
    
    def __init__(self, filepath):
        self.filepath = Path(filepath)
        self.header = {}
        self.items = []
        self.version = 0
        self.encrypted = False
        
    def parse(self):
        """Parse the image file"""
        if not self.filepath.exists():
            raise FileNotFoundError(f"Image file not found: {self.filepath}")
            
        with open(self.filepath, 'rb') as f:
            # Read and parse header
            header_data = f.read(self.IMAGE_HEAD_SIZE)
            if len(header_data) < self.IMAGE_HEAD_SIZE:
                raise ValueError("File too small to be valid PhoenixSuit image")
                
            self.parse_header(header_data)
            
            # Check if this is a valid PhoenixSuit image
            if not self.is_valid_image():
                print(f"⚠️  Warning: File may not be a PhoenixSuit image")
                print(f"   Magic bytes: {header_data[:8].hex()}")
                return False
                
            # Parse item table if present
            if self.header.get('item_count', 0) > 0:
                item_offset = self.header.get('item_offset', 0)
                if item_offset > 0:
                    f.seek(item_offset)
                    self.parse_item_table(f)
                    
        return True
        
    def parse_header(self, data):
        """Parse 96-byte image header"""
        try:
            # Header structure (from imgdec_fun.lua Img_Head64)
            # Offsets based on IMAGE_HEAD constants
            self.header['magic'] = data[0:8]
            self.header['version'] = struct.unpack('<I', data[8:12])[0]
            self.header['size'] = struct.unpack('<I', data[12:16])[0]
            self.header['attr'] = struct.unpack('<I', data[16:20])[0]
            self.header['img_version'] = data[20:52].decode('ascii', errors='ignore').rstrip('\x00')
            self.header['len_lo'] = struct.unpack('<I', data[56:60])[0]
            self.header['len_hi'] = struct.unpack('<I', data[60:64])[0]
            self.header['align'] = struct.unpack('<I', data[64:68])[0]
            self.header['pid'] = struct.unpack('<H', data[66:68])[0]
            self.header['vid'] = struct.unpack('<H', data[68:70])[0]
            self.header['hardware_id'] = struct.unpack('<H', data[70:72])[0]
            self.header['firmware_id'] = struct.unpack('<H', data[72:74])[0]
            self.header['item_attr'] = struct.unpack('<H', data[74:76])[0]
            self.header['item_size'] = struct.unpack('<H', data[56:58])[0]  # Corrected offset
            self.header['item_count'] = struct.unpack('<H', data[60:62])[0]  # Corrected offset
            self.header['item_offset'] = struct.unpack('<I', data[64:68])[0]  # Corrected offset
            self.header['image_attr'] = data[84]
            self.header['append_size'] = data[85]
            self.header['append_offset_lo'] = data[86]
            self.header['append_offset_hi'] = data[87]
            
            # Calculate full length
            self.header['full_length'] = self.header['len_lo'] + (self.header['len_hi'] << 32)
            
            # Detect encryption
            self.encrypted = (self.header['attr'] & 0x1) != 0
            
            # Detect version (32-bit vs 64-bit items)
            self.version = 2 if self.header.get('item_size', 0) >= self.IMAGE_ITEM_SIZE_V2 else 1
            
        except Exception as e:
            print(f"Error parsing header: {e}")
            
    def is_valid_image(self):
        """Check if this appears to be a valid PhoenixSuit image"""
        magic = self.header.get('magic', b'')
        
        # Check for known magic signatures
        for sig in self.MAGIC_SIGNATURES:
            if magic.startswith(sig):
                return True
                
        # Check for reasonable header values
        item_count = self.header.get('item_count', 0)
        item_offset = self.header.get('item_offset', 0)
        
        # Item count should be reasonable (1-100 partitions)
        if item_count > 0 and item_count < 100:
            # Item offset should be after header
            if item_offset >= self.IMAGE_HEAD_SIZE:
                return True
                
        return False
        
    def parse_item_table(self, f):
        """Parse partition item table"""
        item_size = self.header.get('item_size', self.IMAGE_ITEM_SIZE_V2)
        item_count = self.header.get('item_count', 0)
        
        for i in range(item_count):
            item_data = f.read(item_size)
            if len(item_data) < item_size:
                break
                
            item = self.parse_item(item_data)
            if item:
                self.items.append(item)
                
    def parse_item(self, data):
        """Parse single partition item"""
        try:
            item = {}
            
            # Common fields for both v1 and v2
            item['version'] = struct.unpack('<I', data[0:4])[0]
            item['size'] = struct.unpack('<I', data[4:8])[0]
            item['main_type'] = data[8:24].decode('ascii', errors='ignore').rstrip('\x00')
            item['sub_type'] = data[24:32].decode('ascii', errors='ignore').rstrip('\x00')
            item['filename'] = data[32:96].decode('ascii', errors='ignore').rstrip('\x00').split('\x00')[0]
            item['attr'] = struct.unpack('<I', data[32:36])[0]
            item['data_len_lo'] = struct.unpack('<I', data[36:40])[0]
            item['file_len_lo'] = struct.unpack('<I', data[40:44])[0]
            item['offset_lo'] = struct.unpack('<I', data[44:48])[0]
            
            if self.version == 2 and len(data) >= self.IMAGE_ITEM_SIZE_V2:
                # V2 format - 64-bit offsets and lengths
                item['data_len_hi'] = struct.unpack('<I', data[48:52])[0]
                item['file_len_hi'] = struct.unpack('<I', data[52:56])[0]
                item['offset_hi'] = struct.unpack('<I', data[56:60])[0]
                item['encrypt_id'] = struct.unpack('<I', data[60:64])[0]
                
                item['data_length'] = item['data_len_lo'] + (item['data_len_hi'] << 32)
                item['file_length'] = item['file_len_lo'] + (item['file_len_hi'] << 32)
                item['offset'] = item['offset_lo'] + (item['offset_hi'] << 32)
            else:
                # V1 format - 32-bit only
                item['checksum'] = struct.unpack('<I', data[48:52])[0] if len(data) >= 52 else 0
                item['data_length'] = item['data_len_lo']
                item['file_length'] = item['file_len_lo']
                item['offset'] = item['offset_lo']
                
            return item
            
        except Exception as e:
            print(f"Error parsing item: {e}")
            return None
            
    def print_info(self):
        """Print image information"""
        print("=" * 70)
        print("PhoenixSuit Image Analysis")
        print("=" * 70)
        print(f"File: {self.filepath}")
        print(f"Size: {self.filepath.stat().st_size:,} bytes")
        print()
        
        print("Header Information:")
        print("-" * 70)
        print(f"  Magic:        {self.header.get('magic', b'').hex()} ({self.header.get('magic', b'')})")
        print(f"  Version:      {self.header.get('version', 0)}")
        print(f"  Image Ver:    {self.header.get('img_version', 'N/A')}")
        print(f"  PID:          0x{self.header.get('pid', 0):04x}")
        print(f"  VID:          0x{self.header.get('vid', 0):04x}")
        print(f"  Hardware ID:  0x{self.header.get('hardware_id', 0):04x}")
        print(f"  Firmware ID:  0x{self.header.get('firmware_id', 0):04x}")
        print(f"  Encrypted:    {'Yes' if self.encrypted else 'No'}")
        print(f"  Item Format:  V{self.version} ({self.header.get('item_size', 0)} bytes/item)")
        print(f"  Item Count:   {self.header.get('item_count', 0)}")
        print(f"  Item Offset:  0x{self.header.get('item_offset', 0):08x}")
        print(f"  Full Length:  {self.header.get('full_length', 0):,} bytes")
        print()
        
        if self.items:
            print("Partition Table:")
            print("-" * 70)
            print(f"{'#':<4} {'Main Type':<16} {'Sub Type':<12} {'Offset':<12} {'Size':<12}")
            print("-" * 70)
            for i, item in enumerate(self.items):
                main_type = item.get('main_type', '')
                filename = item.get('filename', item.get('sub_type', ''))
                offset = item.get('offset', 0)
                size = item.get('data_length', 0)
                print(f"{i:<4} {main_type:<16} {filename:<20} 0x{offset:<10x} {size:>10,} B")
        print("=" * 70)
        
    def extract_item(self, item_name, output_path):
        """Extract a partition by name"""
        # Find item
        item = None
        for i in self.items:
            if i.get('main_type') == item_name or i.get('sub_type') == item_name:
                item = i
                break
                
        if not item:
            print(f"Error: Item '{item_name}' not found")
            return False
            
        # Extract data
        offset = item.get('offset', 0)
        length = item.get('data_length', 0)
        
        print(f"Extracting '{item_name}':")
        print(f"  Offset: 0x{offset:08x}")
        print(f"  Length: {length:,} bytes")
        
        try:
            with open(self.filepath, 'rb') as f:
                f.seek(offset)
                data = f.read(length)
                
            with open(output_path, 'wb') as f:
                f.write(data)
                
            print(f"  Output: {output_path}")
            print("✅ Extraction successful")
            return True
            
        except Exception as e:
            print(f"❌ Extraction failed: {e}")
            return False


def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print(f"  {sys.argv[0]} <image_file>")
        print(f"  {sys.argv[0]} extract <image_file> <item_name> <output_file>")
        sys.exit(1)
        
    if sys.argv[1] == 'extract':
        if len(sys.argv) < 5:
            print("Error: extract requires <image_file> <item_name> <output_file>")
            sys.exit(1)
            
        img = PhoenixSuitImage(sys.argv[2])
        if img.parse():
            img.extract_item(sys.argv[3], sys.argv[4])
    else:
        img = PhoenixSuitImage(sys.argv[1])
        if img.parse():
            img.print_info()
        else:
            print("❌ Failed to parse as PhoenixSuit image")
            print("\nAttempting hex dump of first 96 bytes:")
            try:
                with open(sys.argv[1], 'rb') as f:
                    data = f.read(96)
                    for i in range(0, len(data), 16):
                        hex_part = ' '.join(f'{b:02x}' for b in data[i:i+16])
                        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data[i:i+16])
                        print(f"{i:08x}  {hex_part:<48}  {ascii_part}")
            except Exception as e:
                print(f"Error: {e}")


if __name__ == '__main__':
    main()
