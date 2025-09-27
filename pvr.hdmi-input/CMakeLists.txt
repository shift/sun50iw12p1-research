cmake_minimum_required(VERSION 3.16)
project(pvr.hdmi-input LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(kodi REQUIRED)
find_package(PkgConfig REQUIRED)

# Find V4L2
pkg_check_modules(V4L2 REQUIRED libv4l2)

# Source files
set(HDMI_PVR_SOURCES
  src/addon.cpp
  src/hdmi_client.cpp
  src/v4l2_device.cpp
  src/channel_manager.cpp
  src/stream_processor.cpp
  src/signal_monitor.cpp
)

set(HDMI_PVR_HEADERS
  src/hdmi_client.h
  src/v4l2_device.h
  src/channel_manager.h
  src/stream_processor.h
  src/signal_monitor.h
  src/types.h
)

# Build addon
build_addon(pvr.hdmi-input HDMI_PVR DEPLIBS)

# Include directories
target_include_directories(pvr.hdmi-input PRIVATE
  ${KODI_INCLUDE_DIR}
  ${V4L2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(pvr.hdmi-input PRIVATE
  ${KODI_MAIN_LIBRARY}
  ${V4L2_LIBRARIES}
  pthread
)

# Compiler flags
target_compile_options(pvr.hdmi-input PRIVATE
  ${V4L2_CFLAGS_OTHER}
  -Wall
  -Wextra
  -Werror
  -fPIC
)

# Install addon files
install(FILES addon.xml DESTINATION .)
install(FILES icon.png DESTINATION .)
install(FILES fanart.jpg DESTINATION .)
install(FILES resources/settings.xml DESTINATION resources/)
install(FILES resources/language/resource.language.en_gb/strings.po DESTINATION resources/language/resource.language.en_gb/)