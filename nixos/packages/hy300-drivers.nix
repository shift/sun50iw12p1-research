# HY300 Driver Packages
# Kernel modules and drivers for HY300 projector hardware

{ config, lib, pkgs, ... }:

let
  # Kernel version for module compilation
  kernelVersion = config.boot.kernelPackages.kernel.modDirVersion;
  kernelDev = config.boot.kernelPackages.kernel.dev;
  
in
{
  # MIPS co-processor driver
  hy300-mips-driver = pkgs.stdenv.mkDerivation {
    pname = "hy300-mips-driver";
    version = "1.0.0";
    
    src = ../../drivers/misc;
    
    nativeBuildInputs = with pkgs; [
      kmod
      gnumake
      which
    ];
    
    makeFlags = [
      "KVERSION=${kernelVersion}"
      "KDIR=${kernelDev}/lib/modules/${kernelVersion}/build"
      "INSTALL_MOD_PATH=$(out)"
    ];
    
    buildPhase = ''
      # Build MIPS-related drivers
      make -C ${kernelDev}/lib/modules/${kernelVersion}/build \
        M=$PWD \
        modules \
        CONFIG_SUNXI_CPU_COMM=m \
        CONFIG_SUNXI_MIPSLOADER=m \
        CONFIG_HY300_KEYSTONE_MOTOR=m
    '';
    
    installPhase = ''
      mkdir -p $out/lib/modules/${kernelVersion}/extra/hy300
      
      # Install compiled modules
      cp sunxi-cpu-comm.ko $out/lib/modules/${kernelVersion}/extra/hy300/
      cp sunxi-mipsloader.ko $out/lib/modules/${kernelVersion}/extra/hy300/
      cp hy300-keystone-motor.ko $out/lib/modules/${kernelVersion}/extra/hy300/
      
      # Install module configuration
      mkdir -p $out/etc/modprobe.d
      cat > $out/etc/modprobe.d/hy300-drivers.conf << EOF
      # HY300 Driver Configuration
      
      # MIPS co-processor driver
      alias sunxi-cpu-comm hy300-mips-comm
      options sunxi-cpu-comm debug=0
      
      # MIPS loader driver
      alias sunxi-mipsloader hy300-mips-loader
      options sunxi-mipsloader firmware_path=/lib/firmware/display.bin
      
      # Keystone motor driver
      alias hy300-keystone-motor hy300-keystone
      options hy300-keystone-motor steps_per_degree=10
      EOF
      
      # Install udev rules
      mkdir -p $out/etc/udev/rules.d
      cat > $out/etc/udev/rules.d/99-hy300-drivers.rules << EOF
      # HY300 Hardware Device Rules
      
      # MIPS co-processor device
      KERNEL=="mips-loader", GROUP="video", MODE="0664", TAG+="uaccess"
      
      # Keystone motor device  
      KERNEL=="keystone-motor", GROUP="video", MODE="0664", TAG+="uaccess"
      
      # Create symlinks for easy access
      KERNEL=="mips-loader", SYMLINK+="hy300/mips"
      KERNEL=="keystone-motor", SYMLINK+="hy300/keystone"
      EOF
    '';
    
    meta = with lib; {
      description = "HY300 MIPS co-processor and keystone motor drivers";
      license = licenses.gpl2Only;
      platforms = platforms.linux;
      maintainers = [ "HY300 Project" ];
    };
  };

  # HDMI input driver
  hy300-hdmi-driver = pkgs.stdenv.mkDerivation {
    pname = "hy300-hdmi-driver";
    version = "1.0.0";
    
    src = ../../drivers/media/platform/sunxi;
    
    nativeBuildInputs = with pkgs; [
      kmod
      gnumake
      which
    ];
    
    makeFlags = [
      "KVERSION=${kernelVersion}"
      "KDIR=${kernelDev}/lib/modules/${kernelVersion}/build"
      "INSTALL_MOD_PATH=$(out)"
    ];
    
    buildPhase = ''
      # Build HDMI input capture driver
      make -C ${kernelDev}/lib/modules/${kernelVersion}/build \
        M=$PWD \
        modules \
        CONFIG_VIDEO_SUNXI_TVCAP=m
    '';
    
    installPhase = ''
      mkdir -p $out/lib/modules/${kernelVersion}/extra/hy300
      
      # Install HDMI input driver
      cp sunxi-tvcap-enhanced.ko $out/lib/modules/${kernelVersion}/extra/hy300/
      
      # Install V4L2 configuration
      mkdir -p $out/etc/modprobe.d
      cat > $out/etc/modprobe.d/hy300-hdmi.conf << EOF
      # HY300 HDMI Input Configuration
      
      # TV capture driver
      alias sunxi-tvcap-enhanced hy300-hdmi-input
      options sunxi-tvcap-enhanced default_format=YUYV default_width=1920 default_height=1080
      EOF
      
      # Install udev rules for HDMI input
      mkdir -p $out/etc/udev/rules.d
      cat > $out/etc/udev/rules.d/99-hy300-hdmi.rules << EOF
      # HY300 HDMI Input Device Rules
      
      # HDMI input capture device
      KERNEL=="video[0-9]*", ATTRS{name}=="sunxi-tvcap*", GROUP="video", MODE="0664", TAG+="uaccess"
      KERNEL=="video[0-9]*", ATTRS{name}=="sunxi-tvcap*", SYMLINK+="hy300/hdmi-input"
      EOF
    '';
    
    meta = with lib; {
      description = "HY300 HDMI input capture driver";
      license = licenses.gpl2Only;
      platforms = platforms.linux;
      maintainers = [ "HY300 Project" ];
    };
  };

  # Combined driver package
  hy300-all-drivers = pkgs.symlinkJoin {
    name = "hy300-all-drivers";
    paths = [
      config.hy300-mips-driver
      config.hy300-hdmi-driver
    ];
    
    postBuild = ''
      # Create combined module loading script
      mkdir -p $out/bin
      cat > $out/bin/hy300-load-drivers << EOF
      #!/bin/bash
      # HY300 Driver Loading Script
      
      echo "Loading HY300 hardware drivers..."
      
      # Load MIPS drivers
      modprobe sunxi-cpu-comm
      modprobe sunxi-mipsloader
      modprobe hy300-keystone-motor
      
      # Load HDMI input driver
      modprobe sunxi-tvcap-enhanced
      
      echo "HY300 drivers loaded successfully"
      EOF
      
      chmod +x $out/bin/hy300-load-drivers
      
      # Create driver unload script
      cat > $out/bin/hy300-unload-drivers << EOF
      #!/bin/bash
      # HY300 Driver Unloading Script
      
      echo "Unloading HY300 hardware drivers..."
      
      # Unload in reverse order
      rmmod sunxi-tvcap-enhanced 2>/dev/null || true
      rmmod hy300-keystone-motor 2>/dev/null || true
      rmmod sunxi-mipsloader 2>/dev/null || true
      rmmod sunxi-cpu-comm 2>/dev/null || true
      
      echo "HY300 drivers unloaded"
      EOF
      
      chmod +x $out/bin/hy300-unload-drivers
    '';
  };
}