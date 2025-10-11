# Kodi PVR HDMI Input Addon for HY300
{ lib, stdenv, cmake, pkg-config, kodi, libv4l }:

stdenv.mkDerivation rec {
  pname = "kodi-pvr-hdmi-input";
  version = "1.0.0";

  src = ../../pvr.hdmi-input;

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    kodi.dev or kodi
    libv4l
  ];

  cmakeFlags = [
    "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  meta = with lib; {
    description = "Kodi PVR addon for HY300 HDMI input capture";
    longDescription = ''
      PVR client addon for Kodi that integrates HY300 HDMI input capture
      functionality. Provides HDMI input as a live TV channel in Kodi,
      using V4L2 capture driver for real-time streaming.
    '';
    homepage = "https://github.com/hy300/android_projector";
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    maintainers = [ ];
  };
}
