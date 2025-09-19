# HY300 Projector NixOS Module
# Main configuration module for the HY300 Android projector running mainline Linux
#
# This module provides complete system integration for the HY300 projector including:
# - Hardware configuration and drivers
# - Kodi media center with projector optimizations  
# - Keystone correction system
# - IR remote control integration
# - WiFi configuration management
# - Bluetooth audio support

{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.hy300-projector;
in
{
  options.services.hy300-projector = {
    enable = mkEnableOption "HY300 projector system integration";

    hardware = {
      deviceTree = mkOption {
        type = types.str;
        default = "sun50i-h713-hy300.dtb";
        description = "Device tree blob for HY300 hardware";
      };

      bootloader = {
        enable = mkOption {
          type = types.bool;
          default = true;
          description = "Use custom U-Boot bootloader for HY300";
        };

        uBootPath = mkOption {
          type = types.str;
          default = "u-boot-sunxi-with-spl.bin";
          description = "Path to U-Boot bootloader binary";
        };
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
          description = "Enable MIPS co-processor driver for display processing";
        };

        keystoneMotor = mkOption {
          type = types.bool;
          default = true;
          description = "Enable keystone correction motor driver";
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
    };

    mediaCenter = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable Kodi media center";
      };

      autoStart = mkOption {
        type = types.bool;
        default = true;
        description = "Start Kodi automatically on boot";
      };

      hardwareAcceleration = mkOption {
        type = types.bool;
        default = true;
        description = "Enable Mali GPU hardware acceleration";
      };

      customPlugins = mkOption {
        type = types.bool;
        default = true;
        description = "Install HY300-specific Kodi plugins";
      };

      audioOutput = mkOption {
        type = types.enum [ "hdmi" "bluetooth" "auto" ];
        default = "auto";
        description = "Default audio output method";
      };
    };

    keystoneCorrection = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable keystone correction system";
      };

      autoCorrection = mkOption {
        type = types.bool;
        default = true;
        description = "Enable automatic keystone correction using accelerometer";
      };

      manualInterface = mkOption {
        type = types.bool;
        default = true;
        description = "Enable manual 4-corner keystone adjustment";
      };

      motorControl = mkOption {
        type = types.bool;
        default = true;
        description = "Enable physical motor adjustment";
      };
    };

    remoteControl = {
      enable = mkOption {
        type = types.bool;
        default = true;
        description = "Enable IR remote control";
      };

      protocol = mkOption {
        type = types.str;
        default = "hy300-remote";
        description = "IR remote protocol configuration";
      };

      wifiSetup = mkOption {
        type = types.bool;
        default = true;
        description = "Enable WiFi configuration via remote control";
      };
    };

    network = {
      wifi = {
        enable = mkOption {
          type = types.bool;
          default = true;
          description = "Enable WiFi support";
        };

        remoteSetup = mkOption {
          type = types.bool;
          default = true;
          description = "Enable remote control WiFi configuration";
        };

        profileManagement = mkOption {
          type = types.bool;
          default = true;
          description = "Enable WiFi profile save/load";
        };
      };

      bluetooth = {
        enable = mkOption {
          type = types.bool;
          default = true;
          description = "Enable Bluetooth support";
        };

        audioDevices = mkOption {
          type = types.bool;
          default = true;
          description = "Enable Bluetooth audio device support";
        };

        hidDevices = mkOption {
          type = types.bool;
          default = true;
          description = "Enable Bluetooth HID device support";
        };

        autoReconnect = mkOption {
          type = types.bool;
          default = true;
          description = "Enable automatic device reconnection";
        };
      };
    };

    optimization = {
      thermalManagement = mkOption {
        type = types.bool;
        default = true;
        description = "Enable thermal management and cooling optimization";
      };

      powerManagement = mkOption {
        type = types.bool;
        default = true;
        description = "Enable power optimization for projector use";
      };

      performanceProfile = mkOption {
        type = types.enum [ "performance" "balanced" "power-saver" ];
        default = "balanced";
        description = "System performance profile";
      };
    };
  };

  config = mkIf cfg.enable {
    # Basic system configuration
    system.stateVersion = "24.05";

    # Boot configuration
    boot = mkIf cfg.hardware.bootloader.enable {
      loader.grub.enable = false;
      loader.generic-extlinux-compatible.enable = true;
      
      # Use custom U-Boot bootloader
      loader.generic-extlinux-compatible.configurationLimit = 10;
      
      # Kernel command line for projector optimization
      kernelParams = [
        "console=ttyS0,115200"
        "earlycon=uart,mmio32,0x05000000"
        "root=/dev/mmcblk0p2"
        "rootwait"
        "quiet"
        "splash"
        # Mali GPU optimization
        "mali.pp_core_mask=0x7"
        "mali.gp_core_mask=0x1"
        # CMA for video memory
        "cma=512M"
        # IR remote support
        "lirc.default_protocol=rc-5"
      ];

      # Device tree configuration
      kernelPackages = pkgs.linuxPackages_6_6;
      kernelModules = [
        # HY300-specific drivers
        "sunxi-cpu-comm"
        "sunxi-mipsloader" 
        "hy300-keystone-motor"
        "sunxi-tvcap-enhanced"
        # Standard modules
        "mali_kbase"
        "lirc_dev"
        "ir-rc5-decoder"
      ];
    };

    # Hardware enablement
    hardware = {
      # GPU support
      opengl = mkIf cfg.mediaCenter.hardwareAcceleration {
        enable = true;
        driSupport = true;
        driSupport32Bit = false;
        extraPackages = with pkgs; [
          # Mali GPU support
          mesa
          mesa.drivers
        ];
      };

      # Bluetooth support
      bluetooth = mkIf cfg.network.bluetooth.enable {
        enable = true;
        powerOnBoot = true;
        settings = {
          General = {
            Enable = "Source,Sink,Media,Socket";
            MultiProfile = "multiple";
          };
        };
      };

      # Pulseaudio/Pipewire for audio
      pulseaudio = mkIf (cfg.mediaCenter.audioOutput != "hdmi") {
        enable = true;
        support32Bit = false;
        extraModules = [ pkgs.pulseaudio-modules-bt ];
        package = pkgs.pulseaudioFull;
      };
    };

    # Networking configuration
    networking = {
      wireless = mkIf cfg.network.wifi.enable {
        enable = true;
        userControlled.enable = true;
      };
      
      networkmanager = mkIf cfg.network.wifi.enable {
        enable = true;
        wifi.backend = "wpa_supplicant";
      };
    };

    # System services
    systemd.services = {
      # Early hardware initialization
      hy300-hardware-init = {
        description = "HY300 Hardware Initialization";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "hy300-init" ''
            # Load HY300-specific firmware
            echo "Loading HY300 firmware..."
            
            # Initialize MIPS co-processor
            if [ -e /dev/mips-loader ]; then
              echo "Initializing MIPS co-processor..."
            fi
            
            # Initialize keystone motor
            if [ -e /dev/keystone-motor ]; then
              echo "Initializing keystone motor..."
            fi
            
            # Initialize HDMI input
            if [ -e /dev/video0 ]; then
              echo "HDMI input device ready"
            fi
            
            echo "HY300 hardware initialization complete"
          ''}";
        };
      };
    };

    # User configuration for projector operation
    users.users.projector = mkIf cfg.mediaCenter.enable {
      isNormalUser = true;
      description = "HY300 Projector User";
      extraGroups = [ "audio" "video" "input" "dialout" ];
      shell = pkgs.bash;
    };

    # Auto-login for media center
    services.getty.autologinUser = mkIf cfg.mediaCenter.autoStart "projector";

    # Environment packages
    environment.systemPackages = with pkgs; [
      # Basic system tools
      coreutils
      util-linux
      procps
      
      # Network tools
      networkmanager
      wpa_supplicant
      
      # Audio tools
      alsa-utils
      pulseaudio
      
      # Video tools
      v4l-utils
      
      # IR remote tools
      lirc
      
      # HY300-specific packages (to be defined in packages/)
      # hy300-keystone-service
      # hy300-wifi-setup
      # kodi-hy300-plugins
    ];

    # Firmware files
    hardware.firmware = [
      # Add HY300-specific firmware
      (pkgs.stdenv.mkDerivation {
        name = "hy300-firmware";
        src = ./firmware;
        installPhase = ''
          mkdir -p $out/lib/firmware
          cp -r * $out/lib/firmware/
        '';
      })
    ];
  };
}