# picks a platform backend and exposes it as a real STATIC target 'okay_platform'

if(NOT DEFINED OKAY_PLATFORM)
  if(WIN32)
    set(OKAY_PLATFORM "windows")
  elseif(UNIX AND NOT APPLE)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cpu)
    if(_cpu MATCHES "arm|aarch64")
      set(OKAY_PLATFORM "rpi")
    elseif (APPLE)
      set(OKAY_PLATFORM "macos")
    else()
      set(OKAY_PLATFORM "rpi")
    endif()
  else()
    message(FATAL_ERROR "Unsupported platform; set -DOKAY_PLATFORM=<name>")
  endif()
endif()

message(STATUS "OKAY_PLATFORM = ${OKAY_PLATFORM}")

# Create a real, empty STATIC lib that the backend will add sources to.
add_library(okay_platform STATIC)

# C++ standard for platform code
set_property(TARGET okay_platform PROPERTY CXX_STANDARD 20)

# Let the backend subdir add sources/defs/libs to 'okay_platform'
add_subdirectory(${OKAY_PLATFORM})
