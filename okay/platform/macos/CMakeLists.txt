# glfw (static)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW only")
set(GLFW_INSTALL    OFF CACHE BOOL "GLFW only")
set(GLFW_LIBRARY_TYPE "STATIC")
add_subdirectory(${OKAY_VENDOR_DIR}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

file(GLOB PLATFORM_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB PLATFORM_HDR CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)


# okay_platform is a real STATIC lib created in platform/CMakeLists.txt
target_sources(okay_platform PRIVATE 
  ${PLATFORM_SRC} 
  ${PLATFORM_HDR}
)

target_include_directories(okay_platform PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${OKAY_PARENT_DIR}
  ${OKAY_PARENT_DIR}/include
  ${OKAY_VENDOR_DIR}
)

target_compile_definitions(okay_platform PUBLIC __GLFW_SURFACE_H__)

# link low-level deps INTO the platform lib
target_link_libraries(okay_platform PUBLIC
  glfw
  OpenGL::GL
  -static-libstdc++
  "-framework Cocoa"
  "-framework IOKit"
  "-framework CoreVideo"
)

# Force console subsystem on Windows
target_link_options(${PROJECT} PRIVATE
  $<$<AND:$<BOOL:${WIN32}>,$<CXX_COMPILER_ID:GNU,Clang>>:-Wl,-subsystem,console>
  $<$<AND:$<BOOL:${WIN32}>,$<CXX_COMPILER_ID:MSVC>>:/SUBSYSTEM:CONSOLE>
)