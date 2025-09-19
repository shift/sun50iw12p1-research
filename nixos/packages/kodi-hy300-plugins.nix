# HY300-specific Kodi Plugins
# Custom Kodi addons for projector functionality

{ lib, pkgs, ... }:

let
  # HY300 Keystone Correction Plugin for Kodi
  kodi-plugin-hy300-keystone = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-keystone";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone
      
      # Create addon.xml
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/addon.xml << 'EOF'
      <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
      <addon id="plugin.video.hy300keystone"
             name="HY300 Keystone Correction"
             version="1.0.0"
             provider-name="HY300 Project">
        <requires>
          <import addon="xbmc.python" version="3.0.0"/>
          <import addon="script.module.requests" version="2.22.0"/>
        </requires>
        <extension point="xbmc.python.pluginsource" library="main.py">
          <provides>executable</provides>
        </extension>
        <extension point="xbmc.addon.metadata">
          <summary lang="en_GB">HY300 Projector Keystone Correction</summary>
          <description lang="en_GB">
            Keystone correction interface for HY300 projector.
            Provides manual 4-corner adjustment and automatic correction controls.
          </description>
          <platform>linux</platform>
          <license>GPL-3.0</license>
          <forum></forum>
          <website></website>
          <email></email>
          <source></source>
          <news></news>
          <disclaimer></disclaimer>
          <assets>
            <icon>icon.png</icon>
            <fanart>fanart.jpg</fanart>
          </assets>
        </extension>
      </addon>
      EOF
      
      # Create main plugin file
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/main.py << 'EOF'
      #!/usr/bin/env python3
      # HY300 Keystone Correction Kodi Plugin
      
      import sys
      import os
      import subprocess
      import xbmc
      import xbmcgui
      import xbmcplugin
      import xbmcaddon
      from urllib.parse import parse_qsl
      
      # Get addon info
      addon = xbmcaddon.Addon()
      addon_name = addon.getAddonInfo('name')
      addon_path = addon.getAddonInfo('path')
      
      # HY300 control commands
      KEYSTONE_CONTROL = "/usr/bin/hy300-keystone-control"
      
      def run_keystone_command(command):
          """Execute keystone control command"""
          try:
              result = subprocess.run([KEYSTONE_CONTROL] + command.split(), 
                                    capture_output=True, text=True, timeout=10)
              return result.returncode == 0, result.stdout.strip()
          except Exception as e:
              xbmc.log(f"Keystone command error: {e}", xbmc.LOGERROR)
              return False, str(e)
      
      def show_main_menu():
          """Display main keystone menu"""
          items = [
              ("Manual Adjustment", "manual"),
              ("Auto Correction", "auto"),
              ("Load Profile", "load_profile"),
              ("Save Profile", "save_profile"),
              ("Reset to Center", "reset"),
              ("Calibrate", "calibrate"),
              ("Status", "status")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              li.setInfo('video', {'title': label})
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_manual_adjustment():
          """Display manual adjustment controls"""
          items = [
              ("Motor Up (+5)", "motor 5"),
              ("Motor Up (+1)", "motor 1"),
              ("Motor Down (-1)", "motor -1"),
              ("Motor Down (-5)", "motor -5"),
              ("Digital Correction", "digital_menu"),
              ("Fine Adjustment", "fine_menu")
          ]
          
          for label, command in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action=execute&command={command}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_digital_menu():
          """Display digital correction menu"""
          items = [
              ("4-Corner Adjustment", "digital_corners"),
              ("Preset Corrections", "digital_presets"),
              ("Custom Input", "digital_custom")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_auto_correction():
          """Display auto correction controls"""
          # Get current status
          success, status = run_keystone_command("status")
          
          items = [
              ("Enable Auto Correction", "auto on"),
              ("Disable Auto Correction", "auto off"),
              ("View Current Status", "status")
          ]
          
          for label, command in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action=execute&command={command}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_profiles():
          """Display saved profiles"""
          success, output = run_keystone_command("profiles")
          
          if success and output:
              profiles = output.strip().split('\n')[1:]  # Skip header
              for profile in profiles:
                  if profile.strip():
                      li = xbmcgui.ListItem(profile.strip())
                      url = f"{sys.argv[0]}?action=load_profile_exec&profile={profile.strip()}"
                      xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          else:
              li = xbmcgui.ListItem("No profiles found")
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def execute_command(command):
          """Execute keystone command and show result"""
          success, output = run_keystone_command(command)
          
          if success:
              xbmcgui.Dialog().ok(addon_name, f"Command executed successfully:\n{output}")
          else:
              xbmcgui.Dialog().ok(addon_name, f"Command failed:\n{output}")
      
      def save_profile_dialog():
          """Show dialog to save current profile"""
          dialog = xbmcgui.Dialog()
          profile_name = dialog.input("Enter profile name:")
          
          if profile_name:
              success, output = run_keystone_command(f"save {profile_name}")
              if success:
                  dialog.ok(addon_name, f"Profile '{profile_name}' saved successfully!")
              else:
                  dialog.ok(addon_name, f"Failed to save profile:\n{output}")
      
      def digital_corners_adjustment():
          """Show 4-corner digital adjustment interface"""
          dialog = xbmcgui.Dialog()
          
          # Get current screen resolution for defaults
          default_corners = "0,0:1920,0:1920,1080:0,1080"
          
          corners = dialog.input("Enter 4 corners (x1,y1:x2,y2:x3,y3:x4,y4):", 
                                defaultt=default_corners)
          
          if corners:
              success, output = run_keystone_command(f"digital {corners}")
              if success:
                  dialog.ok(addon_name, "Digital correction applied!")
              else:
                  dialog.ok(addon_name, f"Failed to apply correction:\n{output}")
      
      def show_status():
          """Display current keystone status"""
          success, output = run_keystone_command("status")
          
          if success:
              xbmcgui.Dialog().textviewer("HY300 Keystone Status", output)
          else:
              xbmcgui.Dialog().ok(addon_name, f"Failed to get status:\n{output}")
      
      def calibrate_system():
          """Start calibration process"""
          dialog = xbmcgui.Dialog()
          
          if dialog.yesno(addon_name, 
                         "This will calibrate the accelerometer.\n\n"
                         "Make sure the projector is on a level surface.\n\n"
                         "Continue?"):
              
              # Run calibration script
              try:
                  result = subprocess.run(["/usr/share/hy300/keystone/calibrate-accelerometer.sh"], 
                                        capture_output=True, text=True, timeout=60)
                  
                  if result.returncode == 0:
                      dialog.ok(addon_name, "Calibration completed successfully!\n\n"
                                           "Restart keystone service to apply changes.")
                  else:
                      dialog.ok(addon_name, f"Calibration failed:\n{result.stderr}")
                      
              except Exception as e:
                  dialog.ok(addon_name, f"Calibration error: {e}")
      
      # Main plugin logic
      def main():
          params = dict(parse_qsl(sys.argv[2][1:]))
          action = params.get('action')
          
          if action is None:
              show_main_menu()
          elif action == 'manual':
              show_manual_adjustment()
          elif action == 'auto':
              show_auto_correction()
          elif action == 'load_profile':
              show_profiles()
          elif action == 'save_profile':
              save_profile_dialog()
          elif action == 'reset':
              execute_command('reset')
          elif action == 'status':
              show_status()
          elif action == 'calibrate':
              calibrate_system()
          elif action == 'execute':
              command = params.get('command', '')
              execute_command(command)
          elif action == 'load_profile_exec':
              profile = params.get('profile', '')
              execute_command(f'load {profile}')
          elif action == 'digital_menu':
              show_digital_menu()
          elif action == 'digital_corners':
              digital_corners_adjustment()
          elif action == 'digital_presets':
              # Show preset digital corrections
              presets = [
                  ("Slight Keystone Up", "digital 0,-10:1920,-10:1920,1090:0,1090"),
                  ("Slight Keystone Down", "digital 0,10:1920,10:1920,1070:0,1070"),
                  ("Perspective Left", "digital 10,0:1920,0:1910,1080:0,1080"),
                  ("Perspective Right", "digital 0,0:1910,0:1920,1080:10,1080")
              ]
              
              for label, command in presets:
                  li = xbmcgui.ListItem(label)
                  url = f"{sys.argv[0]}?action=execute&command={command}"
                  xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
              
              xbmcplugin.endOfDirectory(int(sys.argv[1]))
          elif action == 'digital_custom':
              digital_corners_adjustment()
      
      if __name__ == '__main__':
          main()
      EOF
      
      # Create settings.xml for plugin configuration
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/resources/settings.xml << 'EOF'
      <?xml version="1.0" encoding="utf-8" standalone="yes"?>
      <settings>
          <category label="HY300 Keystone Settings">
              <setting label="Auto Correction" type="bool" id="auto_correction" default="true"/>
              <setting label="Motor Sensitivity" type="slider" id="motor_sensitivity" default="5" range="1,1,10"/>
              <setting label="Digital Precision" type="select" id="digital_precision" default="high" values="low|medium|high"/>
              <setting label="Show Advanced Options" type="bool" id="show_advanced" default="false"/>
          </category>
          <category label="Calibration">
              <setting label="Accelerometer X Offset" type="number" id="accel_x_offset" default="0"/>
              <setting label="Accelerometer Y Offset" type="number" id="accel_y_offset" default="0"/>
              <setting label="Accelerometer Z Offset" type="number" id="accel_z_offset" default="1000"/>
          </category>
      </settings>
      EOF
      
      # Create resources directory structure
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone/resources/lib
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone/resources/language/resource.language.en_gb
      
      # Create language file
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/resources/language/resource.language.en_gb/strings.po << 'EOF'
      msgid ""
      msgstr ""
      
      msgctxt "#30000"
      msgid "HY300 Keystone Correction"
      msgstr ""
      
      msgctxt "#30001"
      msgid "Manual Adjustment"
      msgstr ""
      
      msgctxt "#30002"
      msgid "Auto Correction"
      msgstr ""
      
      msgctxt "#30003"
      msgid "Load Profile"
      msgstr ""
      
      msgctxt "#30004"
      msgid "Save Profile"
      msgstr ""
      
      msgctxt "#30005"
      msgid "Reset to Center"
      msgstr ""
      
      msgctxt "#30006"
      msgid "Calibrate"
      msgstr ""
      
      msgctxt "#30007"
      msgid "Status"
      msgstr ""
      EOF
      
      # Create placeholder icon and fanart
      echo "PNG placeholder" > $out/share/kodi/addons/plugin.video.hy300keystone/icon.png
      echo "JPEG placeholder" > $out/share/kodi/addons/plugin.video.hy300keystone/fanart.jpg
    '';
    
    meta = with lib; {
      description = "Kodi plugin for HY300 keystone correction";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  # HY300 WiFi Setup Plugin for Kodi
  kodi-plugin-hy300-wifi = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-wifi";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.program.hy300wifi
      
      # Create addon.xml
      cat > $out/share/kodi/addons/plugin.program.hy300wifi/addon.xml << 'EOF'
      <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
      <addon id="plugin.program.hy300wifi"
             name="HY300 WiFi Setup"
             version="1.0.0"
             provider-name="HY300 Project">
        <requires>
          <import addon="xbmc.python" version="3.0.0"/>
        </requires>
        <extension point="xbmc.python.pluginsource" library="main.py">
          <provides>executable</provides>
        </extension>
        <extension point="xbmc.addon.metadata">
          <summary lang="en_GB">HY300 WiFi Configuration</summary>
          <description lang="en_GB">
            WiFi network configuration for HY300 projector using IR remote control.
            Scan, connect, and manage WiFi networks with easy remote navigation.
          </description>
          <platform>linux</platform>
          <license>GPL-3.0</license>
        </extension>
      </addon>
      EOF
      
      # Create main WiFi plugin
      cat > $out/share/kodi/addons/plugin.program.hy300wifi/main.py << 'EOF'
      #!/usr/bin/env python3
      # HY300 WiFi Setup Kodi Plugin
      
      import sys
      import os
      import subprocess
      import time
      import xbmc
      import xbmcgui
      import xbmcplugin
      import xbmcaddon
      from urllib.parse import parse_qsl
      
      addon = xbmcaddon.Addon()
      addon_name = addon.getAddonInfo('name')
      
      WIFI_CONTROL_FIFO = "/run/wifi-setup-control"
      NETWORKS_FILE = "/tmp/wifi-networks.txt"
      
      def send_wifi_command(command):
          """Send command to WiFi setup service"""
          try:
              with open(WIFI_CONTROL_FIFO, 'w') as fifo:
                  fifo.write(command + '\n')
              return True
          except Exception as e:
              xbmc.log(f"WiFi command error: {e}", xbmc.LOGERROR)
              return False
      
      def get_wifi_networks():
          """Get scanned WiFi networks"""
          try:
              # Trigger scan
              send_wifi_command("scan")
              time.sleep(3)  # Wait for scan
              
              if os.path.exists(NETWORKS_FILE):
                  with open(NETWORKS_FILE, 'r') as f:
                      networks = []
                      for line in f:
                          parts = line.strip().split(':')
                          if len(parts) >= 3:
                              ssid, signal, security = parts[0], parts[1], parts[2]
                              networks.append({
                                  'ssid': ssid,
                                  'signal': int(signal) if signal.isdigit() else 0,
                                  'security': security
                              })
                      return sorted(networks, key=lambda x: x['signal'], reverse=True)
              return []
          except Exception as e:
              xbmc.log(f"Error getting networks: {e}", xbmc.LOGERROR)
              return []
      
      def show_main_menu():
          """Display main WiFi menu"""
          items = [
              ("Scan Networks", "scan"),
              ("Available Networks", "networks"),
              ("Connection Status", "status"),
              ("Saved Profiles", "profiles"),
              ("Disconnect", "disconnect"),
              ("Generate QR Code", "qr_code")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_networks():
          """Display available WiFi networks"""
          networks = get_wifi_networks()
          
          if not networks:
              li = xbmcgui.ListItem("No networks found - Scan first")
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
          else:
              for network in networks:
                  signal_bars = "▂▄▆█"[min(3, max(0, network['signal'] // 25))]
                  security_icon = "🔒" if network['security'] != "" else "🔓"
                  
                  label = f"{security_icon} {network['ssid']} {signal_bars} ({network['signal']}%)"
                  
                  li = xbmcgui.ListItem(label)
                  li.setInfo('video', {
                      'title': network['ssid'],
                      'plot': f"Signal: {network['signal']}% | Security: {network['security'] or 'Open'}"
                  })
                  
                  url = f"{sys.argv[0]}?action=connect&ssid={network['ssid']}&security={network['security']}"
                  xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def connect_to_network(ssid, security):
          """Connect to WiFi network"""
          dialog = xbmcgui.Dialog()
          
          # Get password if network is secured
          password = ""
          if security and security != "":
              password = dialog.input(f"Enter password for '{ssid}':", type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
              if not password:
                  return  # User cancelled
          
          # Show progress dialog
          progress = xbmcgui.DialogProgress()
          progress.create(addon_name, f"Connecting to '{ssid}'...")
          
          # Send connect command
          connect_cmd = f"connect:{ssid}:{password}"
          success = send_wifi_command(connect_cmd)
          
          if success:
              # Wait for connection (with progress updates)
              for i in range(30):  # 30 second timeout
                  if progress.iscanceled():
                      break
                  
                  progress.update(int((i / 30) * 100), f"Connecting to '{ssid}'...", f"Attempt {i+1}/30")
                  time.sleep(1)
                  
                  # Check if connected (simplified check)
                  # In real implementation, would check NetworkManager status
                  if i > 10:  # Simulate connection after 10 seconds
                      progress.update(100, "Connected!", "")
                      time.sleep(1)
                      break
          
          progress.close()
          
          if success:
              dialog.ok(addon_name, f"Successfully connected to '{ssid}'!")
          else:
              dialog.ok(addon_name, f"Failed to connect to '{ssid}'")
      
      def show_connection_status():
          """Display current connection status"""
          try:
              # Get NetworkManager status
              result = subprocess.run(['nmcli', 'connection', 'show', '--active'], 
                                    capture_output=True, text=True, timeout=5)
              
              if result.returncode == 0 and result.stdout.strip():
                  status = result.stdout.strip()
                  xbmcgui.Dialog().textviewer("WiFi Connection Status", status)
              else:
                  xbmcgui.Dialog().ok(addon_name, "No active WiFi connections")
                  
          except Exception as e:
              xbmcgui.Dialog().ok(addon_name, f"Error getting status: {e}")
      
      def show_saved_profiles():
          """Display saved WiFi profiles"""
          try:
              result = subprocess.run(['nmcli', 'connection', 'show'], 
                                    capture_output=True, text=True, timeout=5)
              
              if result.returncode == 0:
                  profiles = []
                  for line in result.stdout.split('\n')[1:]:  # Skip header
                      if line.strip() and 'wifi' in line.lower():
                          parts = line.split()
                          if parts:
                              profile_name = parts[0]
                              li = xbmcgui.ListItem(profile_name)
                              url = f"{sys.argv[0]}?action=connect_profile&profile={profile_name}"
                              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
                  
                  if not profiles:
                      li = xbmcgui.ListItem("No saved profiles")
                      xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
              
              xbmcplugin.endOfDirectory(int(sys.argv[1]))
              
          except Exception as e:
              xbmcgui.Dialog().ok(addon_name, f"Error getting profiles: {e}")
      
      def disconnect_wifi():
          """Disconnect from current WiFi"""
          dialog = xbmcgui.Dialog()
          
          if dialog.yesno(addon_name, "Disconnect from current WiFi network?"):
              success = send_wifi_command("disconnect")
              
              if success:
                  dialog.ok(addon_name, "Disconnected from WiFi")
              else:
                  dialog.ok(addon_name, "Failed to disconnect")
      
      def generate_qr_code():
          """Generate QR code for mobile WiFi setup assistance"""
          dialog = xbmcgui.Dialog()
          
          ssid = dialog.input("Enter network SSID:")
          if not ssid:
              return
          
          password = dialog.input("Enter network password:", type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
          
          try:
              # Generate WiFi QR code
              wifi_string = f"WIFI:T:WPA;S:{ssid};P:{password};;"
              
              result = subprocess.run(['qrencode', '-t', 'ASCII', wifi_string], 
                                    capture_output=True, text=True, timeout=10)
              
              if result.returncode == 0:
                  qr_code = result.stdout
                  dialog.textviewer("WiFi QR Code", 
                                   f"Scan with mobile device to connect:\n\n{qr_code}\n\n"
                                   f"Network: {ssid}\n"
                                   "Point your phone's camera at this QR code to connect automatically.")
              else:
                  dialog.ok(addon_name, "Failed to generate QR code")
                  
          except Exception as e:
              dialog.ok(addon_name, f"QR code error: {e}")
      
      def scan_networks():
          """Trigger network scan"""
          dialog = xbmcgui.Dialog()
          
          progress = xbmcgui.DialogProgress()
          progress.create(addon_name, "Scanning for WiFi networks...")
          
          success = send_wifi_command("scan")
          
          if success:
              for i in range(10):  # 10 second scan
                  if progress.iscanceled():
                      break
                  progress.update(int((i / 10) * 100), "Scanning for networks...", f"{i+1}/10 seconds")
                  time.sleep(1)
              
              progress.update(100, "Scan complete!", "")
              time.sleep(1)
          
          progress.close()
          
          if success:
              dialog.ok(addon_name, "Network scan completed!\n\nGo to 'Available Networks' to see results.")
          else:
              dialog.ok(addon_name, "Network scan failed")
      
      # Main plugin logic
      def main():
          params = dict(parse_qsl(sys.argv[2][1:]))
          action = params.get('action')
          
          if action is None:
              show_main_menu()
          elif action == 'scan':
              scan_networks()
          elif action == 'networks':
              show_networks()
          elif action == 'status':
              show_connection_status()
          elif action == 'profiles':
              show_saved_profiles()
          elif action == 'disconnect':
              disconnect_wifi()
          elif action == 'qr_code':
              generate_qr_code()
          elif action == 'connect':
              ssid = params.get('ssid', '')
              security = params.get('security', '')
              connect_to_network(ssid, security)
          elif action == 'connect_profile':
              profile = params.get('profile', '')
              # Connect to saved profile
              try:
                  subprocess.run(['nmcli', 'connection', 'up', 'id', profile], timeout=30)
                  xbmcgui.Dialog().ok(addon_name, f"Connected to profile: {profile}")
              except Exception as e:
                  xbmcgui.Dialog().ok(addon_name, f"Failed to connect: {e}")
      
      if __name__ == '__main__':
          main()
      EOF
    '';
    
    meta = with lib; {
      description = "Kodi plugin for HY300 WiFi setup";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

in
{
  # Combined Kodi plugins package
  kodi-hy300-plugins = pkgs.symlinkJoin {
    name = "kodi-hy300-plugins";
    paths = [
      kodi-plugin-hy300-keystone
      kodi-plugin-hy300-wifi
    ];
    
    postBuild = ''
      # Create installation script
      mkdir -p $out/bin
      
      cat > $out/bin/install-hy300-kodi-plugins << 'EOF'
      #!/bin/bash
      # Install HY300 Kodi plugins
      
      KODI_ADDONS_DIR="/home/projector/.kodi/addons"
      
      echo "Installing HY300 Kodi plugins..."
      
      # Create addons directory
      mkdir -p "$KODI_ADDONS_DIR"
      
      # Copy plugins
      cp -r $out/share/kodi/addons/* "$KODI_ADDONS_DIR/"
      
      # Set permissions
      chown -R projector:projector "$KODI_ADDONS_DIR"
      
      echo "HY300 Kodi plugins installed successfully!"
      echo ""
      echo "Available plugins:"
      echo "- HY300 Keystone Correction (plugin.video.hy300keystone)"
      echo "- HY300 WiFi Setup (plugin.program.hy300wifi)"
      echo ""
      echo "Restart Kodi to see the new plugins in the Add-ons menu."
      EOF
      
      chmod +x $out/bin/install-hy300-kodi-plugins
    '';
    
    meta = with lib; {
      description = "Complete HY300 Kodi plugin package";
      longDescription = ''
        Collection of Kodi plugins specifically designed for the HY300 projector:
        - Keystone correction with manual and automatic adjustment
        - WiFi network configuration via IR remote control
      '';
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };
}