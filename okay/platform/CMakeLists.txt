# picks a platform backend and exposes it as a real STATIC target 'okay_platform'

if(NOT DEFINED OKAY_PLATFORM)
    if(WIN32)
        set(OKAY_PLATFORM "windows")
    elseif(UNIX AND NOT APPLE)
        set(OKAY_PLATFORM "rpi")
    elseif(APPLE)
        set(OKAY_PLATFORM "macos")
    else()
        message(FATAL_ERROR "Unsupported platform; set -DOKAY_PLATFORM=<name>")
    endif()
endif()

message(STATUS "OKAY_PLATFORM = ${OKAY_PLATFORM}")

# C++ standard for platform code
set_property(TARGET okay_platform PROPERTY CXX_STANDARD 20)

# add to the sources of okay_platform glad.c
target_sources(okay_platform PRIVATE
  ${OKAY_VENDOR_DIR}/glad/src/glad.c
)

target_include_directories(okay_platform PRIVATE
 ${OKAY_VENDOR_DIR}/glad/include
)

# sockpp

set(SOCKPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)

add_subdirectory(
    ${OKAY_VENDOR_DIR}/sockpp
    ${CMAKE_CURRENT_BINARY_DIR}/sockpp_build
)

target_link_libraries(okay_platform PUBLIC sockpp-static)

# if(WIN32)
#     set_target_properties(sockpp-static PROPERTIES
#         RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:okay_runtime>
#     )
# endif()

# Let the backend subdir add sources/defs/libs to 'okay_platform'
add_subdirectory(${OKAY_PLATFORM})
