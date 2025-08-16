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
