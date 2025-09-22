# HY300 VM Configuration for Local Testing
# This configuration emulates the HY300 projector environment on x86_64 for development

{ config, lib, pkgs, ... }:

{
  # System configuration
  system.stateVersion = "24.05";

  # VM-specific user configuration
  users.users.hy300 = {
    isNormalUser = true;
    description = "HY300 Test User";
    password = "test123";  # For VM testing only
    extraGroups = [ "wheel" "audio" "video" "networkmanager" ];
  };

  # Enable automatic login for easy VM testing
  services.getty.autologinUser = "hy300";

  # Basic packages for testing
  environment.systemPackages = with pkgs; [
    # Media center
    kodi
    
    # Media files for testing
    ffmpeg
    vlc
    
    # Network testing tools
    curl
    wget
    
    # Development tools
    htop
    neofetch
    tree
    
    # Testing utilities
    stress-ng
    
    # Browser for web interface testing
    firefox
  ];

  # Enable services useful for testing
  services = {
    # SSH for remote access
    openssh = {
      enable = true;
      settings.PasswordAuthentication = true;  # For VM testing
    };
    
    # Web server for hosting test content
    nginx = {
      enable = true;
      virtualHosts."localhost" = {
        root = "/var/www";
        locations."/" = {
          index = "index.html";
        };
      };
    };
  };

  # Enable Kodi service
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

  # Networking configuration
  networking = {
    hostName = "hy300-vm";
    firewall = {
      enable = true;
      allowedTCPPorts = [ 22 80 8080 ];
    };
    networkmanager.enable = true;
  };

  # Graphics and audio for VM
  hardware = {
    opengl = {
      enable = true;
      driSupport = true;
    };
    pulseaudio = {
      enable = true;
      support32Bit = true;
    };
  };

  # X11 configuration for Kodi
  services.xserver = {
    enable = true;
    displayManager = {
      autoLogin = {
        enable = true;
        user = "hy300";
      };
      # Use LightDM for simplicity
      lightdm.enable = true;
    };
    
    # Use a simple window manager
    windowManager.openbox.enable = true;
    desktopManager.xterm.enable = false;
  };

  # Boot configuration for VM
  boot = {
    kernelPackages = pkgs.linuxPackages_latest;
    
    # VM-friendly kernel parameters
    kernelParams = [
      "quiet"
      "splash"
    ];
    
    loader = {
      grub.enable = true;
      grub.device = "/dev/vda";
    };
  };

  # File system for VM
  fileSystems."/" = {
    device = "/dev/disk/by-label/nixos";
    fsType = "ext4";
  };
}