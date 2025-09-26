# platform/windows/CMakeLists.txt
# glfw + desktop opengl, defines __GLFW_SURFACE_H__

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
set(GLFW_LIBRARY_TYPE "STATIC")

# vendor/glfw is outside this dir; add a binary dir explicitly
add_subdirectory(${OKAY_VENDOR_DIR}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# carry link libs/defs via interface target
target_link_libraries(okay_platform INTERFACE glfw OpenGL::GL)
target_compile_definitions(okay_platform INTERFACE __GLFW_SURFACE_H__)

# add the .hpp and .cpp files to the okay_platform target
# use wildcards to avoid listing every file
file(GLOB_RECURSE OKAY_PLATFORM_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_PLATFORM_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)
target_sources(okay_platform INTERFACE ${OKAY_PLATFORM_SOURCES} ${OKAY_PLATFORM_HEADERS})
target_include_directories(okay_platform INTERFACE ${CMAKE_CURRENT_LIST_DIR})