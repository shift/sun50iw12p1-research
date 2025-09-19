# HY300 Services Configuration Module
# System services for HY300 projector functionality
#
# This module handles:
# - Kodi media center service
# - Keystone correction service  
# - IR remote control service
# - WiFi management service
# - Bluetooth audio service

{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.hy300-services;
  
  # Custom Kodi configuration for HY300
  kodiConfig = pkgs.writeText "kodi-hy300.xml" ''
    <settings version="1">
      <section id="system">
        <category id="display">
          <setting id="videoscreen.resolution" default="true">19</setting>
          <setting id="videoscreen.screen">0</setting>
          <setting id="videoscreen.whitelist">1920x1080@60.00</setting>
        </category>
        <category id="audio">
          <setting id="audiooutput.audiodevice">HDMI</setting>
          <setting id="audiooutput.channels">2</setting>
        </category>
        <category id="input">
          <setting id="input.enableremote">true</setting>
          <setting id="input.remoteport">9777</setting>
        </category>
      </section>
      <section id="videoplayer">
        <category id="processing">
          <setting id="videoplayer.useamcodec">true</setting>
          <setting id="videoplayer.usevdpau">false</setting>
          <setting id="videoplayer.usevaapi">true</setting>
        </category>
      </section>
    </settings>
  '';
  
  # Keystone correction service
  keystoneService = pkgs.writeShellScript "keystone-service" ''
    #!/bin/bash
    
    # Keystone correction daemon for HY300 projector
    # Handles automatic correction via accelerometer and manual adjustments
    
    DEVICE="/dev/keystone-motor"
    ACCEL_DEVICE="/dev/iio:device0"
    CONFIG_FILE="/etc/hy300/keystone.conf"
    
    # Default keystone parameters
    AUTO_CORRECTION=true
    MANUAL_MODE=false
    CORRECTION_INTERVAL=5
    
    # Load configuration if exists
    if [ -f "$CONFIG_FILE" ]; then
      source "$CONFIG_FILE"
    fi
    
    # Function to read accelerometer data
    read_accelerometer() {
      if [ -f "$ACCEL_DEVICE/in_accel_x_raw" ]; then
        ACCEL_X=$(cat "$ACCEL_DEVICE/in_accel_x_raw")
        ACCEL_Y=$(cat "$ACCEL_DEVICE/in_accel_y_raw")
        ACCEL_Z=$(cat "$ACCEL_DEVICE/in_accel_z_raw")
        
        # Calculate tilt angle (simplified)
        # Real implementation would need proper trigonometric calculation
        TILT_X=$((ACCEL_X / 100))
        TILT_Y=$((ACCEL_Y / 100))
        
        echo "Accelerometer: X=$ACCEL_X Y=$ACCEL_Y Z=$ACCEL_Z"
        echo "Calculated tilt: X=$TILT_X Y=$TILT_Y"
      fi
    }
    
    # Function to apply keystone correction
    apply_keystone_correction() {
      local mode=$1
      local value=$2
      
      if [ -e "$DEVICE" ]; then
        case $mode in
          "auto")
            # Automatic correction based on accelerometer
            read_accelerometer
            if [ -n "$TILT_Y" ]; then
              echo "auto:$TILT_Y" > "$DEVICE"
              echo "Applied automatic keystone correction: $TILT_Y"
            fi
            ;;
          "manual")
            # Manual correction value
            echo "manual:$value" > "$DEVICE"
            echo "Applied manual keystone correction: $value"
            ;;
          "reset")
            # Reset to center position
            echo "reset" > "$DEVICE"
            echo "Reset keystone to center position"
            ;;
        esac
      else
        echo "Warning: Keystone motor device not found"
      fi
    }
    
    # Function to handle MIPS digital correction
    apply_digital_correction() {
      local corners=$1
      
      if [ -e "/dev/mips-loader" ]; then
        # Send 4-corner correction coordinates to MIPS processor
        echo "corners:$corners" > /sys/class/mips-loader/mips0/keystone
        echo "Applied digital keystone correction: $corners"
      else
        echo "Warning: MIPS co-processor not available for digital correction"
      fi
    }
    
    # Signal handlers
    handle_sigterm() {
      echo "Keystone service shutting down..."
      exit 0
    }
    
    handle_sigusr1() {
      echo "Reloading keystone configuration..."
      if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
        echo "Configuration reloaded"
      fi
    }
    
    trap handle_sigterm TERM
    trap handle_sigusr1 USR1
    
    echo "Starting HY300 keystone correction service..."
    echo "Auto correction: $AUTO_CORRECTION"
    echo "Correction interval: $CORRECTION_INTERVAL seconds"
    
    # Create FIFO for external control
    mkfifo /run/keystone-control 2>/dev/null || true
    
    # Main service loop
    while true; do
      # Check for external commands
      if read -t 1 command < /run/keystone-control 2>/dev/null; then
        case $command in
          auto:*)
            AUTO_CORRECTION=true
            echo "Enabled automatic correction"
            ;;
          manual:*)
            AUTO_CORRECTION=false
            value=$(echo $command | cut -d: -f2)
            apply_keystone_correction "manual" "$value"
            ;;
          digital:*)
            corners=$(echo $command | cut -d: -f2)
            apply_digital_correction "$corners"
            ;;
          reset)
            apply_keystone_correction "reset"
            ;;
        esac
      fi
      
      # Automatic correction if enabled
      if [ "$AUTO_CORRECTION" = "true" ] && [ "$MANUAL_MODE" = "false" ]; then
        apply_keystone_correction "auto"
      fi
      
      sleep $CORRECTION_INTERVAL
    done
  '';
  
  # WiFi setup service for IR remote control
  wifiSetupService = pkgs.writeShellScript "wifi-setup-service" ''
    #!/bin/bash
    
    # WiFi setup service for HY300 projector
    # Provides IR remote-controlled WiFi configuration interface
    
    NETWORKS_FILE="/tmp/wifi-networks.txt"
    CONFIG_FILE="/etc/hy300/wifi-setup.conf"
    CONTROL_FIFO="/run/wifi-setup-control"
    
    # Create control FIFO
    mkfifo "$CONTROL_FIFO" 2>/dev/null || true
    
    # Function to scan for WiFi networks
    scan_networks() {
      echo "Scanning for WiFi networks..."
      nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list > "$NETWORKS_FILE"
      
      # Sort by signal strength
      sort -t: -k2 -nr "$NETWORKS_FILE" > "${NETWORKS_FILE}.sorted"
      mv "${NETWORKS_FILE}.sorted" "$NETWORKS_FILE"
      
      echo "Found $(wc -l < "$NETWORKS_FILE") networks"
    }
    
    # Function to connect to WiFi network
    connect_network() {
      local ssid=$1
      local password=$2
      
      echo "Connecting to network: $ssid"
      
      if [ -n "$password" ]; then
        nmcli dev wifi connect "$ssid" password "$password"
      else
        nmcli dev wifi connect "$ssid"
      fi
      
      if [ $? -eq 0 ]; then
        echo "Successfully connected to $ssid"
        # Save profile for future use
        echo "LAST_SSID=$ssid" > "$CONFIG_FILE"
      else
        echo "Failed to connect to $ssid"
      fi
    }
    
    # Function to generate setup QR code
    generate_qr_code() {
      local ssid=$1
      local password=$2
      
      # Generate WiFi QR code for mobile assistance
      qr_data="WIFI:T:WPA;S:$ssid;P:$password;;"
      echo "$qr_data" | qrencode -t ASCII
    }
    
    # Function to handle IR remote commands
    handle_remote_command() {
      local command=$1
      
      case $command in
        "scan")
          scan_networks
          ;;
        "connect:"*)
          ssid=$(echo $command | cut -d: -f2)
          password=$(echo $command | cut -d: -f3)
          connect_network "$ssid" "$password"
          ;;
        "status")
          nmcli connection show --active
          ;;
        "disconnect")
          nmcli connection down id "$(nmcli -t -f NAME connection show --active | head -1)"
          ;;
        "profiles")
          nmcli connection show
          ;;
      esac
    }
    
    echo "Starting WiFi setup service..."
    
    # Initial network scan
    scan_networks
    
    # Main service loop
    while true; do
      if read -t 5 command < "$CONTROL_FIFO" 2>/dev/null; then
        handle_remote_command "$command"
      fi
      
      # Periodic network monitoring
      if ! nmcli connection show --active | grep -q wifi; then
        echo "No active WiFi connection"
        
        # Auto-reconnect to last known network if available
        if [ -f "$CONFIG_FILE" ]; then
          source "$CONFIG_FILE"
          if [ -n "$LAST_SSID" ]; then
            echo "Attempting to reconnect to $LAST_SSID"
            nmcli connection up id "$LAST_SSID" 2>/dev/null || true
          fi
        fi
      fi
      
      sleep 30
    done
  '';

in
{
  options.services.hy300-services = {
    enable = mkEnableOption "HY300 projector services";

    mediaCenter = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable Kodi media center service";
      };

      autoStart = mkOption {
        type = types.bool;
        default = true;
        description = "Auto-start Kodi on boot";
      };

      hardwareAcceleration = mkOption {
        type = types.bool;
        default = true;
        description = "Enable hardware acceleration";
      };

      customPlugins = mkOption {
        type = types.bool;
        default = true;
        description = "Install HY300-specific plugins";
      };

      audioOutput = mkOption {
        type = types.enum [ "hdmi" "bluetooth" "auto" ];
        default = "auto";
        description = "Audio output configuration";
      };
    };

    keystoneCorrection = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable keystone correction service";
      };

      autoCorrection = mkOption {
        type = types.bool;
        default = true;
        description = "Enable automatic correction";
      };

      manualInterface = mkOption {
        type = types.bool;
        default = true;
        description = "Enable manual adjustment interface";
      };

      motorControl = mkOption {
        type = types.bool;
        default = true;
        description = "Enable physical motor control";
      };
    };

    remoteControl = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable IR remote control service";
      };

      protocol = mkOption {
        type = types.str;
        default = "hy300-remote";
        description = "IR remote protocol";
      };

      wifiSetup = mkOption {
        type = types.bool;
        default = true;
        description = "Enable WiFi setup via remote";
      };
    };

    network = {
      wifi = {
        enable = mkOption {
          type = types.bool;
          default = true;
          description = "Enable WiFi management service";
        };

        remoteSetup = mkOption {
          type = types.bool;
          default = true;
          description = "Enable remote WiFi setup";
        };

        profileManagement = mkOption {
          type = types.bool;
          default = true;
          description = "Enable profile management";
        };
      };

      bluetooth = {
        enable = mkOption {
          type = types.bool;
          default = true;
          description = "Enable Bluetooth service";
        };

        audioDevices = mkOption {
          type = types.bool;
          default = true;
          description = "Enable audio device support";
        };

        hidDevices = mkOption {
          type = types.bool;
          default = true;
          description = "Enable HID device support";
        };

        autoReconnect = mkOption {
          type = types.bool;
          default = true;
          description = "Enable auto-reconnection";
        };
      };
    };
  };

  config = mkIf cfg.enable {
    # Systemd services configuration
    systemd.services = {
      # Kodi media center service
      kodi-hy300 = mkIf cfg.mediaCenter.enable {
        description = "Kodi Media Center for HY300 Projector";
        wantedBy = mkIf cfg.mediaCenter.autoStart [ "graphical-session.target" ];
        after = [ "graphical-session.target" "network.target" ];
        
        environment = {
          DISPLAY = ":0";
          HOME = "/home/projector";
          XDG_RUNTIME_DIR = "/run/user/1000";
        } // optionalAttrs cfg.mediaCenter.hardwareAcceleration {
          MALI_SHARED_MEM_SIZE = "256M";
          LIBVA_DRIVER_NAME = "mali";
        };
        
        serviceConfig = {
          Type = "simple";
          User = "projector";
          Group = "video";
          Restart = "always";
          RestartSec = "10s";
          
          ExecStart = "${pkgs.kodi}/bin/kodi --standalone --no-splash";
          ExecStop = "${pkgs.coreutils}/bin/pkill -f kodi";
          
          # Security settings
          NoNewPrivileges = true;
          PrivateTmp = true;
          
          # Hardware access
          DeviceAllow = [
            "/dev/dri rw"
            "/dev/video0 rw"
            "/dev/input rw"
            "/dev/lirc0 rw"
          ];
        };
        
        preStart = ''
          # Create Kodi configuration directory
          mkdir -p /home/projector/.kodi/userdata
          
          # Copy HY300-specific configuration
          cp ${kodiConfig} /home/projector/.kodi/userdata/advancedsettings.xml
          
          # Set ownership
          chown -R projector:projector /home/projector/.kodi
        '';
      };

      # Keystone correction service
      keystone-correction = mkIf cfg.keystoneCorrection.enable {
        description = "HY300 Keystone Correction Service";
        wantedBy = [ "multi-user.target" ];
        after = [ "mips-coprocessor-init.service" "keystone-motor-init.service" ];
        
        serviceConfig = {
          Type = "simple";
          User = "root";
          Restart = "always";
          RestartSec = "15s";
          
          ExecStart = keystoneService;
          ExecReload = "${pkgs.coreutils}/bin/kill -USR1 $MAINPID";
          
          # Create runtime directory
          RuntimeDirectory = "keystone";
          RuntimeDirectoryMode = "0755";
        };
        
        environment = {
          AUTO_CORRECTION = if cfg.keystoneCorrection.autoCorrection then "true" else "false";
          MOTOR_CONTROL = if cfg.keystoneCorrection.motorControl then "true" else "false";
        };
      };

      # IR remote control service
      ir-remote-control = mkIf cfg.remoteControl.enable {
        description = "HY300 IR Remote Control Service";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        
        serviceConfig = {
          Type = "simple";
          Restart = "always";
          RestartSec = "10s";
          
          ExecStart = "${pkgs.writeShellScript "ir-remote-service" ''
            #!/bin/bash
            
            # LIRC daemon for HY300 remote control
            LIRC_DEVICE="/dev/lirc0"
            REMOTE_CONFIG="/etc/lirc/lircd.conf.d/hy300-remote.conf"
            
            # Wait for IR device
            timeout=10
            while [ $timeout -gt 0 ] && [ ! -e "$LIRC_DEVICE" ]; do
              sleep 1
              timeout=$((timeout - 1))
            done
            
            if [ ! -e "$LIRC_DEVICE" ]; then
              echo "Error: IR device not found"
              exit 1
            fi
            
            echo "Starting LIRC daemon for HY300 remote..."
            ${pkgs.lirc}/bin/lircd --nodaemon --device="$LIRC_DEVICE" "$REMOTE_CONFIG"
          ''}";
        };
      };

      # WiFi setup service
      wifi-setup = mkIf (cfg.network.wifi.enable && cfg.network.wifi.remoteSetup) {
        description = "HY300 WiFi Setup Service";
        wantedBy = [ "multi-user.target" ];
        after = [ "network.target" "ir-remote-control.service" ];
        
        serviceConfig = {
          Type = "simple";
          Restart = "always";
          RestartSec = "20s";
          
          ExecStart = wifiSetupService;
          
          # Network access required
          PrivateNetwork = false;
        };
      };

      # Bluetooth audio service
      bluetooth-audio = mkIf (cfg.network.bluetooth.enable && cfg.network.bluetooth.audioDevices) {
        description = "HY300 Bluetooth Audio Service";
        wantedBy = [ "multi-user.target" ];
        after = [ "bluetooth.service" ];
        
        serviceConfig = {
          Type = "simple";
          Restart = "always";
          RestartSec = "15s";
          
          ExecStart = "${pkgs.writeShellScript "bluetooth-audio-service" ''
            #!/bin/bash
            
            # Bluetooth audio management for HY300
            
            # Function to handle A2DP devices
            handle_a2dp_device() {
              local device=$1
              local action=$2
              
              case $action in
                "connect")
                  bluetoothctl connect "$device"
                  if [ $? -eq 0 ]; then
                    echo "Connected to A2DP device: $device"
                    # Switch audio output to Bluetooth
                    pactl set-default-sink "bluez_sink.$device.a2dp_sink"
                  fi
                  ;;
                "disconnect")
                  bluetoothctl disconnect "$device"
                  echo "Disconnected A2DP device: $device"
                  # Switch back to HDMI audio
                  pactl set-default-sink "alsa_output.platform-hdmi-sound.stereo-fallback"
                  ;;
              esac
            }
            
            # Main service loop
            echo "Starting Bluetooth audio service..."
            
            # Enable discoverable mode
            bluetoothctl discoverable on
            bluetoothctl pairable on
            
            # Monitor for device connections
            bluetoothctl | while read line; do
              if echo "$line" | grep -q "Device.*Connected: yes"; then
                device=$(echo "$line" | grep -o '[0-9A-F:]\{17\}')
                echo "Device connected: $device"
                handle_a2dp_device "$device" "connect"
              elif echo "$line" | grep -q "Device.*Connected: no"; then
                device=$(echo "$line" | grep -o '[0-9A-F:]\{17\}')
                echo "Device disconnected: $device"
                handle_a2dp_device "$device" "disconnect"
              fi
            done
          ''}";
        };
      };
    };

    # Configuration files
    environment.etc = {
      # Keystone correction configuration
      "hy300/keystone.conf" = mkIf cfg.keystoneCorrection.enable {
        text = ''
          # HY300 Keystone Correction Configuration
          AUTO_CORRECTION=${if cfg.keystoneCorrection.autoCorrection then "true" else "false"}
          MANUAL_MODE=false
          CORRECTION_INTERVAL=5
          
          # Calibration values (to be determined during setup)
          ACCEL_X_OFFSET=0
          ACCEL_Y_OFFSET=0
          ACCEL_Z_OFFSET=0
          
          # Motor control parameters
          MOTOR_STEPS_PER_DEGREE=10
          MOTOR_MAX_POSITION=100
          MOTOR_MIN_POSITION=-100
        '';
      };

      # LIRC remote control configuration
      "lirc/lircd.conf.d/hy300-remote.conf" = mkIf cfg.remoteControl.enable {
        text = ''
          # HY300 Remote Control Configuration
          # Protocol: RC-5 (typical for projector remotes)
          
          begin remote
            name  hy300-remote
            bits           13
            flags RC5|CONST_LENGTH
            eps            30
            aeps          100
            
            header       889   889
            one          889   889
            zero         889   889
            plead        889
            gap          113792
            toggle_bit_mask 0x800
            
            begin codes
              POWER        0x100C
              MENU         0x1029
              UP           0x1020
              DOWN         0x1021
              LEFT         0x1011
              RIGHT        0x1010
              OK           0x1057
              BACK         0x1022
              HOME         0x101E
              
              # Volume controls
              VOL_UP       0x1010
              VOL_DOWN     0x1011
              MUTE         0x100D
              
              # Media controls
              PLAY         0x1035
              PAUSE        0x1030
              STOP         0x1036
              
              # Numbers
              KEY_1        0x1001
              KEY_2        0x1002
              KEY_3        0x1003
              KEY_4        0x1004
              KEY_5        0x1005
              KEY_6        0x1006
              KEY_7        0x1007
              KEY_8        0x1008
              KEY_9        0x1009
              KEY_0        0x1000
              
              # WiFi setup
              WIFI         0x1040
              
              # Keystone
              KEYSTONE_UP  0x1041
              KEYSTONE_DOWN 0x1042
            end codes
          end remote
        '';
      };

      # WiFi setup configuration
      "hy300/wifi-setup.conf" = mkIf (cfg.network.wifi.enable && cfg.network.wifi.remoteSetup) {
        text = ''
          # HY300 WiFi Setup Configuration
          
          # Network scanning parameters
          SCAN_INTERVAL=30
          MAX_NETWORKS=20
          SIGNAL_THRESHOLD=-80
          
          # Connection parameters
          CONNECTION_TIMEOUT=30
          RETRY_COUNT=3
          
          # Profile management
          MAX_PROFILES=10
          AUTO_CONNECT=true
        '';
      };
    };

    # Required packages for services
    environment.systemPackages = with pkgs; [
      # Media center
      kodi
      
      # IR remote control
      lirc
      
      # Network management
      networkmanager
      wpa_supplicant
      
      # Bluetooth
      bluez
      bluez-tools
      
      # Audio
      pulseaudio
      pavucontrol
      alsa-utils
      
      # QR code generation for WiFi setup
      qrencode
      
      # System utilities
      jq
      curl
    ];

    # User groups for service access
    users.groups.hy300 = {};
    
    # Add projector user to service groups
    users.users.projector = mkIf (config.users.users ? projector) {
      extraGroups = [ "hy300" "audio" "video" "input" "bluetooth" "networkmanager" ];
    };

    # Runtime directories
    systemd.tmpfiles.rules = [
      "d /run/hy300 0755 root hy300 -"
      "d /var/lib/hy300 0755 root hy300 -"
      "d /var/log/hy300 0755 root hy300 -"
    ];
  };
}