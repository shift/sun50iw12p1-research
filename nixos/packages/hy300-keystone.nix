# HY300 Keystone Correction Service Package
# System service for automatic and manual keystone correction

{ lib, pkgs, writeShellScript, ... }:

pkgs.stdenv.mkDerivation {
  pname = "hy300-keystone-service";
  version = "1.0.0";
  
  src = ./.;
  
  nativeBuildInputs = with pkgs; [
    makeWrapper
  ];
  
  buildInputs = with pkgs; [
    bash
    coreutils
    util-linux
    inotify-tools
  ];
  
  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/lib/systemd/system
    mkdir -p $out/etc/hy300
    mkdir -p $out/share/hy300/keystone
    
    # Install keystone correction daemon
    cat > $out/bin/hy300-keystone-daemon << 'EOF'
    #!/bin/bash
    # HY300 Keystone Correction Daemon
    
    DEVICE="/dev/keystone-motor"
    MIPS_DEVICE="/dev/mips-loader"
    ACCEL_DEVICE="/dev/iio:device0"
    CONFIG_FILE="/etc/hy300/keystone.conf"
    PROFILES_DIR="/var/lib/hy300/keystone-profiles"
    CONTROL_FIFO="/run/hy300/keystone-control"
    
    # Default configuration
    AUTO_CORRECTION=true
    CORRECTION_INTERVAL=5
    MOTOR_ENABLED=true
    DIGITAL_ENABLED=true
    
    # Load configuration
    if [ -f "$CONFIG_FILE" ]; then
      source "$CONFIG_FILE"
    fi
    
    # Create profiles directory
    mkdir -p "$PROFILES_DIR"
    
    # Create control FIFO
    mkdir -p "$(dirname "$CONTROL_FIFO")"
    mkfifo "$CONTROL_FIFO" 2>/dev/null || true
    
    # Function to read accelerometer data
    read_accelerometer() {
      if [ -f "$ACCEL_DEVICE/in_accel_x_raw" ]; then
        local accel_x=$(cat "$ACCEL_DEVICE/in_accel_x_raw")
        local accel_y=$(cat "$ACCEL_DEVICE/in_accel_y_raw")
        local accel_z=$(cat "$ACCEL_DEVICE/in_accel_z_raw")
        
        # Calculate tilt angles (simplified calculation)
        # Real implementation would use proper trigonometry
        local tilt_x=$((accel_x / 100))
        local tilt_y=$((accel_y / 100))
        
        echo "$tilt_x:$tilt_y"
      else
        echo "0:0"
      fi
    }
    
    # Function to apply motor correction
    apply_motor_correction() {
      local correction_value=$1
      
      if [ "$MOTOR_ENABLED" = "true" ] && [ -e "$DEVICE" ]; then
        echo "motor:$correction_value" > "$DEVICE"
        echo "Applied motor correction: $correction_value"
      fi
    }
    
    # Function to apply digital correction via MIPS
    apply_digital_correction() {
      local corners=$1
      
      if [ "$DIGITAL_ENABLED" = "true" ] && [ -e "$MIPS_DEVICE" ]; then
        echo "keystone:$corners" > /sys/class/mips-loader/mips0/control
        echo "Applied digital correction: $corners"
      fi
    }
    
    # Function to save keystone profile
    save_profile() {
      local profile_name=$1
      local motor_value=$2
      local digital_corners=$3
      
      local profile_file="$PROFILES_DIR/$profile_name.conf"
      
      cat > "$profile_file" << EOL
    # HY300 Keystone Profile: $profile_name
    # Saved: $(date)
    
    MOTOR_VALUE=$motor_value
    DIGITAL_CORNERS=$digital_corners
    EOL
      
      echo "Saved keystone profile: $profile_name"
    }
    
    # Function to load keystone profile
    load_profile() {
      local profile_name=$1
      local profile_file="$PROFILES_DIR/$profile_name.conf"
      
      if [ -f "$profile_file" ]; then
        source "$profile_file"
        
        if [ -n "$MOTOR_VALUE" ]; then
          apply_motor_correction "$MOTOR_VALUE"
        fi
        
        if [ -n "$DIGITAL_CORNERS" ]; then
          apply_digital_correction "$DIGITAL_CORNERS"
        fi
        
        echo "Loaded keystone profile: $profile_name"
      else
        echo "Profile not found: $profile_name"
      fi
    }
    
    # Function to handle external commands
    handle_command() {
      local command=$1
      
      case $command in
        auto:on)
          AUTO_CORRECTION=true
          echo "Enabled automatic keystone correction"
          ;;
        auto:off)
          AUTO_CORRECTION=false
          echo "Disabled automatic keystone correction"
          ;;
        motor:*)
          value=$(echo $command | cut -d: -f2)
          apply_motor_correction "$value"
          ;;
        digital:*)
          corners=$(echo $command | cut -d: -f2)
          apply_digital_correction "$corners"
          ;;
        save:*)
          profile_name=$(echo $command | cut -d: -f2)
          # Get current settings for saving
          motor_value=$(cat /sys/class/keystone-motor/motor0/position 2>/dev/null || echo "0")
          digital_corners="0,0:1920,0:1920,1080:0,1080"  # Default
          save_profile "$profile_name" "$motor_value" "$digital_corners"
          ;;
        load:*)
          profile_name=$(echo $command | cut -d: -f2)
          load_profile "$profile_name"
          ;;
        reset)
          apply_motor_correction "0"
          apply_digital_correction "0,0:1920,0:1920,1080:0,1080"
          echo "Reset keystone correction"
          ;;
        status)
          echo "Auto correction: $AUTO_CORRECTION"
          echo "Motor enabled: $MOTOR_ENABLED"
          echo "Digital enabled: $DIGITAL_ENABLED"
          if [ -e "$DEVICE" ]; then
            echo "Motor position: $(cat /sys/class/keystone-motor/motor0/position 2>/dev/null || echo "unknown")"
          fi
          ;;
      esac
    }
    
    # Signal handlers
    handle_sigterm() {
      echo "Keystone service shutting down..."
      rm -f "$CONTROL_FIFO"
      exit 0
    }
    
    handle_sigusr1() {
      echo "Reloading keystone configuration..."
      if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
        echo "Configuration reloaded"
      fi
    }
    
    trap handle_sigterm TERM INT
    trap handle_sigusr1 USR1
    
    echo "Starting HY300 keystone correction service..."
    echo "Auto correction: $AUTO_CORRECTION"
    echo "Motor enabled: $MOTOR_ENABLED"
    echo "Digital enabled: $DIGITAL_ENABLED"
    echo "Control FIFO: $CONTROL_FIFO"
    
    # Initialize to center position
    apply_motor_correction "0"
    apply_digital_correction "0,0:1920,0:1920,1080:0,1080"
    
    # Main service loop
    while true; do
      # Check for external commands
      if read -t 1 command < "$CONTROL_FIFO" 2>/dev/null; then
        handle_command "$command"
      fi
      
      # Automatic correction if enabled
      if [ "$AUTO_CORRECTION" = "true" ]; then
        tilt_data=$(read_accelerometer)
        tilt_y=$(echo $tilt_data | cut -d: -f2)
        
        if [ -n "$tilt_y" ] && [ "$tilt_y" != "0" ]; then
          # Apply motor correction for major adjustments
          if [ ${tilt_y#-} -gt 5 ]; then
            motor_correction=$((tilt_y / 2))
            apply_motor_correction "$motor_correction"
          fi
          
          # Apply digital fine-tuning
          # Calculate 4-corner coordinates based on tilt
          digital_correction="0,$tilt_y:1920,$tilt_y:1920,$((1080-tilt_y)):0,$((1080-tilt_y))"
          apply_digital_correction "$digital_correction"
        fi
      fi
      
      sleep $CORRECTION_INTERVAL
    done
    EOF
    
    chmod +x $out/bin/hy300-keystone-daemon
    
    # Install keystone control utility
    cat > $out/bin/hy300-keystone-control << 'EOF'
    #!/bin/bash
    # HY300 Keystone Control Utility
    
    CONTROL_FIFO="/run/hy300/keystone-control"
    
    usage() {
      echo "Usage: $0 <command>"
      echo ""
      echo "Commands:"
      echo "  auto on|off          - Enable/disable automatic correction"
      echo "  motor <value>        - Set motor position (-100 to +100)"
      echo "  digital <corners>    - Set digital correction (x1,y1:x2,y2:x3,y3:x4,y4)"
      echo "  save <profile>       - Save current settings to profile"
      echo "  load <profile>       - Load settings from profile"
      echo "  reset                - Reset to default position"
      echo "  status               - Show current status"
      echo "  profiles             - List available profiles"
    }
    
    if [ $# -eq 0 ]; then
      usage
      exit 1
    fi
    
    command=$1
    shift
    
    case $command in
      auto)
        if [ "$1" = "on" ]; then
          echo "auto:on" > "$CONTROL_FIFO"
        elif [ "$1" = "off" ]; then
          echo "auto:off" > "$CONTROL_FIFO"
        else
          echo "Error: auto requires 'on' or 'off'"
          exit 1
        fi
        ;;
      motor)
        if [ -n "$1" ]; then
          echo "motor:$1" > "$CONTROL_FIFO"
        else
          echo "Error: motor requires a value"
          exit 1
        fi
        ;;
      digital)
        if [ -n "$1" ]; then
          echo "digital:$1" > "$CONTROL_FIFO"
        else
          echo "Error: digital requires corner coordinates"
          exit 1
        fi
        ;;
      save)
        if [ -n "$1" ]; then
          echo "save:$1" > "$CONTROL_FIFO"
        else
          echo "Error: save requires a profile name"
          exit 1
        fi
        ;;
      load)
        if [ -n "$1" ]; then
          echo "load:$1" > "$CONTROL_FIFO"
        else
          echo "Error: load requires a profile name"
          exit 1
        fi
        ;;
      reset)
        echo "reset" > "$CONTROL_FIFO"
        ;;
      status)
        echo "status" > "$CONTROL_FIFO"
        # Wait for response and display
        sleep 1
        ;;
      profiles)
        echo "Available keystone profiles:"
        ls -1 /var/lib/hy300/keystone-profiles/*.conf 2>/dev/null | sed 's/.*\///;s/\.conf$//' || echo "No profiles found"
        ;;
      *)
        echo "Error: Unknown command: $command"
        usage
        exit 1
        ;;
    esac
    EOF
    
    chmod +x $out/bin/hy300-keystone-control
    
    # Install systemd service
    cat > $out/lib/systemd/system/hy300-keystone.service << EOF
    [Unit]
    Description=HY300 Keystone Correction Service
    After=mips-coprocessor-init.service keystone-motor-init.service
    Wants=mips-coprocessor-init.service keystone-motor-init.service
    
    [Service]
    Type=simple
    User=root
    Restart=always
    RestartSec=15s
    
    ExecStart=$out/bin/hy300-keystone-daemon
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
    
    # Install default configuration
    cat > $out/etc/hy300/keystone.conf << EOF
    # HY300 Keystone Correction Configuration
    
    # Automatic correction settings
    AUTO_CORRECTION=true
    CORRECTION_INTERVAL=5
    
    # Hardware control
    MOTOR_ENABLED=true
    DIGITAL_ENABLED=true
    
    # Motor calibration
    MOTOR_STEPS_PER_DEGREE=10
    MOTOR_MAX_POSITION=100
    MOTOR_MIN_POSITION=-100
    
    # Accelerometer calibration
    ACCEL_X_OFFSET=0
    ACCEL_Y_OFFSET=0
    ACCEL_Z_OFFSET=0
    
    # Digital correction parameters
    DIGITAL_SMOOTHING=true
    DIGITAL_PRECISION=high
    EOF
    
    # Install calibration scripts
    cat > $out/share/hy300/keystone/calibrate-accelerometer.sh << 'EOF'
    #!/bin/bash
    # HY300 Accelerometer Calibration Script
    
    echo "HY300 Accelerometer Calibration"
    echo "==============================="
    echo ""
    echo "This script will help calibrate the accelerometer for accurate keystone correction."
    echo ""
    
    ACCEL_DEVICE="/dev/iio:device0"
    CONFIG_FILE="/etc/hy300/keystone.conf"
    
    if [ ! -e "$ACCEL_DEVICE" ]; then
      echo "Error: Accelerometer device not found: $ACCEL_DEVICE"
      exit 1
    fi
    
    echo "Step 1: Place the projector on a level surface"
    read -p "Press Enter when ready..."
    
    # Read level position values
    echo "Reading level position values..."
    x_level=$(cat "$ACCEL_DEVICE/in_accel_x_raw")
    y_level=$(cat "$ACCEL_DEVICE/in_accel_y_raw")
    z_level=$(cat "$ACCEL_DEVICE/in_accel_z_raw")
    
    echo "Level position: X=$x_level Y=$y_level Z=$z_level"
    
    # Calculate offsets (assuming level should be 0,0,1g)
    x_offset=$((0 - x_level))
    y_offset=$((0 - y_level))
    z_offset=$((1000 - z_level))  # 1g = ~1000 in raw units
    
    echo ""
    echo "Calculated offsets:"
    echo "X offset: $x_offset"
    echo "Y offset: $y_offset"
    echo "Z offset: $z_offset"
    
    # Update configuration file
    if [ -f "$CONFIG_FILE" ]; then
      cp "$CONFIG_FILE" "$CONFIG_FILE.backup"
      sed -i "s/ACCEL_X_OFFSET=.*/ACCEL_X_OFFSET=$x_offset/" "$CONFIG_FILE"
      sed -i "s/ACCEL_Y_OFFSET=.*/ACCEL_Y_OFFSET=$y_offset/" "$CONFIG_FILE"
      sed -i "s/ACCEL_Z_OFFSET=.*/ACCEL_Z_OFFSET=$z_offset/" "$CONFIG_FILE"
      
      echo ""
      echo "Configuration updated successfully!"
      echo "Restart the keystone service to apply changes:"
      echo "  systemctl restart hy300-keystone"
    else
      echo "Warning: Configuration file not found: $CONFIG_FILE"
    fi
    EOF
    
    chmod +x $out/share/hy300/keystone/calibrate-accelerometer.sh
    
    # Wrap executables
    wrapProgram $out/bin/hy300-keystone-daemon \
      --prefix PATH : ${lib.makeBinPath [ pkgs.coreutils pkgs.util-linux pkgs.inotify-tools ]}
    
    wrapProgram $out/bin/hy300-keystone-control \
      --prefix PATH : ${lib.makeBinPath [ pkgs.coreutils ]}
  '';
  
  meta = with lib; {
    description = "HY300 keystone correction service";
    longDescription = ''
      System service providing automatic and manual keystone correction
      for the HY300 projector using accelerometer data and motor control.
    '';
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    maintainers = [ "HY300 Project" ];
  };
}