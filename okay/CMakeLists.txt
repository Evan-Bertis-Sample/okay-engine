# okay CMakeLists.txt
# this is placed in the root of the okay engine, and
# is used to compile and link your game with the engine.

# so the directory structure looks like:
# okay/
#   CMakeLists.txt
#   core/
#     ...header files...
#     ...source files...
#   include/
#     ...
#     header files for interacting with the engine
#
# my_game/
#   okay.cmake
#   (your game files)

# Required definitions:
# - PROJECT: The name of the project
# - OKAY_PROJECT_NAME: The name of the game project
# - OKAY_PROJECT_ROOT_DIR: The root directory of the project
# - OKAY_BUILD_TYPE: Debug, Release, etc.
# - OKAY_TARGET: The target platform (x86_64, arm64, etc.)
# - OKAY_RUNTIME: The runtime (editor or player)

#
# C/CXX SETUP + OTHER BOILERPLATE
#

cmake_minimum_required(VERSION 3.13)
project(${PROJECT} C CXX)



# targets setup
add_library(okay SHARED)
add_library(okay_platform SHARED)
add_library(${PROJECT} SHARED)
add_executable(okay_runtime)

# do some initial checking
if(NOT DEFINED PROJECT)
    message(FATAL_ERROR "PROJECT not defined.")
endif()
if(NOT DEFINED OKAY_PROJECT_ROOT_DIR)
    message(FATAL_ERROR "OKAY_PROJECT_ROOT_DIR not defined.")
endif()
if(NOT EXISTS ${OKAY_PROJECT_ROOT_DIR}/okay.cmake)
    message(FATAL_ERROR "okay.cmake not found in project root directory.")
endif()

# c++ standard
set_property(TARGET okay PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 20)

target_compile_features(okay PUBLIC cxx_std_20)
target_compile_features(${PROJECT} PUBLIC cxx_std_20)

# Version guards for C++20 features
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
        message(FATAL_ERROR "GCC >= 11 required for C++20 features")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # libc++/libstdc++ version also matters, but this is a decent floor
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
        message(FATAL_ERROR "Clang >= 13 required for C++20")
    endif()
elseif(MSVC)
    # 1929 corresponds to VS 2019 16.10 (v19.29); VS 2022 is newer
    if(MSVC_VERSION LESS 1929)
        message(FATAL_ERROR "MSVC v19.29 (VS 2019 16.10) or newer required for C++20 features")
    endif()
endif()

# vendor dependencies
include(FetchContent)

#
# ADDITIONAL OKAY-SPECIFIC COMPILE FLAGS
#

# define verbosity settings for Release Builds
if(OKAY_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(okay PUBLIC -DOKAY_RELEASE)
    target_compile_definitions(okay PUBLIC -DOKAY_COMPILED_MIN_SEVERITY=2)
    target_compile_definitions(okay PUBLIC -DOKAY_COMPILED_MIN_VERBOSITY=3)
endif()


#
# ENGINE PATHS
#

set(OKAY_ENGINE_DIR ${CMAKE_SOURCE_DIR})
set(OKAY_PARENT_DIR ${OKAY_ENGINE_DIR}/..)
set(OKAY_VENDOR_DIR ${OKAY_ENGINE_DIR}/vendor)

#
# ENGINE
#

# collect core sources and headers
file(GLOB_RECURSE OKAY_CORE_SOURCES ${OKAY_ENGINE_DIR}/core/*.cpp)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB_RECURSE OKAY_CORE_HEADERS ${OKAY_ENGINE_DIR}/core/*.hpp)

target_sources(okay PRIVATE ${OKAY_CORE_SOURCES} ${OKAY_CORE_HEADERS})

# common include paths for okay
target_include_directories(okay
  PUBLIC ${OKAY_PARENT_DIR}
  PUBLIC ${OKAY_PARENT_DIR}/include
)

#
# VENDORS
#

# find OpenGL once for all platforms
find_package(OpenGL REQUIRED)

# glm (header-only)
set(OKAY_ENGINE_LIBRARIES glm)
option(GLM_SHARED "Shared build" ON)
option(GLM_STATIC "Static build" OFF)
option(GLM_USE_TEST "Enable Tests" OFF)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_subdirectory(${OKAY_VENDOR_DIR}/glm)

# freetype
add_subdirectory(${OKAY_VENDOR_DIR}/freetype)
list(APPEND OKAY_ENGINE_LIBRARIES freetype)

# IMGUI
set(IMGUI_DIR ${OKAY_VENDOR_DIR}/imgui)
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
# link glfw and OpenGL to imgui
target_link_libraries(imgui
  PUBLIC
  OpenGL::GL
)
list(APPEND OKAY_ENGINE_LIBRARIES imgui)


#
# LINKING & INCLUDES
#

# TODO: Combine together runtime & old platform target

# allow glm & glad, and stb to be included in the game
set(OKAY_INCLUDE_LIST
    ${OKAY_VENDOR_DIR}/glm
    ${OKAY_VENDOR_DIR}/glad/include
    ${OKAY_VENDOR_DIR}
)

# platform
add_subdirectory(${OKAY_ENGINE_DIR}/platform)
target_include_directories(okay_platform PUBLIC ${OKAY_INCLUDE_LIST})

# engine
target_include_directories(okay PUBLIC ${OKAY_INCLUDE_LIST})
target_link_libraries(okay PUBLIC ${OKAY_ENGINE_LIBRARIES})
target_link_libraries(okay PUBLIC okay_platform)

target_compile_definitions(okay PRIVATE OKAY_BUILD_DLL)


# runtime
add_subdirectory(${OKAY_ENGINE_DIR}/runtime)
target_include_directories(okay_runtime PUBLIC ${OKAY_INCLUDE_LIST})
target_link_libraries(okay_runtime
  PUBLIC
  okay
)

#
# GAME!
#

# compile the game
message(STATUS "Compiling Game")
include(${OKAY_PROJECT_ROOT_DIR}/okay.cmake)

get_target_property(INCLUDE_DIRS okay INCLUDE_DIRECTORIES)
message(STATUS "okay Include Directories: ${INCLUDE_DIRS}")

target_link_libraries(${PROJECT} PUBLIC okay)
target_link_libraries(okay_runtime PRIVATE ${PROJECT})


# backwards compatability with older cmake, where there was no runtime
# that owned the main() function
set_target_properties(${PROJECT} PROPERTIES
    LIBRARY_OUTPUT_NAME "game"
)

set_target_properties(okay_runtime PROPERTIES
    RUNTIME_OUTPUT_NAME "${PROJECT}"
)

target_compile_definitions(${PROJECT}
    PRIVATE OKAY_GAME_BUILD_DLL
)
