#!/usr/bin/env python3
"""
HY300 Accelerometer Detection and Integration Service

This service runs on boot to:
1. Detect which accelerometer is present using the detection utility
2. Update the sysfs interface with the detection result
3. Enable the appropriate device tree configuration
4. Provide runtime accelerometer information for keystone correction

Copyright (C) 2025 HY300 Linux Porting Project
SPDX-License-Identifier: GPL-2.0+
"""

import os
import sys
import time
import logging
import subprocess
import json
from pathlib import Path
from typing import Optional, Dict, Any


class AccelerometerService:
    """Service to manage HY300 accelerometer detection and integration."""
    
    def __init__(self):
        self.logger = self._setup_logging()
        self.detection_utility = "/usr/bin/accelerometer_detection"
        self.sysfs_path = "/sys/class/hy300/accelerometer_type"
        self.device_tree_overlays = "/sys/kernel/config/device-tree/overlays"
        self.service_state_file = "/tmp/hy300_accelerometer_state.json"
        
        # Ensure state directory exists
        os.makedirs(os.path.dirname(self.service_state_file), exist_ok=True)
    
    def _setup_logging(self) -> logging.Logger:
        """Setup logging configuration."""
        handlers = [logging.StreamHandler(sys.stdout)]
        
        # Try to add file handler, fall back gracefully if permissions fail
        try:
            handlers.append(logging.FileHandler('/var/log/hy300-accelerometer.log'))
        except (PermissionError, FileNotFoundError):
            # Cannot write to /var/log, continue with stdout only
            pass
        
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            handlers=handlers
        )
        return logging.getLogger('hy300-accelerometer')
    
    def detect_accelerometer(self) -> Optional[Dict[str, Any]]:
        """
        Run accelerometer detection utility and parse results.
        
        Returns:
            Dictionary with detection results or None if detection failed
        """
        if not os.path.exists(self.detection_utility):
            self.logger.error(f"Detection utility not found: {self.detection_utility}")
            return None
        
        try:
            self.logger.info("Running accelerometer detection...")
            result = subprocess.run(
                [self.detection_utility, "--verbose"],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode != 0:
                self.logger.error(f"Detection failed with code {result.returncode}")
                self.logger.error(f"STDERR: {result.stderr}")
                return None
            
            # Parse output to extract detection information
            output = result.stdout
            detected_info = self._parse_detection_output(output)
            
            self.logger.info(f"Detection successful: {detected_info}")
            return detected_info
            
        except subprocess.TimeoutExpired:
            self.logger.error("Detection utility timed out")
            return None
        except Exception as e:
            self.logger.error(f"Detection failed with exception: {e}")
            return None
    
    def _parse_detection_output(self, output: str) -> Dict[str, Any]:
        """
        Parse detection utility output to extract accelerometer information.
        
        Args:
            output: Raw output from detection utility
            
        Returns:
            Dictionary with parsed detection information
        """
        info = {
            'detected': False,
            'type': 'unknown',
            'address': None,
            'compatible': None,
            'driver': None
        }
        
        lines = output.split('\n')
        for line in lines:
            if "✓ Detected" in line and "accelerometer" in line:
                # Parse line like: "✓ Detected STK8BA58 accelerometer at I2C address 0x18"
                parts = line.split()
                if len(parts) >= 6:
                    info['detected'] = True
                    info['type'] = parts[2]  # STK8BA58 or KXTTJ3
                    info['address'] = parts[-1]  # 0x18 or 0x0e
            
            elif "Compatible string:" in line:
                info['compatible'] = line.split(": ")[1].strip()
            
            elif "Driver name:" in line:
                info['driver'] = line.split(": ")[1].strip()
        
        return info
    
    def update_sysfs_interface(self, accel_type: str) -> bool:
        """
        Update the sysfs interface with detected accelerometer type.
        
        Args:
            accel_type: Detected accelerometer type (STK8BA58, KXTTJ3, etc.)
            
        Returns:
            True if successful, False otherwise
        """
        try:
            if not os.path.exists(self.sysfs_path):
                self.logger.warning(f"Sysfs path not available: {self.sysfs_path}")
                # Wait for kernel module to be loaded
                max_wait = 30
                for i in range(max_wait):
                    if os.path.exists(self.sysfs_path):
                        break
                    time.sleep(1)
                else:
                    self.logger.error("Sysfs interface never became available")
                    return False
            
            with open(self.sysfs_path, 'w') as f:
                f.write(accel_type + '\n')
            
            self.logger.info(f"Updated sysfs interface: {accel_type}")
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to update sysfs interface: {e}")
            return False
    
    def apply_device_tree_overlay(self, accel_info: Dict[str, Any]) -> bool:
        """
        Apply appropriate device tree overlay for detected accelerometer.
        
        Args:
            accel_info: Dictionary with accelerometer detection information
            
        Returns:
            True if successful, False otherwise
        """
        if not accel_info.get('detected'):
            self.logger.warning("No accelerometer detected, skipping device tree overlay")
            return False
        
        accel_type = accel_info['type'].lower()
        overlay_name = f"hy300-{accel_type}"
        
        try:
            # Create device tree overlay content
            overlay_content = self._generate_dt_overlay(accel_info)
            
            # Apply via configfs (if available)
            if os.path.exists(self.device_tree_overlays):
                overlay_dir = os.path.join(self.device_tree_overlays, overlay_name)
                os.makedirs(overlay_dir, exist_ok=True)
                
                dtbo_path = os.path.join(overlay_dir, "dtbo")
                with open(dtbo_path, 'w') as f:
                    f.write(overlay_content)
                
                self.logger.info(f"Applied device tree overlay: {overlay_name}")
                return True
            else:
                self.logger.warning("Device tree overlay interface not available")
                # Log the overlay that would be applied
                self.logger.info(f"Would apply overlay:\n{overlay_content}")
                return True
                
        except Exception as e:
            self.logger.error(f"Failed to apply device tree overlay: {e}")
            return False
    
    def _generate_dt_overlay(self, accel_info: Dict[str, Any]) -> str:
        """
        Generate device tree overlay content for detected accelerometer.
        
        Args:
            accel_info: Dictionary with accelerometer detection information
            
        Returns:
            Device tree overlay source code
        """
        accel_type = accel_info['type']
        address = accel_info['address']
        compatible = accel_info['compatible']
        
        # Convert hex address to decimal for reg property
        addr_int = int(address, 16) if address else 0x18
        
        overlay = f"""
/dts-v1/;
/plugin/;

/ {{
    compatible = "allwinner,sun50i-h713";
    
    fragment@0 {{
        target-path = "/soc/i2c@5002400";
        __overlay__ {{
            accelerometer: {accel_type.lower()}@{addr_int:x} {{
                compatible = "{compatible}";
                reg = <0x{addr_int:02x}>;
                interrupt-parent = <&pio>;
                interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */
                status = "okay";
        """
        
        # Add device-specific properties
        if accel_type == "STK8BA58":
            overlay += """
                stk,direction = <2>;
            """
        
        overlay += """
            };
        };
    };
};
"""
        
        return overlay
    
    def save_state(self, detection_info: Dict[str, Any]) -> None:
        """
        Save accelerometer detection state for later use.
        
        Args:
            detection_info: Dictionary with detection results
        """
        try:
            state = {
                'timestamp': int(time.time()),
                'detection': detection_info,
                'service_version': '1.0'
            }
            
            with open(self.service_state_file, 'w') as f:
                json.dump(state, f, indent=2)
            
            self.logger.info(f"Saved state to {self.service_state_file}")
            
        except Exception as e:
            self.logger.error(f"Failed to save state: {e}")
    
    def load_state(self) -> Optional[Dict[str, Any]]:
        """
        Load previously saved accelerometer detection state.
        
        Returns:
            Dictionary with previous state or None if not available
        """
        try:
            if os.path.exists(self.service_state_file):
                with open(self.service_state_file, 'r') as f:
                    state = json.load(f)
                
                self.logger.info("Loaded previous state")
                return state
            
        except Exception as e:
            self.logger.error(f"Failed to load state: {e}")
        
        return None
    
    def run_detection_cycle(self) -> bool:
        """
        Run a complete detection and integration cycle.
        
        Returns:
            True if successful, False otherwise
        """
        self.logger.info("Starting accelerometer detection cycle")
        
        # Run detection
        detection_info = self.detect_accelerometer()
        if not detection_info:
            self.logger.error("Accelerometer detection failed")
            return False
        
        if not detection_info.get('detected'):
            self.logger.warning("No accelerometer detected")
            # Update sysfs with 'none' to indicate no device
            self.update_sysfs_interface('none')
            return False
        
        # Update sysfs interface
        if not self.update_sysfs_interface(detection_info['type']):
            self.logger.error("Failed to update sysfs interface")
            # Continue anyway - detection worked
        
        # Apply device tree overlay
        if not self.apply_device_tree_overlay(detection_info):
            self.logger.error("Failed to apply device tree overlay")
            # Continue anyway - detection worked
        
        # Save state for later reference
        self.save_state(detection_info)
        
        self.logger.info("Detection cycle completed successfully")
        return True
    
    def run_service(self, daemon_mode: bool = False) -> int:
        """
        Run the accelerometer service.
        
        Args:
            daemon_mode: If True, run as daemon with periodic checks
            
        Returns:
            Exit code (0 for success)
        """
        self.logger.info("Starting HY300 Accelerometer Service")
        
        try:
            if daemon_mode:
                self.logger.info("Running in daemon mode")
                while True:
                    self.run_detection_cycle()
                    # Check every 5 minutes for hardware changes
                    time.sleep(300)
            else:
                # Single detection cycle
                success = self.run_detection_cycle()
                return 0 if success else 1
                
        except KeyboardInterrupt:
            self.logger.info("Service interrupted by user")
            return 0
        except Exception as e:
            self.logger.error(f"Service failed with exception: {e}")
            return 1


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="HY300 Accelerometer Detection and Integration Service"
    )
    parser.add_argument(
        '--daemon', '-d',
        action='store_true',
        help='Run as daemon with periodic detection'
    )
    parser.add_argument(
        '--detect-once', '-o',
        action='store_true',
        help='Run single detection cycle and exit'
    )
    parser.add_argument(
        '--status', '-s',
        action='store_true',
        help='Show current accelerometer status'
    )
    
    args = parser.parse_args()
    
    service = AccelerometerService()
    
    if args.status:
        # Show current status
        state = service.load_state()
        if state:
            print(f"Last detection: {time.ctime(state['timestamp'])}")
            detection = state['detection']
            if detection.get('detected'):
                print(f"Accelerometer: {detection['type']} at {detection['address']}")
                print(f"Compatible: {detection['compatible']}")
                print(f"Driver: {detection['driver']}")
            else:
                print("No accelerometer detected")
        else:
            print("No previous detection state available")
        return 0
    
    elif args.detect_once or not args.daemon:
        # Single detection cycle
        return service.run_service(daemon_mode=False)
    
    else:
        # Daemon mode
        return service.run_service(daemon_mode=True)


if __name__ == '__main__':
    sys.exit(main())