# Minimal HY300 NixOS Configuration for Testing
# This configuration focuses on software components that can be built without hardware

{ config, lib, pkgs, ... }:

{
  # System configuration
  system.stateVersion = "24.05";
  
  # Hostname
  networking.hostName = "hy300-projector";
  
  # Boot configuration for ARM64
  boot = {
    kernelPackages = pkgs.linuxPackages_6_6;
    
    kernelParams = [
      "console=ttyS0,115200"
      "root=/dev/mmcblk0p2"
      "rootwait"
      "quiet"
    ];
    
    loader = {
      grub.enable = false;
      generic-extlinux-compatible.enable = true;
    };
  };

  # Filesystem configuration
  fileSystems = {
    "/" = {
      device = "/dev/disk/by-label/NIXOS_SD";
      fsType = "ext4";
    };
  };

  # User configuration
  users.users.projector = {
    isNormalUser = true;
    description = "HY300 Projector User";
    extraGroups = [ "wheel" "audio" "video" ];
  };

  # Enable SSH for remote access
  services.openssh.enable = true;

  # Basic packages
  environment.systemPackages = with pkgs; [
    kodi
    alsa-utils
    networkmanager
    bluez-tools
    v4l-utils
  ];

  # Enable basic services
  services.networkmanager.enable = true;
  hardware.bluetooth.enable = true;
  hardware.pulseaudio.enable = true;

  # Enable Kodi service
  systemd.services.kodi = {
    description = "Kodi Media Center";
    after = [ "graphical-session.target" ];
    wantedBy = [ "multi-user.target" ];
    serviceConfig = {
      Type = "simple";
      User = "projector";
      ExecStart = "${pkgs.kodi}/bin/kodi --standalone";
      Restart = "always";
    };
  };

  # Minimal hardware support
  hardware.opengl.enable = true;
}