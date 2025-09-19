# HY300 Keystone Correction Service - Simple Implementation
{ lib, stdenv, writeText, python3 }:

stdenv.mkDerivation {
  pname = "hy300-keystone";
  version = "1.0.0";
  
  src = writeText "keystone.py" ''
    #!/usr/bin/env python3
    """
    HY300 Keystone Correction Service
    Simple implementation that handles keystone correction and motor control.
    """
    
    import os
    import sys
    import json
    import time
    import signal
    import logging
    import argparse
    import math
    from pathlib import Path
    
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    logger = logging.getLogger('hy300-keystone')
    
    class KeystoneService:
        def __init__(self, simulation_mode=False):
            self.simulation_mode = simulation_mode
            self.config_file = Path("/var/lib/hy300/keystone.json")
            self.motor_device = Path("/dev/keystone-motor") 
            self.running = True
            
            # Default settings
            self.settings = {
                "auto_correction": True,
                "motor_position": {"h": 0, "v": 0},
                "keystone_values": {"tl": 0, "tr": 0, "bl": 0, "br": 0}
            }
            
            self.load_config()
            logger.info(f"Keystone service started (simulation={simulation_mode})")
        
        def load_config(self):
            try:
                if self.config_file.exists():
                    with open(self.config_file) as f:
                        self.settings.update(json.load(f))
                else:
                    self.save_config()
            except Exception as e:
                logger.error(f"Config load failed: {e}")
        
        def save_config(self):
            try:
                self.config_file.parent.mkdir(parents=True, exist_ok=True)
                with open(self.config_file, 'w') as f:
                    json.dump(self.settings, f, indent=2)
            except Exception as e:
                logger.error(f"Config save failed: {e}")
        
        def control_motor(self, h_steps, v_steps):
            """Control keystone correction motors"""
            if self.simulation_mode:
                logger.info(f"Motor control (sim): H={h_steps}, V={v_steps}")
                self.settings["motor_position"] = {"h": h_steps, "v": v_steps}
                return True
            
            try:
                if self.motor_device.exists():
                    with open(self.motor_device, 'w') as f:
                        f.write(f"{h_steps},{v_steps}\n")
                    self.settings["motor_position"] = {"h": h_steps, "v": v_steps}
                    logger.info(f"Motor moved: H={h_steps}, V={v_steps}")
                    return True
                else:
                    logger.warning("Motor device not available")
                    return False
            except Exception as e:
                logger.error(f"Motor control failed: {e}")
                return False
        
        def auto_correct(self):
            """Perform automatic keystone correction based on tilt"""
            if not self.settings["auto_correction"]:
                return
            
            # Simulate reading accelerometer data
            if self.simulation_mode:
                # Simulate small random movements
                import random
                tilt_x = random.uniform(-0.1, 0.1)
                tilt_y = random.uniform(-0.1, 0.1)
            else:
                # Read from actual accelerometer (placeholder)
                tilt_x = 0.0
                tilt_y = 0.0
            
            # Apply correction if tilt is significant
            if abs(tilt_x) > 0.05 or abs(tilt_y) > 0.05:
                h_correction = int(tilt_x * 100)  # Convert to motor steps
                v_correction = int(tilt_y * 100)
                
                current_h = self.settings["motor_position"]["h"]
                current_v = self.settings["motor_position"]["v"]
                
                new_h = max(-100, min(100, current_h + h_correction))
                new_v = max(-100, min(100, current_v + v_correction))
                
                if new_h != current_h or new_v != current_v:
                    self.control_motor(new_h, new_v)
                    self.save_config()
        
        def run(self):
            """Main service loop"""
            while self.running:
                try:
                    self.auto_correct()
                    time.sleep(2.0)  # Check every 2 seconds
                except Exception as e:
                    logger.error(f"Service error: {e}")
                    time.sleep(5.0)
        
        def stop(self):
            self.running = False
            logger.info("Keystone service stopping")
    
    def main():
        parser = argparse.ArgumentParser(description='HY300 Keystone Service')
        parser.add_argument('--simulation', action='store_true', help='Simulation mode')
        args = parser.parse_args()
        
        service = KeystoneService(simulation_mode=args.simulation)
        
        def signal_handler(signum, frame):
            service.stop()
            sys.exit(0)
        
        signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGINT, signal_handler)
        
        try:
            service.run()
        except KeyboardInterrupt:
            service.stop()
    
    if __name__ == "__main__":
        main()
  '';
  
  buildInputs = [ python3 ];
  
  installPhase = ''
    mkdir -p $out/bin
    cp $src $out/bin/hy300-keystone
    chmod +x $out/bin/hy300-keystone
  '';
  
  meta = with lib; {
    description = "Simple HY300 keystone correction service";
    license = licenses.mit;
  };
}