# platform/rpi/CMakeLists.txt
# egl + gbm + drm (KMS), defines __EGL_SURFACE_H__; still OpenGL (not GLES)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(EGL REQUIRED egl)

# include paths for the platform bits
target_include_directories(okay_platform INTERFACE
    ${GBM_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

# link dirs in case pkg-config doesn't propagate them
target_link_directories(okay_platform INTERFACE
    ${GBM_LIBRARY_DIRS}
    ${DRM_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
)

# link platform libs + OpenGL
target_link_libraries(okay_platform INTERFACE
    ${GBM_LIBRARIES}
    ${DRM_LIBRARIES}
    ${EGL_LIBRARIES}
    OpenGL::GL
)

target_compile_definitions(okay_platform INTERFACE __EGL_SURFACE_H__)

# add the .hpp and .cpp files to the okay_platform target
# use wildcards to avoid listing every file
file(GLOB_RECURSE OKAY_PLATFORM_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_PLATFORM_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)
target_sources(okay_platform INTERFACE ${OKAY_PLATFORM_SOURCES} ${OKAY_PLATFORM_HEADERS})
target_include_directories(okay_platform INTERFACE ${CMAKE_CURRENT_LIST_DIR})
