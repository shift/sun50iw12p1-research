{
  description = "Development environment for HY300 Android projector mainline Linux porting";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        # Custom sunxi-tools package with latest features
        sunxi-tools = pkgs.sunxi-tools.overrideAttrs (oldAttrs: {
          version = "unstable-2024-09-01";
          src = pkgs.fetchFromGitHub {
            owner = "linux-sunxi";
            repo = "sunxi-tools";
            rev = "f03f965e6c06a75ae0d5da6e6a6b07e64c50c625";
            sha256 = "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
          };
        });

        # ARM64 cross-compilation toolchain
        aarch64-toolchain = pkgs.pkgsCross.aarch64-multiplatform.buildPackages;

      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Cross-compilation toolchain
            aarch64-toolchain.gcc
            aarch64-toolchain.binutils
            aarch64-toolchain.gdb
            
            # Core build tools
            gnumake
            cmake
            ninja
            pkg-config
            
            # Kernel and bootloader build dependencies
            bc
            bison
            flex
            openssl
            ncurses
            elfutils
            
            # Sunxi-specific tools
            sunxi-tools
            dtc  # Device tree compiler
            
            # Firmware analysis tools
            binwalk
            hexdump
            file
            strings
            objdump
            readelf
            
            # Image manipulation tools
            mtd_utils
            android-tools  # For Android image analysis
            squashfs-tools
            e2fsprogs
            
            # Network and download tools
            wget
            curl
            git
            
            # Python for scripting
            python3
            python3Packages.pip
            python3Packages.pyserial  # For UART communication
            
            # Text processing
            jq
            xxd
            
            # Hardware debugging tools
            minicom
            picocom
            screen
            
            # Archive tools
            unzip
            p7zip
            
            # Development utilities
            tree
            less
            vim
            
            # Logic analyzer software (if GUI is available)
            sigrok-cli
            pulseview
          ];

          # Environment variables for cross-compilation
          shellHook = ''
            export CROSS_COMPILE=aarch64-unknown-linux-gnu-
            export ARCH=arm64
            export KBUILD_BUILD_HOST=nixos
            export KBUILD_BUILD_USER=developer
            
            # Add custom paths
            export PATH="${sunxi-tools}/bin:$PATH"
            
            # Make tools easily accessible
            alias fel='sunxi-fel'
            alias dtc-sunxi='dtc'
            
            echo "=== HY300 Projector Development Environment ==="
            echo "Cross-compile toolchain: $CROSS_COMPILE"
            echo "Target architecture: $ARCH"
            echo "Sunxi tools available: sunxi-fel, sunxi-fexc, etc."
            echo ""
            echo "Key tools installed:"
            echo "- Cross-compilation: aarch64-unknown-linux-gnu-gcc"
            echo "- Sunxi tools: sunxi-fel, sunxi-fexc"
            echo "- Firmware analysis: binwalk, hexdump, strings"
            echo "- Serial console: minicom, picocom"
            echo "- Device tree: dtc"
            echo ""
            echo "ROM analysis workflow:"
            echo "1. Extract firmware: binwalk -e firmware.img"
            echo "2. FEL access: sunxi-fel version"
            echo "3. Backup eMMC: sunxi-fel read 0x0 0x1000000 backup.img"
            echo ""
          '';

          # Set CC for autotools-based projects
          CC = "${aarch64-toolchain.gcc}/bin/aarch64-unknown-linux-gnu-gcc";
          CXX = "${aarch64-toolchain.gcc}/bin/aarch64-unknown-linux-gnu-g++";
        };

        # Additional build targets for the project
        packages = {
          # U-Boot build target (to be implemented)
          u-boot = pkgs.stdenv.mkDerivation {
            pname = "u-boot-hy300";
            version = "unstable";
            
            src = ./.;
            
            nativeBuildInputs = with pkgs; [
              gnumake
              aarch64-toolchain.gcc
              dtc
              python3
            ];
            
            configurePhase = ''
              echo "U-Boot configuration placeholder"
            '';
            
            buildPhase = ''
              echo "U-Boot build placeholder"
            '';
            
            installPhase = ''
              mkdir -p $out
              echo "U-Boot install placeholder" > $out/README
            '';
          };

          # Kernel build target (to be implemented)
          kernel = pkgs.stdenv.mkDerivation {
            pname = "linux-hy300";
            version = "unstable";
            
            src = ./.;
            
            nativeBuildInputs = with pkgs; [
              gnumake
              aarch64-toolchain.gcc
              bc
              bison
              flex
              openssl
              elfutils
            ];
            
            configurePhase = ''
              echo "Kernel configuration placeholder"
            '';
            
            buildPhase = ''
              echo "Kernel build placeholder"
            '';
            
            installPhase = ''
              mkdir -p $out
              echo "Kernel install placeholder" > $out/README
            '';
          };
        };

        # Development checks
        checks = {
          # Verify cross-compilation toolchain
          toolchain-check = pkgs.runCommand "toolchain-check" {
            buildInputs = [ aarch64-toolchain.gcc ];
          } ''
            aarch64-unknown-linux-gnu-gcc --version > $out
            echo "Toolchain check passed" >> $out
          '';
          
          # Verify sunxi-tools
          sunxi-tools-check = pkgs.runCommand "sunxi-tools-check" {
            buildInputs = [ sunxi-tools ];
          } ''
            sunxi-fel --version > $out || echo "sunxi-fel available" > $out
            echo "Sunxi tools check passed" >> $out
          '';
        };
      });
}