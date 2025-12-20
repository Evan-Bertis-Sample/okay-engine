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

cmake_minimum_required(VERSION 3.13)
project(${PROJECT} C CXX)

add_executable(${PROJECT})

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

# engine paths
set(OKAY_ENGINE_DIR ${CMAKE_SOURCE_DIR})
set(OKAY_PARENT_DIR ${OKAY_ENGINE_DIR}/..)
set(OKAY_VENDOR_DIR ${OKAY_ENGINE_DIR}/vendor)

message(STATUS "Compiling okay Engine")
message(STATUS "Engine dir: ${OKAY_ENGINE_DIR}")
message(STATUS "Vendor dir: ${OKAY_VENDOR_DIR}")

# collect core sources and headers
file(GLOB_RECURSE OKAY_CORE_SOURCES ${OKAY_ENGINE_DIR}/core/*.cpp)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(GLOB_RECURSE OKAY_CORE_HEADERS ${OKAY_ENGINE_DIR}/core/*.hpp)

add_library(okay STATIC ${OKAY_CORE_SOURCES} ${OKAY_CORE_HEADERS})

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

# common include paths for okay
target_include_directories(okay
  PUBLIC ${OKAY_PARENT_DIR}
  PUBLIC ${OKAY_PARENT_DIR}/include
)

# find OpenGL once for all platforms
find_package(OpenGL REQUIRED)

# vendor dependencies
include(FetchContent)

# glm (header-only)
set(OKAY_ENGINE_LIBRARIES glm)
option(GLM_SHARED "Shared build" ON)
option(GLM_STATIC "Static build" OFF)
option(GLM_USE_TEST "Enable Tests" OFF)
add_subdirectory(${OKAY_VENDOR_DIR}/glm)

# platform selection (creates INTERFACE target 'okay_platform')
# set OKAY_PLATFORM to override autodetect (e.g., -DOKAY_PLATFORM=windows)
add_subdirectory(${OKAY_ENGINE_DIR}/platform)

# allow glm & glad to be included in the game
target_include_directories(okay
  PUBLIC ${OKAY_VENDOR_DIR}/glm
  # for glad/gl.h
  PUBLIC ${OKAY_VENDOR_DIR}
)

# link common libs and platform bundle into the engine
target_link_libraries(okay
  PUBLIC
  okay_platform
  ${OKAY_ENGINE_LIBRARIES}
  OpenGL::GL
)

# compile the game
message(STATUS "Compiling Game")
include(${OKAY_PROJECT_ROOT_DIR}/okay.cmake)

get_target_property(INCLUDE_DIRS okay INCLUDE_DIRECTORIES)
message(STATUS "okay Include Directories: ${INCLUDE_DIRS}")

target_link_libraries(${PROJECT} PRIVATE okay -static-libstdc++)

# root CMakeLists.txt
target_link_options(${PROJECT} PRIVATE -fuse-ld=lld)

