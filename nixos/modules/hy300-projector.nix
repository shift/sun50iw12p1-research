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

    # VM testing mode
    vmMode = mkOption {
      type = types.bool;
      default = false;
      description = "Enable VM testing mode (disables hardware-specific features)";
    };

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

      # VM-specific hardware options
      motors = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable motor control hardware";
            };
          };
        };
        default = {};
      };

      gpio = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable GPIO hardware support";
            };
          };
        };
        default = {};
      };

      mips = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable MIPS co-processor";
            };
          };
        };
        default = {};
      };

      audio = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable audio hardware";
            };
          };
        };
        default = {};
      };

      bluetooth = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable Bluetooth hardware";
            };
          };
        };
        default = {};
      };

      mali = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable Mali GPU acceleration";
            };
            softwareRendering = mkOption {
              type = types.bool;
              default = false;
              description = "Use software rendering instead of GPU acceleration";
            };
          };
        };
        default = {};
      };
    };

    # Kodi media center configuration
    kodi = {
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

      webInterface = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = false;
              description = "Enable Kodi web interface";
            };
            port = mkOption {
              type = types.int;
              default = 8080;
              description = "Port for Kodi web interface";
            };
          };
        };
        default = {};
      };

      plugins = mkOption {
        type = types.submodule {
          options = {
            keystoneCorrection = mkOption {
              type = types.submodule {
                options = {
                  enable = mkOption {
                    type = types.bool;
                    default = true;
                    description = "Enable keystone correction plugin";
                  };
                };
              };
              default = {};
            };
            wifiSetup = mkOption {
              type = types.submodule {
                options = {
                  enable = mkOption {
                    type = types.bool;
                    default = true;
                    description = "Enable WiFi setup plugin";
                  };
                };
              };
              default = {};
            };
            systemInfo = mkOption {
              type = types.submodule {
                options = {
                  enable = mkOption {
                    type = types.bool;
                    default = true;
                    description = "Enable system info plugin";
                  };
                };
              };
              default = {};
            };
          };
        };
        default = {};
      };

      testContent = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = false;
              description = "Include test content for demonstration";
            };
            includeTestVideos = mkOption {
              type = types.bool;
              default = false;
              description = "Include test video files";
            };
            includeSampleMusic = mkOption {
              type = types.bool;
              default = false;
              description = "Include sample music files";
            };
          };
        };
        default = {};
      };
    };

    # Services configuration
    services = {
      keystone = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable keystone correction service";
            };
            simulationMode = mkOption {
              type = types.bool;
              default = false;
              description = "Run in simulation mode (no hardware interaction)";
            };
          };
        };
        default = {};
      };

      wifi = mkOption {
        type = types.submodule {
          options = {
            enable = mkOption {
              type = types.bool;
              default = true;
              description = "Enable WiFi management service";
            };
            interface = mkOption {
              type = types.str;
              default = "wlan0";
              description = "WiFi interface name";
            };
          };
        };
        default = {};
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

    # Boot configuration (only for hardware, not VM)
    boot = mkIf (!cfg.vmMode && cfg.hardware.bootloader.enable) {
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
      kernelModules = optionals (!cfg.vmMode) [
        # HY300-specific drivers (only on hardware)
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
      # GPU support - unified configuration for both hardware and VM
      opengl = {
        enable = true;
        driSupport = true;
        driSupport32Bit = false;
        extraPackages = with pkgs; [
          mesa
          mesa.drivers
        ] ++ optionals (cfg.hardware.mali.enable && !cfg.hardware.mali.softwareRendering && !cfg.vmMode) [
          # Mali GPU support (hardware only)
        ];
      };

      # Bluetooth support
      bluetooth = mkIf cfg.hardware.bluetooth.enable {
        enable = true;
        powerOnBoot = true;
        settings = {
          General = {
            Enable = "Source,Sink,Media,Socket";
            MultiProfile = "multiple";
          };
        };
      };

      # Pulseaudio for audio
      pulseaudio = mkIf cfg.hardware.audio.enable {
        enable = true;
        support32Bit = false;
        extraModules = optionals cfg.hardware.bluetooth.enable [ pkgs.pulseaudio-modules-bt ];
        package = pkgs.pulseaudioFull;
      };
    };

    # Networking configuration
    networking = {
      wireless = mkIf (cfg.network.wifi.enable && !cfg.vmMode) {
        enable = true;
        userControlled.enable = true;
      };
      
      networkmanager = mkIf cfg.network.wifi.enable {
        enable = true;
        wifi.backend = if cfg.vmMode then "iwd" else "wpa_supplicant";
      };
    };

    # Kodi configuration
    systemd.services.kodi = mkIf cfg.kodi.enable {
      description = "Kodi Media Center";
      after = [ "graphical-session.target" ];
      wantedBy = [ "multi-user.target" ];
      serviceConfig = {
        Type = "simple";
        User = if cfg.vmMode then "hy300" else "projector";
        ExecStart = "${pkgs.kodi}/bin/kodi --standalone";
        Restart = "always";
        RestartSec = 5;
      };
    };

    # Keystone correction service
    systemd.services.hy300-keystone = mkIf cfg.services.keystone.enable {
      description = "HY300 Keystone Correction Service";
      wantedBy = [ "multi-user.target" ];
      after = if cfg.vmMode then [ "hy300-vm-simulation.service" ] else [ "hy300-hardware-init.service" ];
      serviceConfig = {
        Type = "simple";
        ExecStart = "${(pkgs.callPackage ../packages/hy300-keystone.nix {})}/bin/hy300-keystone" + 
          (if cfg.services.keystone.simulationMode || cfg.vmMode then " --simulation" else "");
        Restart = "always";
        RestartSec = 5;
        # Create state directory
        StateDirectory = "hy300";
        StateDirectoryMode = "0755";
      };
    };

    # System services
    systemd.services = {
      # WiFi management service
      hy300-wifi = mkIf cfg.services.wifi.enable {
        description = "HY300 WiFi Management Service";
        wantedBy = [ "multi-user.target" ];
        after = [ "network.target" ];
        serviceConfig = {
          Type = "simple";
          ExecStart = "${(pkgs.callPackage ../packages/hy300-wifi.nix {})}/bin/hy300-wifi" +
            (if cfg.vmMode then " --simulation" else "");
          Restart = "always";
          RestartSec = 5;
          # Create state directory
          StateDirectory = "hy300";
          StateDirectoryMode = "0755";
        };
      };

      # Early hardware initialization (only on real hardware)
      hy300-hardware-init = mkIf (!cfg.vmMode) {
        description = "HY300 Hardware Initialization";
        wantedBy = [ "multi-user.target" ];
        after = [ "systemd-modules-load.service" ];
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "hy300-init" ''
            set -e
            
            echo "Starting HY300 hardware initialization..."
            
            # Check for required device tree nodes
            if [ -d /proc/device-tree ]; then
              echo "Device tree loaded successfully"
            else
              echo "Warning: Device tree not found"
            fi
            
            # Initialize MIPS co-processor
            if [ -e /dev/mips-loader ]; then
              echo "MIPS co-processor device found"
              # Load MIPS firmware if available
              if [ -f /lib/firmware/hy300/display.bin ]; then
                echo "Loading MIPS firmware..."
                cat /lib/firmware/hy300/display.bin > /dev/mips-loader
                echo "MIPS firmware loaded"
              else
                echo "Warning: MIPS firmware not found"
              fi
            else
              echo "Warning: MIPS loader device not available"
            fi
            
            # Initialize keystone motor
            if [ -e /dev/keystone-motor ]; then
              echo "Keystone motor device found"
              # Center the motor
              echo "0,0" > /dev/keystone-motor
              echo "Keystone motor centered"
            else
              echo "Warning: Keystone motor device not available"
            fi
            
            # Check HDMI input
            if [ -e /dev/video0 ]; then
              echo "HDMI input device ready"
              # Test capture capability
              if command -v v4l2-ctl >/dev/null; then
                v4l2-ctl --device=/dev/video0 --list-formats >/dev/null 2>&1 && echo "HDMI capture formats detected" || echo "Warning: HDMI capture test failed"
              fi
            else
              echo "Warning: HDMI input device not available"
            fi
            
            # Check WiFi interface
            if [ -e /sys/class/net/wlan0 ]; then
              echo "WiFi interface available"
            else
              echo "Warning: WiFi interface not found"
            fi
            
            # Check Mali GPU
            if [ -e /dev/mali0 ]; then
              echo "Mali GPU device found"
            else
              echo "Info: Mali GPU device not available (using software rendering)"
            fi
            
            # Create runtime directories
            mkdir -p /var/lib/hy300
            chmod 755 /var/lib/hy300
            
            echo "HY300 hardware initialization complete"
          ''}";
        };
      };

      # VM simulation services
      hy300-vm-simulation = mkIf cfg.vmMode {
        description = "HY300 VM Hardware Simulation";
        wantedBy = [ "multi-user.target" ];
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${pkgs.writeShellScript "hy300-vm-init" ''
            echo "HY300 VM simulation mode initialized"
            echo "Simulating hardware components:"
            echo "- Motors: disabled"
            echo "- GPIO: disabled" 
            echo "- MIPS: disabled"
            echo "- Audio: software"
            echo "- Graphics: software rendering"
          ''}";
        };
      };
    };

    # User configuration
    users.users = if cfg.vmMode then {
      hy300 = {
        isNormalUser = true;
        description = "HY300 Test User";
        extraGroups = [ "wheel" "audio" "video" "networkmanager" ];
        shell = pkgs.bash;
      };
    } else {
      projector = {
        isNormalUser = true;
        description = "HY300 Projector User";
        extraGroups = [ "audio" "video" "input" "dialout" ];
        shell = pkgs.bash;
      };
    };

    # Auto-login configuration
    services.getty.autologinUser = mkIf cfg.kodi.autoStart (
      if cfg.vmMode then "hy300" else "projector"
    );

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
      
      # Media center
      kodi
      
      # VM-specific tools
    ] ++ optionals cfg.vmMode [
      firefox
      htop
      tree
      curl
      wget
    ] ++ optionals (!cfg.vmMode) [
      # Hardware-specific tools
      lirc
      i2c-tools
    ];

    # Firmware files (only for hardware)
    hardware.firmware = optionals (!cfg.vmMode) [
      # Add HY300-specific firmware
      (pkgs.stdenv.mkDerivation {
        name = "hy300-firmware";
        src = if builtins.pathExists ./firmware then ./firmware else pkgs.emptyDirectory;
        installPhase = ''
          mkdir -p $out/lib/firmware
          if [ -d "${./firmware}" ]; then
            cp -r * $out/lib/firmware/ || true
          fi
        '';
      })
    ];
  };
}