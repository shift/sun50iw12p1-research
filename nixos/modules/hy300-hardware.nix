# HY300 Hardware Configuration Module
# Hardware-specific configuration for HY300 projector
#
# This module handles:
# - Device tree integration
# - Kernel module loading
# - Hardware driver configuration
# - Platform-specific settings

{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.hy300-hardware;
  
  # HY300-specific kernel modules
  hy300-drivers = pkgs.stdenv.mkDerivation {
    pname = "hy300-drivers";
    version = "1.0.0";
    
    src = ../../drivers;
    
    nativeBuildInputs = with pkgs; [
      kmod
      gnumake
    ];
    
    makeFlags = [
      "KVERSION=${config.boot.kernelPackages.kernel.modDirVersion}"
      "KDIR=${config.boot.kernelPackages.kernel.dev}/lib/modules/${config.boot.kernelPackages.kernel.modDirVersion}/build"
    ];
    
    installPhase = ''
      mkdir -p $out/lib/modules/${config.boot.kernelPackages.kernel.modDirVersion}/extra
      
      # Copy compiled kernel modules
      find . -name "*.ko" -exec cp {} $out/lib/modules/${config.boot.kernelPackages.kernel.modDirVersion}/extra/ \;
      
      # Generate modules.dep
      depmod -b $out ${config.boot.kernelPackages.kernel.modDirVersion}
    '';
  };

in
{
  options.services.hy300-hardware = {
    enable = mkEnableOption "HY300 hardware support";

    deviceTree = mkOption {
      type = types.str;
      default = "sun50i-h713-hy300.dtb";
      description = "Device tree blob for HY300 hardware";
    };

    drivers = {
      enableAll = mkOption {
        type = types.bool;
        default = true;
        description = "Enable all HY300-specific drivers";
      };

      mipsCoprocessor = mkOption {
        type = types.bool;
        default = true;
        description = "Enable MIPS co-processor driver";
      };

      keystoneMotor = mkOption {
        type = types.bool;
        default = true;
        description = "Enable keystone motor driver";
      };

      hdmiInput = mkOption {
        type = types.bool;
        default = true;
        description = "Enable HDMI input capture driver";
      };

      wifiAIC8800 = mkOption {
        type = types.bool;
        default = true;
        description = "Enable AIC8800 WiFi driver";
      };
    };

    platformConfig = {
      memoryLayout = mkOption {
        type = types.attrs;
        default = {
          # MIPS co-processor memory layout from firmware analysis
          mipsBootCode = "0x4b100000";      # 4KB
          mipsFirmware = "0x4b101000";      # 12MB
          mipsDebug = "0x4bd01000";         # 1MB
          mipsConfig = "0x4be01000";        # 256KB
          mipsDatabase = "0x4be41000";      # 1MB
          mipsFramebuffer = "0x4bf41000";   # 26MB
          
          # CMA allocation for video processing
          cmaSize = "512M";
          
          # Mali GPU memory
          maliSharedSize = "256M";
        };
        description = "Memory layout configuration for HY300 hardware";
      };

      clockConfiguration = mkOption {
        type = types.attrs;
        default = {
          # CPU frequencies for H713 (based on H6 configuration)
          cpuFreqMin = "480000";  # 480 MHz minimum
          cpuFreqMax = "1800000"; # 1.8 GHz maximum
          cpuFreqGovernor = "ondemand";
          
          # GPU frequencies for Mali-T720MP2
          gpuFreqMin = "200000";  # 200 MHz minimum
          gpuFreqMax = "600000";  # 600 MHz maximum
          
          # Memory frequencies
          dramFreq = "672000";    # 672 MHz from extracted DRAM parameters
        };
        description = "Clock frequency configuration";
      };

      powerManagement = mkOption {
        type = types.attrs;
        default = {
          # Thermal thresholds for projector operation
          tempWarning = "70";     # 70°C warning threshold
          tempCritical = "85";    # 85°C critical threshold
          tempShutdown = "95";    # 95°C emergency shutdown
          
          # Fan control (if available)
          fanMinSpeed = "30";     # 30% minimum fan speed
          fanMaxSpeed = "100";    # 100% maximum fan speed
          
          # Power saving features
          cpuIdle = true;
          ddrSelfRefresh = true;
          suspendToRam = false;   # Disable S2R for projector use
        };
        description = "Power management configuration";
      };
    };
  };

  config = mkIf cfg.enable {
    # Boot configuration for HY300 hardware
    boot = {
      # Device tree configuration
      kernelParams = [
        # Memory configuration
        "cma=${cfg.platformConfig.memoryLayout.cmaSize}"
        "mali.mali_shared_mem_size=${cfg.platformConfig.memoryLayout.maliSharedSize}"
        
        # Clock configuration
        "sunxi-cpufreq.min_freq=${cfg.platformConfig.clockConfiguration.cpuFreqMin}"
        "sunxi-cpufreq.max_freq=${cfg.platformConfig.clockConfiguration.cpuFreqMax}"
        
        # Hardware-specific parameters
        "allwinner.soc=h713"
        "allwinner.variant=hy300"
        
        # HDMI and display configuration
        "video=HDMI-A-1:1920x1080@60"
        "drm.debug=0"
        
        # IR remote configuration
        "lirc.default_protocol=rc-5"
        
        # Disable unnecessary features for embedded use
        "quiet"
        "loglevel=3"
        "systemd.show_status=false"
      ];

      # Kernel modules for HY300 drivers
      kernelModules = flatten [
        # Core platform modules
        [ "sunxi-cpu-comm" ]
        
        # Conditional driver loading
        (optional cfg.drivers.mipsCoprocessor "sunxi-mipsloader")
        (optional cfg.drivers.keystoneMotor "hy300-keystone-motor")
        (optional cfg.drivers.hdmiInput "sunxi-tvcap-enhanced")
        
        # Mali GPU modules
        [ "mali_kbase" "mali_csffw" ]
        
        # Video and media modules
        [ "videodev" "v4l2-common" "videobuf2-core" ]
        
        # IR remote modules
        [ "lirc_dev" "ir-rc5-decoder" "ir-nec-decoder" ]
        
        # WiFi modules (conditional)
        (optional cfg.drivers.wifiAIC8800 "aic8800_fdrv")
        
        # Standard platform modules
        [ "i2c-dev" "spi-dev" "gpio-sunxi" ]
      ];

      # Extra module packages
      extraModulePackages = [
        hy300-drivers
      ];

      # Firmware loading configuration
      initrd = {
        availableKernelModules = [
          # Storage modules for early boot
          "mmc_block"
          "sunxi-mmc"
          
          # Essential drivers
          "sunxi-cpu-comm"
        ];
        
        kernelModules = [
          # Load critical drivers early
          "sunxi-cpu-comm"
        ];
      };
    };

    # Hardware platform configuration
    hardware = {
      # Device tree specification
      deviceTree = {
        enable = true;
        name = cfg.deviceTree;
      };

      # I2C and SPI support for sensors and controls
      i2c.enable = true;
      
      # GPIO support for hardware controls
      gpio.enable = true;
    };

    # System services for hardware management
    systemd.services = {
      # MIPS co-processor initialization
      mips-coprocessor-init = mkIf cfg.drivers.mipsCoprocessor {
        description = "MIPS Co-processor Initialization";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "mips-init" ''
            # Wait for MIPS loader device
            timeout=10
            while [ $timeout -gt 0 ] && [ ! -e /dev/mips-loader ]; do
              sleep 1
              timeout=$((timeout - 1))
            done
            
            if [ -e /dev/mips-loader ]; then
              echo "MIPS co-processor device ready"
              
              # Load display firmware if available
              if [ -f /lib/firmware/display.bin ]; then
                echo "Loading MIPS firmware..."
                echo load > /sys/class/mips-loader/mips0/control
              else
                echo "Warning: display.bin firmware not found"
              fi
            else
              echo "Error: MIPS co-processor device not found"
              exit 1
            fi
          ''}";
        };
      };

      # Keystone motor initialization
      keystone-motor-init = mkIf cfg.drivers.keystoneMotor {
        description = "Keystone Motor Initialization";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "keystone-motor-init" ''
            # Wait for keystone motor device
            timeout=10
            while [ $timeout -gt 0 ] && [ ! -e /dev/keystone-motor ]; do
              sleep 1
              timeout=$((timeout - 1))
            done
            
            if [ -e /dev/keystone-motor ]; then
              echo "Keystone motor device ready"
              
              # Initialize motor to center position
              echo "center" > /sys/class/keystone-motor/motor0/position
              echo "Keystone motor initialized to center position"
            else
              echo "Warning: Keystone motor device not found"
            fi
          ''}";
        };
      };

      # HDMI input initialization
      hdmi-input-init = mkIf cfg.drivers.hdmiInput {
        description = "HDMI Input Initialization";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "hdmi-input-init" ''
            # Wait for video capture device
            timeout=10
            while [ $timeout -gt 0 ] && [ ! -e /dev/video0 ]; do
              sleep 1
              timeout=$((timeout - 1))
            done
            
            if [ -e /dev/video0 ]; then
              echo "HDMI input capture device ready"
              
              # Set default input format
              v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat=YUYV
              echo "HDMI input configured for 1080p"
            else
              echo "Warning: HDMI input device not found"
            fi
          ''}";
        };
      };

      # Hardware monitoring and thermal management
      hardware-monitor = {
        description = "HY300 Hardware Monitoring";
        wantedBy = [ "multi-user.target" ];
        
        serviceConfig = {
          Type = "simple";
          Restart = "always";
          RestartSec = "30s";
          ExecStart = "${pkgs.writeShellScript "hardware-monitor" ''
            #!/bin/bash
            
            # Temperature monitoring
            check_temperature() {
              if [ -f /sys/class/thermal/thermal_zone0/temp ]; then
                temp=$(cat /sys/class/thermal/thermal_zone0/temp)
                temp_celsius=$((temp / 1000))
                
                if [ $temp_celsius -gt ${cfg.platformConfig.powerManagement.tempCritical} ]; then
                  echo "Critical temperature: ${temp_celsius}°C - initiating thermal protection"
                  # Reduce CPU frequency
                  echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                elif [ $temp_celsius -gt ${cfg.platformConfig.powerManagement.tempWarning} ]; then
                  echo "Warning temperature: ${temp_celsius}°C"
                fi
              fi
            }
            
            # Main monitoring loop
            while true; do
              check_temperature
              sleep 30
            done
          ''}";
        };
      };
    };

    # udev rules for HY300 hardware
    services.udev.extraRules = ''
      # MIPS co-processor device
      KERNEL=="mips-loader", GROUP="video", MODE="0664"
      
      # Keystone motor device
      KERNEL=="keystone-motor", GROUP="video", MODE="0664"
      
      # HDMI input capture device
      KERNEL=="video[0-9]*", ATTRS{name}=="sunxi-tvcap*", GROUP="video", MODE="0664"
      
      # IR remote receiver
      KERNEL=="lirc[0-9]*", GROUP="input", MODE="0664"
      
      # GPIO devices for hardware control
      KERNEL=="gpiochip*", GROUP="gpio", MODE="0664"
      
      # I2C devices for sensor access
      KERNEL=="i2c-[0-9]*", GROUP="i2c", MODE="0664"
    '';

    # Create necessary device groups
    users.groups = {
      gpio = {};
      i2c = {};
    };

    # Add projector user to hardware groups
    users.users.projector = mkIf (config.users.users ? projector) {
      extraGroups = [ "video" "audio" "input" "gpio" "i2c" "dialout" ];
    };

    # Environment variables for hardware access
    environment.variables = {
      # Mali GPU configuration
      MALI_SHARED_MEM_SIZE = cfg.platformConfig.memoryLayout.maliSharedSize;
      
      # Video capture configuration
      V4L2_DEVICE = "/dev/video0";
      
      # Hardware device paths
      HY300_MIPS_DEVICE = "/dev/mips-loader";
      HY300_KEYSTONE_DEVICE = "/dev/keystone-motor";
      HY300_IR_DEVICE = "/dev/lirc0";
    };

    # System packages for hardware interaction
    environment.systemPackages = with pkgs; [
      # Video tools
      v4l-utils
      
      # GPIO tools
      libgpiod
      
      # I2C tools
      i2c-tools
      
      # IR remote tools
      lirc
      
      # Thermal monitoring
      lm_sensors
      
      # System monitoring
      htop
      iotop
      
      # Hardware debugging
      usbutils
      pciutils
    ];
  };
}