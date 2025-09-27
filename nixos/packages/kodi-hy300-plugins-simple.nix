# Simple HY300 Kodi Plugins - Minimal working version to unblock VM testing
{ lib, pkgs, ... }:

let
  # Simple placeholder plugins without complex Python code
  kodi-plugin-hy300-keystone-simple = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-keystone-simple";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone
      
      # Create simple addon.xml
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/addon.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<addon id="plugin.video.hy300keystone"
       name="HY300 Keystone Correction"
       version="1.0.0"
       provider-name="HY300 Project">
    <requires>
      <import addon="xbmc.python" version="3.0.0"/>
    </requires>
    <extension point="xbmc.python.pluginsource" library="main.py">
      <provides>executable</provides>
    </extension>
    <extension point="xbmc.addon.metadata">
      <summary lang="en_GB">HY300 Projector Keystone Correction</summary>
      <description lang="en_GB">Simple keystone correction interface for HY300 projector in simulation mode</description>
      <platform>all</platform>
      <license>GPL-3.0-only</license>
    </extension>
</addon>
EOF

      # Create minimal main.py with no single quotes in params
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/main.py << 'EOF'
#!/usr/bin/env python3
import sys
import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon
from urllib.parse import parse_qsl

addon = xbmcaddon.Addon()
addon_handle = int(sys.argv[1])

def show_menu():
    li = xbmcgui.ListItem("Keystone Correction (Simulation Mode)")
    xbmcplugin.addDirectoryItem(handle=addon_handle, url="", listitem=li)
    xbmcplugin.endOfDirectory(addon_handle)

def main():
    show_menu()

if __name__ == "__main__":
    main()
EOF
    '';
    
    meta = with lib; {
      description = "Simple HY300 keystone correction plugin";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  kodi-plugin-hy300-wifi-simple = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-wifi-simple";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.program.hy300wifi
      
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
      <description lang="en_GB">Simple WiFi setup interface for HY300 projector in simulation mode</description>
      <platform>all</platform>
      <license>GPL-3.0-only</license>
    </extension>
</addon>
EOF

      cat > $out/share/kodi/addons/plugin.program.hy300wifi/main.py << 'EOF'
#!/usr/bin/env python3
import sys
import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon

addon = xbmcaddon.Addon()
addon_handle = int(sys.argv[1])

def show_menu():
    li = xbmcgui.ListItem("WiFi Setup (Simulation Mode)")
    xbmcplugin.addDirectoryItem(handle=addon_handle, url="", listitem=li)
    xbmcplugin.endOfDirectory(addon_handle)

def main():
    show_menu()

if __name__ == "__main__":
    main()
EOF
    '';
    
    meta = with lib; {
      description = "Simple HY300 WiFi setup plugin";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

in {
  # Main package combining all plugins
  hy300-kodi-plugins = pkgs.symlinkJoin {
    name = "hy300-kodi-plugins";
    paths = [
      kodi-plugin-hy300-keystone-simple
      kodi-plugin-hy300-wifi-simple
    ];
    
    meta = with lib; {
      description = "Simple HY300 hardware-specific Kodi plugins for VM testing";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  # Complete package with configuration scripts
  kodi-hy300-complete = pkgs.stdenv.mkDerivation {
    pname = "kodi-hy300-complete";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/bin $out/share/hy300/kodi
      
      # Create simple installation script
      cat > $out/bin/install-hy300-kodi-complete << 'EOF'
#!/bin/bash
echo "HY300 Kodi installation complete - simulation mode"
mkdir -p /home/hy300/.kodi/userdata
mkdir -p /home/hy300/.kodi/addons
echo "Installation complete"
EOF
      chmod +x $out/bin/install-hy300-kodi-complete
      
      # Create simple kodi launcher
      cat > $out/bin/hy300-kodi << 'EOF'
#!/bin/bash
echo "Starting HY300 Kodi in simulation mode..."
export DISPLAY=:0
exec kodi --standalone --windowed
EOF
      chmod +x $out/bin/hy300-kodi
      
      # Create minimal kodi configuration
      cat > $out/share/hy300/kodi/guisettings.xml << 'EOF'
<settings version="2">
  <setting id="system.movieinfolanguage" default="true">en</setting>
</settings>
EOF
    '';
    
    meta = with lib; {
      description = "Complete HY300 Kodi configuration package (simulation mode)";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  # Individual exports
  kodi-plugin-hy300-keystone = kodi-plugin-hy300-keystone-simple;
  kodi-plugin-hy300-wifi = kodi-plugin-hy300-wifi-simple;
}