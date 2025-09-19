# HY300 WiFi Management Service - Simple Implementation
{ lib, stdenv, writeText, python3, networkmanager }:

stdenv.mkDerivation {
  pname = "hy300-wifi";
  version = "1.0.0";
  
  src = writeText "wifi-manager.py" ''
    #!/usr/bin/env python3
    """
    HY300 WiFi Management Service
    Simple WiFi management using NetworkManager
    """
    
    import os
    import sys
    import json
    import time
    import signal
    import logging
    import argparse
    import subprocess
    from pathlib import Path
    
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    logger = logging.getLogger('hy300-wifi')
    
    class WiFiService:
        def __init__(self, simulation_mode=False):
            self.simulation_mode = simulation_mode
            self.config_file = Path("/var/lib/hy300/wifi.json")
            self.running = True
            
            # WiFi profiles
            self.profiles = {}
            self.load_config()
            logger.info(f"WiFi service started (simulation={simulation_mode})")
        
        def load_config(self):
            try:
                if self.config_file.exists():
                    with open(self.config_file) as f:
                        self.profiles = json.load(f)
                else:
                    self.save_config()
            except Exception as e:
                logger.error(f"Config load failed: {e}")
        
        def save_config(self):
            try:
                self.config_file.parent.mkdir(parents=True, exist_ok=True)
                with open(self.config_file, 'w') as f:
                    json.dump(self.profiles, f, indent=2)
            except Exception as e:
                logger.error(f"Config save failed: {e}")
        
        def nmcli_command(self, args):
            """Run nmcli command"""
            if self.simulation_mode:
                logger.info(f"nmcli (sim): {' '.join(args)}")
                return True, "OK"
            
            try:
                result = subprocess.run(['nmcli'] + args, capture_output=True, text=True, timeout=30)
                return result.returncode == 0, result.stdout.strip()
            except Exception as e:
                logger.error(f"nmcli failed: {e}")
                return False, str(e)
        
        def scan_networks(self):
            """Scan for available networks"""
            if self.simulation_mode:
                return ["TestNetwork", "ProjectorWiFi", "OpenNetwork"]
            
            success, output = self.nmcli_command(["device", "wifi", "list"])
            if success:
                # Parse network list (simplified)
                networks = []
                for line in output.split('\n')[1:]:  # Skip header
                    if line.strip():
                        parts = line.split()
                        if parts:
                            networks.append(parts[0])
                return networks
            return []
        
        def connect_to_network(self, ssid, password=None):
            """Connect to WiFi network"""
            logger.info(f"Connecting to {ssid}")
            
            if self.simulation_mode:
                logger.info(f"Connected to {ssid} (simulation)")
                self.profiles[ssid] = {"password": password or "", "last_used": time.time()}
                self.save_config()
                return True
            
            args = ["device", "wifi", "connect", ssid]
            if password:
                args.extend(["password", password])
            
            success, output = self.nmcli_command(args)
            if success:
                logger.info(f"Connected to {ssid}")
                self.profiles[ssid] = {"password": password or "", "last_used": time.time()}
                self.save_config()
                return True
            else:
                logger.error(f"Failed to connect: {output}")
                return False
        
        def get_status(self):
            """Get current WiFi status"""
            if self.simulation_mode:
                return {"connected": True, "ssid": "TestNetwork", "signal": 85}
            
            success, output = self.nmcli_command(["device", "status"])
            if success and "wifi" in output and "connected" in output:
                return {"connected": True, "status": "connected"}
            return {"connected": False}
        
        def monitor_connection(self):
            """Monitor WiFi connection and auto-reconnect"""
            status = self.get_status()
            if not status.get("connected"):
                # Try to reconnect to last used network
                if self.profiles:
                    last_ssid = max(self.profiles.keys(), 
                                  key=lambda k: self.profiles[k].get("last_used", 0))
                    password = self.profiles[last_ssid].get("password")
                    logger.info(f"Attempting auto-reconnect to {last_ssid}")
                    self.connect_to_network(last_ssid, password)
        
        def run(self):
            """Main service loop"""
            while self.running:
                try:
                    self.monitor_connection()
                    time.sleep(30)  # Check every 30 seconds
                except Exception as e:
                    logger.error(f"Service error: {e}")
                    time.sleep(60)
        
        def stop(self):
            self.running = False
            logger.info("WiFi service stopping")
    
    def main():
        parser = argparse.ArgumentParser(description='HY300 WiFi Service')
        parser.add_argument('--simulation', action='store_true', help='Simulation mode')
        args = parser.parse_args()
        
        service = WiFiService(simulation_mode=args.simulation)
        
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
  
  buildInputs = [ python3 networkmanager ];
  
  installPhase = ''
    mkdir -p $out/bin
    cp $src $out/bin/hy300-wifi
    chmod +x $out/bin/hy300-wifi
  '';
  
  meta = with lib; {
    description = "Simple HY300 WiFi management service";
    license = licenses.mit;
  };
}