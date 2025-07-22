# okay CMakeLists.txt
# this is placed in the root of the okay engine, and
# is used to compile and link your game with the engine.

# so the the directory structure looks kinda like:
# okay/
#   CMakeLists.txt
#   core/
#     ...header files...
#     ...source files...
#   include/
#     ...
#     header files for interacting with the engine

# somewhere else in your computer
# my_game/
#   okay.cmake
#   whatever files you have

# Required Definitions:
# - PROJECT: The name of the project
# - OKAY_PROJECT_NAME: The name of the game project.
#   This is used to set the name of the executable.
# - OKAY_PROJECT_ROOT_DIR: The root directory of the project,
#   this is where the okay project (the game) is located.
#   Remember that the game is separate from the engine.
# - OKAY_BUILD_TYPE: The build type (Debug, Release, etc.)
#   This is used to set the build type for the game.
# - OKAY_TARGET: The target platform (e.g. x86_64, arm64, etc.)


# --- CMAKE SETUP ---

cmake_minimum_required(VERSION 3.13)

project(${PROJECT} C CXX)

add_executable(${PROJECT})

# --- Do some initial checking ---

if(NOT DEFINED PROJECT)
    message(FATAL_ERROR "PROJECT not defined.")
endif()

if(NOT DEFINED OKAY_PROJECT_ROOT_DIR)
    message(FATAL_ERROR "OKAY_PROJECT_ROOT_DIR not defined.")
endif()

# check there is a okay.cmake file in the project root directory
if(NOT EXISTS ${OKAY_PROJECT_ROOT_DIR}/okay.cmake)
    message(FATAL_ERROR "okay.cmake not found in project root directory.")
endif()


# --- COMPILING OKAY ENGINE ---

message(STATUS "Compiling okay Engine")

set(OKAY_ENGINE_DIR ${CMAKE_SOURCE_DIR})

message(STATUS "Okay Engine Directory: ${OKAY_ENGINE_DIR}")

# collect all core .c and .h files recursively
file(GLOB_RECURSE OKAY_CORE_SOURCES ${OKAY_ENGINE_DIR}/core/*.cpp)
file(GLOB_RECURSE OKAY_CORE_HEADERS ${OKAY_ENGINE_DIR}/core/*.hpp)

# print out the files
message(STATUS "okay Core Sources: ${OKAY_CORE_SOURCES}")
message(STATUS "okay Core Headers: ${OKAY_CORE_HEADERS}")

add_library(okay STATIC 
    ${OKAY_CORE_SOURCES} 
    ${OKAY_CORE_HEADERS}
)

# sets where the okay engine is located, and how to include it
# I think that being a bit more explicit about the include directories is best
# so you have to do <okay/...> instead of just <...>
# or if you just want to do <...> just place the inlcude file
# in okay/include
set(OKAY_PARENT_DIR ${OKAY_ENGINE_DIR}/..)

# set okay to be a c99 library
set_property(TARGET okay PROPERTY C_STANDARD 99)

# --- Adding dependencies -- OpenGL, GLM, GLFW ---

# GLM & GLFW are added as git submodules in the vendor directory

include(FetchContent)

set(OKAY_ENGINE_VENDOR_DIR ${OKAY_ENGINE_DIR}/vendor)

message(STATUS "Vendor Directory: ${OKAY_ENGINE_VENDOR_DIR}")

# now add GLFW and GLM

# array of libraries to be added
set(OKAY_ENGINE_LIBRARIES
    glm
)

# glm
option(GLM_SHARED "Shared build" ON)
option(GLM_STATIC "Static build" OFF)
option(GLM_USE_TEST "Enable Tests" OFF) # for make check - make test
add_subdirectory(${OKAY_ENGINE_VENDOR_DIR}/glm)


# glfw (only if Windows)
if (WIN32)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only")
    set(GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only")
    set(GLAD_GL "")
    set(GLFW_LIBRARY_TYPE "STATIC")
    add_subdirectory(${OKAY_ENGINE_VENDOR_DIR}/glfw)

    # add to the list of libraries
    list(APPEND OKAY_ENGINE_LIBRARIES glfw)
else()
    message(STATUS "Skipping GLFW on non-Windows platform")
endif()

# Include directories
target_include_directories(okay 
    PUBLIC ${OKAY_PARENT_DIR}
    PUBLIC ${OKAY_PARENT_DIR}/include
)

target_link_libraries(okay 
    PUBLIC ${OKAY_ENGINE_LIBRARIES}
)

# --- COMPILING GAME ---

message(STATUS "Compiling Game")
include(${OKAY_PROJECT_ROOT_DIR}/okay.cmake)

# link the game with the engine
target_link_libraries(${PROJECT} PUBLIC okay)

get_target_property(INCLUDE_DIRS okay INCLUDE_DIRECTORIES)
message(STATUS "okay Include Directories: ${INCLUDE_DIRS}")
