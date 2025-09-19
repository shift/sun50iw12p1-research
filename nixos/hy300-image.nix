# HY300 Projector NixOS System Image Configuration
# Complete system configuration for HY300 projector deployment
#
# This configuration creates a bootable NixOS image optimized for:
# - HY300 hardware platform (Allwinner H713)
# - Kodi media center with projector optimizations
# - IR remote control and WiFi setup
# - Keystone correction system
# - Bluetooth audio support

{ config, lib, pkgs, ... }:

{
  imports = [
    # Include our custom modules
    ./modules/hy300-projector.nix
    
    # Standard NixOS modules for embedded systems
    <nixpkgs/nixos/modules/installer/sd-card/sd-image-aarch64.nix>
  ];

  # Enable HY300 projector system
  services.hy300-projector = {
    enable = true;
    
    # Hardware configuration
    hardware = {
      deviceTree = "sun50i-h713-hy300.dtb";
      bootloader = {
        enable = true;
        uBootPath = "/boot/u-boot-sunxi-with-spl.bin";
      };
      drivers = {
        enableAll = true;
        mipsCoprocessor = true;
        keystoneMotor = true;
        hdmiInput = true;
        wifiAIC8800 = true;
      };
    };
    
    # Media center configuration
    mediaCenter = {
      enable = true;
      autoStart = true;
      hardwareAcceleration = true;
      customPlugins = true;
      audioOutput = "auto";
    };
    
    # Keystone correction
    keystoneCorrection = {
      enable = true;
      autoCorrection = true;
      manualInterface = true;
      motorControl = true;
    };
    
    # Remote control
    remoteControl = {
      enable = true;
      protocol = "hy300-remote";
      wifiSetup = true;
    };
    
    # Network services
    network = {
      wifi = {
        enable = true;
        remoteSetup = true;
        profileManagement = true;
      };
      bluetooth = {
        enable = true;
        audioDevices = true;
        hidDevices = true;
        autoReconnect = true;
      };
    };
    
    # System optimization
    optimization = {
      thermalManagement = true;
      powerManagement = true;
      performanceProfile = "balanced";
    };
  };

  # System configuration
  system.stateVersion = "24.05";
  
  # Hostname
  networking.hostName = "hy300-projector";
  
  # Time zone (can be configured via setup)
  time.timeZone = "UTC";
  
  # Locale settings
  i18n.defaultLocale = "en_US.UTF-8";
  
  # Console configuration
  console = {
    font = "Lat2-Terminus16";
    keyMap = "us";
  };

  # Boot configuration for embedded system
  boot = {
    # Use latest stable kernel with H6 support
    kernelPackages = pkgs.linuxPackages_6_6;
    
    # Additional kernel parameters for HY300
    kernelParams = [
      # Serial console for debugging
      "console=ttyS0,115200"
      "earlycon=uart,mmio32,0x05000000"
      
      # Root filesystem
      "root=/dev/mmcblk0p2"
      "rootwait"
      "rootfstype=ext4"
      
      # Boot optimization
      "quiet"
      "splash"
      "plymouth.enable=0"
      "systemd.show_status=false"
      "loglevel=3"
      
      # Video and display
      "video=HDMI-A-1:1920x1080@60"
      "drm.debug=0"
      
      # Memory management
      "cma=512M"
      "mali.mali_shared_mem_size=256M"
      
      # Hardware optimization
      "allwinner.soc=h713"
      "sunxi-cpufreq.min_freq=480000"
      "sunxi-cpufreq.max_freq=1800000"
      
      # IR remote
      "lirc.default_protocol=rc-5"
      
      # Network optimization
      "net.ifnames=0"
    ];
    
    # Bootloader configuration
    loader = {
      grub.enable = false;
      generic-extlinux-compatible.enable = true;
      timeout = 1;
    };
    
    # Early boot modules
    initrd = {
      availableKernelModules = [
        # Storage
        "mmc_block"
        "sunxi-mmc"
        "ext4"
        "crc32c"
        
        # Essential hardware
        "sunxi-cpu-comm"
        "pinctrl-sun50i-h6"
        "gpio-sunxi"
      ];
      
      kernelModules = [
        "sunxi-cpu-comm"
      ];
      
      # Include firmware in initrd
      includeDefaultModules = true;
    };
  };

  # Filesystem configuration
  fileSystems = {
    "/" = {
      device = "/dev/disk/by-label/NIXOS_SD";
      fsType = "ext4";
      options = [ "defaults" "noatime" ];
    };
    
    "/boot" = {
      device = "/dev/disk/by-label/BOOT";
      fsType = "vfat";
      options = [ "defaults" "noatime" ];
    };
  };

  # Swap configuration (minimal for embedded use)
  swapDevices = [
    {
      device = "/swapfile";
      size = 1024; # 1GB swap file
    }
  ];

  # User configuration
  users = {
    # Disable mutable users for embedded system
    mutableUsers = false;
    
    # Root user with SSH key access
    users.root = {
      password = null;
      openssh.authorizedKeys.keys = [
        # Add SSH public key here for remote access
        # "ssh-rsa AAAAB3NzaC1yc2E... user@host"
      ];
    };
    
    # Main projector user
    users.projector = {
      isNormalUser = true;
      description = "HY300 Projector User";
      password = "projector"; # Change in production
      extraGroups = [ 
        "wheel" 
        "audio" 
        "video" 
        "input" 
        "gpio" 
        "i2c" 
        "dialout"
        "networkmanager"
        "bluetooth"
      ];
      shell = pkgs.bash;
      uid = 1000;
    };
  };

  # Security configuration
  security = {
    # Allow wheel group to use sudo
    sudo.wheelNeedsPassword = false;
    
    # Disable polkit for embedded use
    polkit.enable = false;
    
    # Minimal PAM configuration
    pam.loginLimits = [
      {
        domain = "*";
        type = "soft";
        item = "nofile";
        value = "65536";
      }
    ];
  };

  # System services
  services = {
    # SSH access for remote management
    openssh = {
      enable = true;
      settings = {
        PasswordAuthentication = true;
        PermitRootLogin = "yes";
      };
      ports = [ 22 ];
    };
    
    # Network time synchronization
    timesyncd.enable = true;
    
    # Hardware monitoring
    thermald.enable = false; # Use custom thermal management
    
    # Disable unnecessary services for embedded use
    udisks2.enable = false;
    packagekit.enable = false;
    
    # Enable resolved for network name resolution
    resolved = {
      enable = true;
      domains = [ "local" ];
    };
  };

  # Networking configuration
  networking = {
    # Use systemd-networkd for consistent networking
    useNetworkd = true;
    useDHCP = false;
    
    # WiFi configuration (managed by NetworkManager)
    wireless.enable = false; # Use NetworkManager instead
    
    # Firewall configuration
    firewall = {
      enable = true;
      allowedTCPPorts = [ 
        22    # SSH
        8080  # Kodi web interface
        9090  # Kodi remote control
      ];
      allowedUDPPorts = [
        5353  # mDNS
      ];
    };
    
    # mDNS for easy discovery
    hosts = {
      "127.0.0.1" = [ "hy300-projector" "hy300-projector.local" ];
    };
  };

  # System packages
  environment.systemPackages = with pkgs; [
    # Basic system tools
    coreutils
    util-linux
    procps
    psmisc
    
    # Network tools
    iproute2
    iputils
    nettools
    wget
    curl
    
    # Text editors
    nano
    vim
    
    # System monitoring
    htop
    iotop
    lsof
    
    # Hardware tools
    usbutils
    pciutils
    lshw
    
    # Development tools (for debugging)
    binutils
    file
    strace
    
    # Audio/video tools
    alsa-utils
    v4l-utils
    
    # IR remote tools
    lirc
    
    # Network configuration
    wpa_supplicant
    iw
    
    # Bluetooth tools
    bluez-tools
    
    # File management
    tree
    less
    
    # Archive tools
    unzip
    p7zip
    
    # JSON processing
    jq
  ];

  # Environment configuration
  environment = {
    # Set default variables
    variables = {
      EDITOR = "nano";
      PAGER = "less";
      
      # Hardware device paths
      HY300_MIPS_DEVICE = "/dev/mips-loader";
      HY300_KEYSTONE_DEVICE = "/dev/keystone-motor";
      HY300_IR_DEVICE = "/dev/lirc0";
      
      # Display configuration
      DISPLAY = ":0";
      
      # Mali GPU configuration
      MALI_SHARED_MEM_SIZE = "256M";
    };
    
    # Shell configuration
    shellAliases = {
      ll = "ls -l";
      la = "ls -la";
      l = "ls -CF";
      
      # HY300-specific aliases
      keystone-status = "cat /sys/class/keystone-motor/motor0/status";
      mips-status = "cat /sys/class/mips-loader/mips0/status";
      wifi-scan = "echo scan > /run/wifi-setup-control";
      kodi-restart = "systemctl restart kodi-hy300";
    };
  };

  # System optimization
  systemd = {
    # Optimize service startup times
    extraConfig = ''
      DefaultTimeoutStopSec=30s
      DefaultTimeoutStartSec=30s
    '';
    
    # Create necessary runtime directories
    tmpfiles.rules = [
      "d /var/lib/hy300 0755 root root -"
      "d /var/log/hy300 0755 root root -"
      "d /run/hy300 0755 root root -"
      "d /home/projector/.kodi 0755 projector projector -"
      "d /home/projector/.kodi/userdata 0755 projector projector -"
    ];
    
    # Network configuration
    network = {
      enable = true;
      networks = {
        # Wired ethernet (if available)
        "10-ethernet" = {
          matchConfig.Name = "eth*";
          networkConfig = {
            DHCP = "yes";
            IPv6AcceptRA = true;
          };
          dhcpV4Config = {
            RouteMetric = 10;
          };
        };
        
        # WiFi configuration (managed by NetworkManager)
        "20-wireless" = {
          matchConfig.Name = "wlan*";
          networkConfig = {
            DHCP = "yes";
            IPv6AcceptRA = true;
          };
          dhcpV4Config = {
            RouteMetric = 20;
          };
        };
      };
    };
  };

  # Hardware configuration
  hardware = {
    # Enable hardware video acceleration
    opengl = {
      enable = true;
      driSupport = true;
      driSupport32Bit = false;
      extraPackages = with pkgs; [
        mesa
        mesa.drivers
      ];
    };
    
    # Enable firmware loading
    enableRedistributableFirmware = true;
    enableAllFirmware = true;
    
    # Audio configuration
    pulseaudio = {
      enable = true;
      support32Bit = false;
      extraModules = [ pkgs.pulseaudio-modules-bt ];
      package = pkgs.pulseaudioFull;
      daemon.config = {
        flat-volumes = "no";
        default-sample-format = "s16le";
        default-sample-rate = "44100";
        alternate-sample-rate = "48000";
      };
    };
    
    # Bluetooth configuration
    bluetooth = {
      enable = true;
      powerOnBoot = true;
      settings = {
        General = {
          Enable = "Source,Sink,Media,Socket";
          MultiProfile = "multiple";
          FastConnectable = "true";
        };
      };
    };
  };

  # Image generation configuration
  sdImage = {
    # Image size (8GB)
    imageBaseName = "hy300-projector-nixos";
    
    # Compression
    compressImage = true;
    
    # Boot partition size
    firmwareSize = 128; # MB
    
    # Include U-Boot in image
    populateFirmwareCommands = ''
      # Copy device tree
      cp ${config.system.build.kernel}/dtbs/allwinner/sun50i-h713-hy300.dtb firmware/
      
      # Copy U-Boot bootloader (if available)
      if [ -f ${../../u-boot-sunxi-with-spl.bin} ]; then
        cp ${../../u-boot-sunxi-with-spl.bin} firmware/
      fi
    '';
    
    # Boot configuration
    populateRootCommands = ''
      # Create boot configuration
      mkdir -p ./files/boot/extlinux
      cat > ./files/boot/extlinux/extlinux.conf << EOF
      TIMEOUT 10
      DEFAULT nixos
      
      LABEL nixos
        MENU LABEL NixOS - HY300 Projector
        LINUX ../nixos/${config.system.build.kernel.version}/Image
        FDT ../sun50i-h713-hy300.dtb
        APPEND ${toString config.boot.kernelParams}
      EOF
      
      # Create HY300 configuration directory
      mkdir -p ./files/etc/hy300
      
      # Copy firmware files
      mkdir -p ./files/lib/firmware
      if [ -d ${../../firmware/extracted_components} ]; then
        cp -r ${../../firmware/extracted_components}/* ./files/lib/firmware/
      fi
    '';
  };

  # Development and debugging
  programs = {
    # Enable useful programs
    bash = {
      enableCompletion = true;
      enableLsColors = true;
    };
    
    # Enable nano as default editor
    nano.enable = true;
  };

  # Nix configuration for embedded system
  nix = {
    # Optimize store
    settings = {
      auto-optimise-store = true;
      allowed-users = [ "@wheel" ];
    };
    
    # Garbage collection
    gc = {
      automatic = true;
      dates = "weekly";
      options = "--delete-older-than 30d";
    };
  };

  # Documentation (minimal for embedded use)
  documentation = {
    enable = true;
    nixos.enable = false;
    man.enable = true;
    info.enable = false;
  };
}