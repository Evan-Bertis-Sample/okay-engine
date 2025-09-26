# platform/CMakeLists.txt
# picks a platform backend and exposes it as INTERFACE target 'okay_platform'
# set OKAY_PLATFORM to override autodetect

if(NOT DEFINED OKAY_PLATFORM)
    if(WIN32)
        set(OKAY_PLATFORM "windows")
    elseif(UNIX AND NOT APPLE)
        string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cpu)
        if(_cpu MATCHES "arm" OR _cpu MATCHES "aarch64")
            set(OKAY_PLATFORM "rpi")  # raspberry pi os lite (kms/egl)
        else()
            set(OKAY_PLATFORM "rpi")  # default for now; change later if you add linux-x11
        endif()
    else()
        message(FATAL_ERROR "Unsupported platform; set -DOKAY_PLATFORM=<name>")
    endif()
endif()

message(STATUS "OKAY_PLATFORM = ${OKAY_PLATFORM}")

add_library(okay_platform INTERFACE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${OKAY_PLATFORM})