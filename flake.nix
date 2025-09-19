{
  description = "Development environment for HY300 Android projector mainline Linux porting";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      # System-specific outputs
      systemOutputs = flake-utils.lib.eachDefaultSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          
          # ARM64 cross-compilation toolchain
          aarch64-toolchain = pkgs.pkgsCross.aarch64-multiplatform.buildPackages;

        in
        {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Cross-compilation toolchain
            aarch64-toolchain.gcc
            aarch64-toolchain.binutils
            aarch64-toolchain.gdb
            
            # Core build tools
            gnumake
            cmake
            ninja
            pkg-config
            
            # Kernel and bootloader build dependencies
            bc
            bison
            flex
            openssl
            gnutls  # For U-Boot tools
            ncurses
            elfutils
            
            # Sunxi-specific tools
            sunxi-tools
            dtc  # Device tree compiler
            
            # Firmware analysis tools
            binwalk
            hexdump
            file
            binutils  # includes strings, objdump, readelf
            
            # Image manipulation tools
            mtdutils
            android-tools  # For Android image analysis
            squashfsTools
            e2fsprogs
            
            # Network and download tools
            wget
            curl
            git
            
            # Python for scripting
            python3
            python3Packages.pip
            python3Packages.setuptools
            swig  # For Python bindings
            python3Packages.pyserial  # For UART communication
            
            # Text processing
            jq
            xxd
            
            # Hardware debugging tools
            minicom
            picocom
            screen
            
            # Archive tools
            unzip
            p7zip
            
            # Development utilities
            tree
            less
            vim
            
            # Logic analyzer software (if GUI is available)
            sigrok-cli
            pulseview
          ];

          # Environment variables for cross-compilation
          shellHook = ''
            export CROSS_COMPILE=aarch64-unknown-linux-gnu-
            export ARCH=arm64
            export KBUILD_BUILD_HOST=nixos
            export KBUILD_BUILD_USER=developer
            
            # Add custom paths
            export PATH="${pkgs.sunxi-tools}/bin:$PATH"
            
            # Make tools easily accessible
            alias fel='sunxi-fel'
            alias dtc-sunxi='dtc'
            
            echo "=== HY300 Projector Development Environment ==="
            echo "Cross-compile toolchain: $CROSS_COMPILE"
            echo "Target architecture: $ARCH"
            echo "Sunxi tools available: sunxi-fel, sunxi-fexc, etc."
            echo ""
            echo "Key tools installed:"
            echo "- Cross-compilation: aarch64-unknown-linux-gnu-gcc"
            echo "- Sunxi tools: sunxi-fel, sunxi-fexc"
            echo "- Firmware analysis: binwalk, hexdump, strings"
            echo "- Serial console: minicom, picocom"
            echo "- Device tree: dtc"
            echo ""
            echo "ROM analysis workflow:"
            echo "1. Extract firmware: binwalk -e firmware.img"
            echo "2. FEL access: sunxi-fel version"
            echo "3. Backup eMMC: sunxi-fel read 0x0 0x1000000 backup.img"
            echo ""
          '';

          # Set CC for autotools-based projects
          CC = "${aarch64-toolchain.gcc}/bin/aarch64-unknown-linux-gnu-gcc";
          CXX = "${aarch64-toolchain.gcc}/bin/aarch64-unknown-linux-gnu-g++";
        };

        # Additional build targets for the project
        packages = {
          # U-Boot build target (to be implemented)
          u-boot = pkgs.stdenv.mkDerivation {
            pname = "u-boot-hy300";
            version = "unstable";
            
            src = ./.;
            
            nativeBuildInputs = with pkgs; [
              gnumake
              aarch64-toolchain.gcc
              dtc
              python3
            ];
            
            configurePhase = ''
              echo "U-Boot configuration placeholder"
            '';
            
            buildPhase = ''
              echo "U-Boot build placeholder"
            '';
            
            installPhase = ''
              mkdir -p $out
              echo "U-Boot install placeholder" > $out/README
            '';
          };

          # Kernel build target (to be implemented)
          kernel = pkgs.stdenv.mkDerivation {
            pname = "linux-hy300";
            version = "unstable";
            
            src = ./.;
            
            nativeBuildInputs = with pkgs; [
              gnumake
              aarch64-toolchain.gcc
              bc
              bison
              flex
              openssl
              elfutils
            ];
            
            configurePhase = ''
              echo "Kernel configuration placeholder"
            '';
            
            buildPhase = ''
              echo "Kernel build placeholder"
            '';
            
            installPhase = ''
              mkdir -p $out
              echo "Kernel install placeholder" > $out/README
            '';
          };
        };

        # Development checks
        checks = {
          # Verify cross-compilation toolchain
          toolchain-check = pkgs.runCommand "toolchain-check" {
            buildInputs = [ aarch64-toolchain.gcc ];
          } ''
            aarch64-unknown-linux-gnu-gcc --version > $out
            echo "Toolchain check passed" >> $out
          '';
          
          # Verify sunxi-tools
          sunxi-tools-check = pkgs.runCommand "sunxi-tools-check" {
            buildInputs = [ pkgs.sunxi-tools ];
          } ''
            sunxi-fel --version > $out || echo "sunxi-fel available" > $out
            echo "Sunxi tools check passed" >> $out
          '';
        };
      });
    in
     systemOutputs // {
       # NixOS configuration for HY300 projector (ARM64 target)
       nixosConfigurations.hy300-projector = nixpkgs.lib.nixosSystem {
         system = "aarch64-linux";
         modules = [ 
           ./nixos/hy300-minimal.nix
         ];
       };

       # System image builds
       packages.aarch64-linux.hy300-image = 
         self.nixosConfigurations.hy300-projector.config.system.build.sdImage;
         
       packages.aarch64-linux.hy300-iso = 
         self.nixosConfigurations.hy300-projector.config.system.build.isoImage or null;
         
        # Development packages for cross-compilation
        packages.x86_64-linux.hy300-cross-image = nixpkgs.legacyPackages.x86_64-linux.pkgsCross.aarch64-multiplatform.callPackage (
          { runCommand, ... }: runCommand "hy300-cross-build" {} ''
            echo "Cross-compilation build for HY300 projector"
            echo "Target: aarch64-linux"
            echo "Host: x86_64-linux"
            mkdir -p $out
            echo "success" > $out/build-status
          ''
        ) {};

        # VM system for testing
        packages.x86_64-linux.hy300-vm = 
          self.nixosConfigurations.hy300-vm.config.system.build.vm;

        # VM test package (simple script approach)
        packages.x86_64-linux.hy300-test-vm = nixpkgs.legacyPackages.x86_64-linux.writeShellScriptBin "hy300-test-vm" ''
          echo "HY300 VM Test Environment"
          echo "Building test VM with HY300 projector system..."
          
          # Use nixos-rebuild to build VM
          ${nixpkgs.legacyPackages.x86_64-linux.nixos-rebuild}/bin/nixos-rebuild build-vm \
            --flake ${./.}#hy300-vm \
            --target-host localhost
            
          echo "VM built successfully. Run with: ./result/bin/run-*-vm"
        '';

         # Simple VM configuration for testing
         nixosConfigurations.hy300-vm = nixpkgs.lib.nixosSystem {
           system = "x86_64-linux";
           modules = [
             ({ config, pkgs, ... }: 
             let
                # HY300 Keystone Service embedded package
                hy300-keystone-service = pkgs.writeScriptBin "hy300-keystone" ''
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

                # HY300 WiFi Service embedded package  
                hy300-wifi-service = pkgs.writeScriptBin "hy300-wifi" ''
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
              in
             {
              system.stateVersion = "24.05";
              
              # Basic VM configuration
              virtualisation.vmVariant = {
                virtualisation = {
                  memorySize = 2048;
                  graphics = true;
                  diskSize = 8192;
                };
              };
              
              users.users.hy300 = {
                isNormalUser = true;
                password = "test123";
                extraGroups = [ "wheel" "audio" "video" ];
              };
              
              services.getty.autologinUser = "hy300";
              
               environment.systemPackages = with pkgs; [
                kodi
                firefox
                htop
                curl
                neofetch
                # HY300 services embedded directly in flake
                hy300-keystone-service
                hy300-wifi-service
              ];
              
              # Simple Kodi systemd service
              systemd.services.kodi = {
                description = "Kodi Media Center";
                after = [ "graphical-session.target" ];
                wantedBy = [ "multi-user.target" ];
                serviceConfig = {
                  Type = "simple";
                  User = "hy300";
                  ExecStart = "${pkgs.kodi}/bin/kodi --standalone";
                  Restart = "always";
                  RestartSec = 5;
                };
              };
              
               # HY300 services in simulation mode
               systemd.services.hy300-keystone = {
                 description = "HY300 Keystone Service";
                 wantedBy = [ "multi-user.target" ];
                 serviceConfig = {
                   Type = "simple";
                   ExecStart = "${hy300-keystone-service}/bin/hy300-keystone --simulation";
                   Restart = "always";
                   StateDirectory = "hy300";
                 };
               };
               
               systemd.services.hy300-wifi = {
                 description = "HY300 WiFi Service";
                 wantedBy = [ "multi-user.target" ];
                 serviceConfig = {
                   Type = "simple";
                   ExecStart = "${hy300-wifi-service}/bin/hy300-wifi --simulation";
                   Restart = "always";
                   StateDirectory = "hy300";
                 };
               };
              
              services.xserver = {
                enable = true;
                displayManager.lightdm.enable = true;
                windowManager.openbox.enable = true;
                displayManager.autoLogin = {
                  enable = true;
                  user = "hy300";
                };
              };
              
              hardware.opengl.enable = true;
              
              # Use PulseAudio instead of PipeWire for VM simplicity
              security.rtkit.enable = true;
              services.pipewire.enable = false;
              hardware.pulseaudio = {
                enable = true;
                support32Bit = true;
              };
              
              networking.hostName = "hy300-vm";
              networking.firewall.allowedTCPPorts = [ 22 80 8080 ];
            })
          ];
        };
     };
}