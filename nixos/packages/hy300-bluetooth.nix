# HY300 Bluetooth Audio Management Package
# Complete Bluetooth audio device support and management

{ lib, pkgs, writeShellScript, ... }:

pkgs.stdenv.mkDerivation {
  pname = "hy300-bluetooth-audio";
  version = "1.0.0";
  
  src = ./.;
  
  nativeBuildInputs = with pkgs; [
    makeWrapper
  ];
  
  buildInputs = with pkgs; [
    bash
    bluez
    bluez-tools
    pulseaudio
    coreutils
  ];
  
  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/lib/systemd/system
    mkdir -p $out/etc/hy300
    mkdir -p $out/share/hy300/bluetooth
    
    # Bluetooth audio management daemon
    cat > $out/bin/hy300-bluetooth-daemon << 'EOF'
    #!/bin/bash
    # HY300 Bluetooth Audio Management Daemon
    
    CONFIG_FILE="/etc/hy300/bluetooth-audio.conf"
    DEVICES_FILE="/var/lib/hy300/bluetooth-devices.conf"
    CONTROL_FIFO="/run/hy300/bluetooth-control"
    
    # Default configuration
    AUTO_RECONNECT=true
    DISCOVERY_TIMEOUT=30
    PAIRING_TIMEOUT=60
    A2DP_ENABLED=true
    HID_ENABLED=true
    
    # Load configuration
    if [ -f "$CONFIG_FILE" ]; then
      source "$CONFIG_FILE"
    fi
    
    # Create control FIFO
    mkdir -p "$(dirname "$CONTROL_FIFO")"
    mkfifo "$CONTROL_FIFO" 2>/dev/null || true
    
    # Create devices database
    mkdir -p "$(dirname "$DEVICES_FILE")"
    touch "$DEVICES_FILE"
    
    # Function to log with timestamp
    log_message() {
      echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
    }
    
    # Function to check if Bluetooth is ready
    check_bluetooth_ready() {
      bluetoothctl show | grep -q "Powered: yes"
    }
    
    # Function to enable Bluetooth if needed
    ensure_bluetooth_enabled() {
      if ! check_bluetooth_ready; then
        log_message "Enabling Bluetooth..."
        bluetoothctl power on
        sleep 2
        
        if check_bluetooth_ready; then
          log_message "Bluetooth enabled successfully"
          return 0
        else
          log_message "Failed to enable Bluetooth"
          return 1
        fi
      fi
      return 0
    }
    
    # Function to make device discoverable
    make_discoverable() {
      local duration=$1
      
      log_message "Making device discoverable for $duration seconds..."
      bluetoothctl discoverable on
      bluetoothctl pairable on
      
      # Set discovery timeout
      if [ -n "$duration" ] && [ "$duration" -gt 0 ]; then
        (
          sleep "$duration"
          bluetoothctl discoverable off
          log_message "Discovery mode disabled after timeout"
        ) &
      fi
    }
    
    # Function to scan for devices
    scan_devices() {
      local scan_time=${1:-15}
      
      log_message "Scanning for Bluetooth devices for $scan_time seconds..."
      
      # Start scan
      bluetoothctl scan on &
      scan_pid=$!
      
      # Scan for specified time
      sleep "$scan_time"
      
      # Stop scan
      bluetoothctl scan off
      kill $scan_pid 2>/dev/null || true
      
      log_message "Device scan completed"
    }
    
    # Function to get discovered devices
    get_discovered_devices() {
      bluetoothctl devices | while read line; do
        if [[ $line =~ Device\ ([0-9A-F:]{17})\ (.+) ]]; then
          mac="${BASH_REMATCH[1]}"
          name="${BASH_REMATCH[2]}"
          
          # Get device info
          info=$(bluetoothctl info "$mac")
          
          # Check if it's an audio device
          if echo "$info" | grep -q "UUID.*Audio"; then
            echo "AUDIO:$mac:$name"
          elif echo "$info" | grep -q "UUID.*Human Interface Device"; then
            echo "HID:$mac:$name"
          else
            echo "OTHER:$mac:$name"
          fi
        fi
      done
    }
    
    # Function to pair with device
    pair_device() {
      local mac=$1
      local device_name=$2
      
      log_message "Attempting to pair with device: $device_name ($mac)"
      
      # Trust the device first
      bluetoothctl trust "$mac"
      
      # Pair with device
      if bluetoothctl pair "$mac"; then
        log_message "Successfully paired with $device_name"
        
        # Save to devices database
        echo "$mac:$device_name:$(date)" >> "$DEVICES_FILE"
        
        # Try to connect
        connect_device "$mac" "$device_name"
        return 0
      else
        log_message "Failed to pair with $device_name"
        return 1
      fi
    }
    
    # Function to connect to device
    connect_device() {
      local mac=$1
      local device_name=$2
      
      log_message "Connecting to device: $device_name ($mac)"
      
      if bluetoothctl connect "$mac"; then
        log_message "Successfully connected to $device_name"
        
        # Check if it's an audio device and configure audio
        if bluetoothctl info "$mac" | grep -q "UUID.*Audio"; then
          configure_audio_device "$mac" "$device_name"
        fi
        
        return 0
      else
        log_message "Failed to connect to $device_name"
        return 1
      fi
    }
    
    # Function to disconnect device
    disconnect_device() {
      local mac=$1
      local device_name=$2
      
      log_message "Disconnecting from device: $device_name ($mac)"
      
      if bluetoothctl disconnect "$mac"; then
        log_message "Successfully disconnected from $device_name"
        
        # Switch audio back to HDMI if it was an audio device
        if bluetoothctl info "$mac" | grep -q "UUID.*Audio"; then
          switch_to_hdmi_audio
        fi
        
        return 0
      else
        log_message "Failed to disconnect from $device_name"
        return 1
      fi
    }
    
    # Function to configure audio device
    configure_audio_device() {
      local mac=$1
      local device_name=$2
      
      log_message "Configuring audio for device: $device_name"
      
      # Wait for PulseAudio to detect the device
      sleep 3
      
      # Find the Bluetooth sink
      local bt_sink=$(pactl list short sinks | grep "bluez_sink.${mac//:/_}" | awk '{print $2}')
      
      if [ -n "$bt_sink" ]; then
        # Set as default sink
        pactl set-default-sink "$bt_sink"
        log_message "Set $device_name as default audio output"
        
        # Set volume to reasonable level
        pactl set-sink-volume "$bt_sink" 70%
        
        # Move existing audio streams to Bluetooth device
        pactl list short sink-inputs | awk '{print $1}' | while read input; do
          pactl move-sink-input "$input" "$bt_sink" 2>/dev/null || true
        done
        
        log_message "Audio configuration completed for $device_name"
      else
        log_message "Warning: Bluetooth audio sink not found for $device_name"
      fi
    }
    
    # Function to switch back to HDMI audio
    switch_to_hdmi_audio() {
      log_message "Switching audio output back to HDMI"
      
      # Find HDMI audio sink
      local hdmi_sink=$(pactl list short sinks | grep -E "(hdmi|alsa_output)" | head -1 | awk '{print $2}')
      
      if [ -n "$hdmi_sink" ]; then
        pactl set-default-sink "$hdmi_sink"
        log_message "Switched to HDMI audio output: $hdmi_sink"
      else
        log_message "Warning: HDMI audio sink not found"
      fi
    }
    
    # Function to remove device
    remove_device() {
      local mac=$1
      local device_name=$2
      
      log_message "Removing device: $device_name ($mac)"
      
      # Disconnect first
      bluetoothctl disconnect "$mac" 2>/dev/null || true
      
      # Remove pairing
      if bluetoothctl remove "$mac"; then
        log_message "Successfully removed $device_name"
        
        # Remove from devices database
        grep -v "^$mac:" "$DEVICES_FILE" > "$DEVICES_FILE.tmp" 2>/dev/null || true
        mv "$DEVICES_FILE.tmp" "$DEVICES_FILE" 2>/dev/null || true
        
        return 0
      else
        log_message "Failed to remove $device_name"
        return 1
      fi
    }
    
    # Function to auto-reconnect to known devices
    auto_reconnect_devices() {
      if [ "$AUTO_RECONNECT" != "true" ]; then
        return
      fi
      
      log_message "Attempting auto-reconnection to known devices..."
      
      if [ -f "$DEVICES_FILE" ]; then
        while IFS=: read -r mac name last_seen; do
          if [ -n "$mac" ] && [ -n "$name" ]; then
            # Check if device is available
            if bluetoothctl info "$mac" >/dev/null 2>&1; then
              # Check if not already connected
              if ! bluetoothctl info "$mac" | grep -q "Connected: yes"; then
                log_message "Auto-reconnecting to $name..."
                connect_device "$mac" "$name" &
              fi
            fi
          fi
        done < "$DEVICES_FILE"
      fi
    }
    
    # Function to handle external commands
    handle_command() {
      local command=$1
      
      case $command in
        scan:*)
          duration=$(echo $command | cut -d: -f2)
          scan_devices "${duration:-15}"
          ;;
        discoverable:*)
          duration=$(echo $command | cut -d: -f2)
          make_discoverable "${duration:-60}"
          ;;
        pair:*)
          mac=$(echo $command | cut -d: -f2)
          name=$(echo $command | cut -d: -f3)
          pair_device "$mac" "$name"
          ;;
        connect:*)
          mac=$(echo $command | cut -d: -f2)
          name=$(echo $command | cut -d: -f3)
          connect_device "$mac" "$name"
          ;;
        disconnect:*)
          mac=$(echo $command | cut -d: -f2)
          name=$(echo $command | cut -d: -f3)
          disconnect_device "$mac" "$name"
          ;;
        remove:*)
          mac=$(echo $command | cut -d: -f2)
          name=$(echo $command | cut -d: -f3)
          remove_device "$mac" "$name"
          ;;
        audio_hdmi)
          switch_to_hdmi_audio
          ;;
        auto_reconnect)
          auto_reconnect_devices
          ;;
        list_devices)
          get_discovered_devices
          ;;
        status)
          echo "Bluetooth Status:"
          bluetoothctl show
          echo ""
          echo "Connected Devices:"
          bluetoothctl devices Connected
          echo ""
          echo "Audio Status:"
          pactl info | grep "Default Sink"
          ;;
      esac
    }
    
    # Signal handlers
    handle_sigterm() {
      log_message "Bluetooth audio daemon shutting down..."
      bluetoothctl discoverable off
      rm -f "$CONTROL_FIFO"
      exit 0
    }
    
    handle_sigusr1() {
      log_message "Reloading Bluetooth configuration..."
      if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
        log_message "Configuration reloaded"
      fi
    }
    
    trap handle_sigterm TERM INT
    trap handle_sigusr1 USR1
    
    log_message "Starting HY300 Bluetooth audio daemon..."
    
    # Ensure Bluetooth is enabled
    if ! ensure_bluetooth_enabled; then
      log_message "Failed to initialize Bluetooth - exiting"
      exit 1
    fi
    
    log_message "Bluetooth daemon started successfully"
    log_message "Auto-reconnect: $AUTO_RECONNECT"
    log_message "A2DP enabled: $A2DP_ENABLED"
    log_message "HID enabled: $HID_ENABLED"
    
    # Initial auto-reconnect attempt
    auto_reconnect_devices
    
    # Main service loop
    while true; do
      # Check for external commands
      if read -t 30 command < "$CONTROL_FIFO" 2>/dev/null; then
        handle_command "$command"
      fi
      
      # Periodic auto-reconnect check
      auto_reconnect_devices
    done
    EOF
    
    chmod +x $out/bin/hy300-bluetooth-daemon
    
    # Bluetooth control utility
    cat > $out/bin/hy300-bluetooth-control << 'EOF'
    #!/bin/bash
    # HY300 Bluetooth Control Utility
    
    CONTROL_FIFO="/run/hy300/bluetooth-control"
    
    usage() {
      echo "Usage: $0 <command> [options]"
      echo ""
      echo "Commands:"
      echo "  scan [duration]           - Scan for devices (default 15s)"
      echo "  discoverable [duration]   - Make discoverable (default 60s)"
      echo "  pair <mac> <name>        - Pair with device"
      echo "  connect <mac> <name>     - Connect to device"
      echo "  disconnect <mac> <name>  - Disconnect from device"
      echo "  remove <mac> <name>      - Remove device pairing"
      echo "  audio-hdmi               - Switch audio to HDMI"
      echo "  auto-reconnect           - Reconnect to known devices"
      echo "  list                     - List discovered devices"
      echo "  status                   - Show Bluetooth status"
      echo "  devices                  - Show known devices"
    }
    
    if [ $# -eq 0 ]; then
      usage
      exit 1
    fi
    
    command=$1
    shift
    
    case $command in
      scan)
        duration=${1:-15}
        echo "scan:$duration" > "$CONTROL_FIFO"
        echo "Scanning for Bluetooth devices for $duration seconds..."
        ;;
      discoverable)
        duration=${1:-60}
        echo "discoverable:$duration" > "$CONTROL_FIFO"
        echo "Device is now discoverable for $duration seconds"
        ;;
      pair)
        if [ $# -lt 2 ]; then
          echo "Error: pair requires MAC address and device name"
          exit 1
        fi
        mac=$1
        name=$2
        echo "pair:$mac:$name" > "$CONTROL_FIFO"
        echo "Pairing with device: $name ($mac)"
        ;;
      connect)
        if [ $# -lt 2 ]; then
          echo "Error: connect requires MAC address and device name"
          exit 1
        fi
        mac=$1
        name=$2
        echo "connect:$mac:$name" > "$CONTROL_FIFO"
        echo "Connecting to device: $name ($mac)"
        ;;
      disconnect)
        if [ $# -lt 2 ]; then
          echo "Error: disconnect requires MAC address and device name"
          exit 1
        fi
        mac=$1
        name=$2
        echo "disconnect:$mac:$name" > "$CONTROL_FIFO"
        echo "Disconnecting from device: $name ($mac)"
        ;;
      remove)
        if [ $# -lt 2 ]; then
          echo "Error: remove requires MAC address and device name"
          exit 1
        fi
        mac=$1
        name=$2
        echo "remove:$mac:$name" > "$CONTROL_FIFO"
        echo "Removing device: $name ($mac)"
        ;;
      audio-hdmi)
        echo "audio_hdmi" > "$CONTROL_FIFO"
        echo "Switching audio output to HDMI"
        ;;
      auto-reconnect)
        echo "auto_reconnect" > "$CONTROL_FIFO"
        echo "Attempting to reconnect to known devices"
        ;;
      list)
        echo "list_devices" > "$CONTROL_FIFO"
        sleep 1
        echo "Check system logs for discovered devices"
        ;;
      status)
        echo "status" > "$CONTROL_FIFO"
        sleep 1
        echo "Check system logs for status information"
        ;;
      devices)
        echo "Known Bluetooth devices:"
        if [ -f "/var/lib/hy300/bluetooth-devices.conf" ]; then
          cat "/var/lib/hy300/bluetooth-devices.conf" | while IFS=: read mac name last_seen; do
            echo "  $name ($mac) - Last seen: $last_seen"
          done
        else
          echo "  No devices found"
        fi
        ;;
      *)
        echo "Error: Unknown command: $command"
        usage
        exit 1
        ;;
    esac
    EOF
    
    chmod +x $out/bin/hy300-bluetooth-control
    
    # Systemd service
    cat > $out/lib/systemd/system/hy300-bluetooth-audio.service << EOF
    [Unit]
    Description=HY300 Bluetooth Audio Service
    After=bluetooth.service pulseaudio.service
    Wants=bluetooth.service pulseaudio.service
    
    [Service]
    Type=simple
    User=root
    Restart=always
    RestartSec=15s
    
    ExecStart=$out/bin/hy300-bluetooth-daemon
    ExecReload=/bin/kill -USR1 \$MAINPID
    
    # Runtime directory
    RuntimeDirectory=hy300
    RuntimeDirectoryMode=0755
    
    # State directory
    StateDirectory=hy300
    StateDirectoryMode=0755
    
    [Install]
    WantedBy=multi-user.target
    EOF
    
    # Configuration file
    cat > $out/etc/hy300/bluetooth-audio.conf << EOF
    # HY300 Bluetooth Audio Configuration
    
    # Automatic reconnection
    AUTO_RECONNECT=true
    
    # Timeouts (seconds)
    DISCOVERY_TIMEOUT=30
    PAIRING_TIMEOUT=60
    CONNECTION_TIMEOUT=30
    
    # Protocol support
    A2DP_ENABLED=true
    HID_ENABLED=true
    AVRCP_ENABLED=true
    
    # Audio settings
    DEFAULT_VOLUME=70
    AUTO_SWITCH_AUDIO=true
    
    # Security settings
    REQUIRE_CONFIRMATION=false
    AUTO_TRUST=true
    EOF
    
    # Bluetooth pairing helper script
    cat > $out/share/hy300/bluetooth/pair-headphones.sh << 'EOF'
    #!/bin/bash
    # HY300 Bluetooth Headphone Pairing Helper
    
    echo "HY300 Bluetooth Headphone Pairing"
    echo "================================="
    echo ""
    echo "This script will help you pair Bluetooth headphones or speakers."
    echo ""
    
    # Make device discoverable
    echo "Step 1: Making HY300 discoverable..."
    hy300-bluetooth-control discoverable 120
    
    echo ""
    echo "Step 2: Put your headphones/speakers in pairing mode"
    echo "      (Usually hold power button or dedicated pairing button)"
    read -p "Press Enter when your device is in pairing mode..."
    
    echo ""
    echo "Step 3: Scanning for devices..."
    hy300-bluetooth-control scan 20
    
    echo ""
    echo "Available audio devices:"
    bluetoothctl devices | while read line; do
      if [[ $line =~ Device\ ([0-9A-F:]{17})\ (.+) ]]; then
        mac="${BASH_REMATCH[1]}"
        name="${BASH_REMATCH[2]}"
        
        # Check if it's likely an audio device
        if bluetoothctl info "$mac" | grep -q "UUID.*Audio\|Class.*Audio"; then
          echo "  $name ($mac)"
        fi
      fi
    done
    
    echo ""
    read -p "Enter MAC address of device to pair: " mac_address
    read -p "Enter device name (optional): " device_name
    
    if [ -z "$device_name" ]; then
      device_name="Audio Device"
    fi
    
    echo ""
    echo "Pairing with $device_name ($mac_address)..."
    hy300-bluetooth-control pair "$mac_address" "$device_name"
    
    echo ""
    echo "Pairing process completed!"
    echo "Your device should now be connected and set as the default audio output."
    echo ""
    echo "To manage this device in the future:"
    echo "  Connect:    hy300-bluetooth-control connect $mac_address \"$device_name\""
    echo "  Disconnect: hy300-bluetooth-control disconnect $mac_address \"$device_name\""
    echo "  Remove:     hy300-bluetooth-control remove $mac_address \"$device_name\""
    EOF
    
    chmod +x $out/share/hy300/bluetooth/pair-headphones.sh
    
    # Audio device test script
    cat > $out/share/hy300/bluetooth/test-audio.sh << 'EOF'
    #!/bin/bash
    # Test Bluetooth audio functionality
    
    echo "Testing Bluetooth audio..."
    
    # Get current default sink
    current_sink=$(pactl info | grep "Default Sink" | awk '{print $3}')
    echo "Current audio sink: $current_sink"
    
    if echo "$current_sink" | grep -q "bluez"; then
      echo "Bluetooth audio device is active"
      
      # Test audio with tone
      echo "Playing test tone for 3 seconds..."
      speaker-test -t sine -f 1000 -l 1 -s 1 &
      test_pid=$!
      sleep 3
      kill $test_pid 2>/dev/null || true
      
      echo "Audio test completed"
    else
      echo "No Bluetooth audio device is currently active"
      echo "Available sinks:"
      pactl list short sinks
    fi
    EOF
    
    chmod +x $out/share/hy300/bluetooth/test-audio.sh
    
    # Wrap executables
    wrapProgram $out/bin/hy300-bluetooth-daemon \
      --prefix PATH : ${lib.makeBinPath [ pkgs.bluez pkgs.bluez-tools pkgs.pulseaudio pkgs.coreutils ]}
    
    wrapProgram $out/bin/hy300-bluetooth-control \
      --prefix PATH : ${lib.makeBinPath [ pkgs.coreutils ]}
  '';
  
  meta = with lib; {
    description = "HY300 Bluetooth audio management system";
    longDescription = ''
      Complete Bluetooth audio device support for the HY300 projector including:
      - Automatic device discovery and pairing
      - Audio device management and switching
      - HID device support for keyboards and mice
      - Auto-reconnection to known devices
    '';
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    maintainers = [ "HY300 Project" ];
  };
}