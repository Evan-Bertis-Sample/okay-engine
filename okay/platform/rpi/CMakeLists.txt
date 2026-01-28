# platform/rpi/CMakeLists.txt
# egl + gbm + drm (KMS), defines __EGL_SURFACE_H__; still OpenGL (not GLES)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(EGL REQUIRED egl)

target_include_directories()(okay_platform PRIVATE
    ${GBM_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${OKAY_PARENT_DIR}
    ${OKAY_PARENT_DIR}/include
    ${OKAY_VENDOR_DIR}
)

# link platform libs + OpenGL
target_link_libraries(okay_platform PRIVATE
    ${GBM_LIBRARIES}
    ${DRM_LIBRARIES}
    ${EGL_LIBRARIES}
    OpenGL::GL
)

target_compile_definitions(okay_platform PRIVATE __EGL_SURFACE_H__)

# add the .hpp and .cpp files to the okay_platform target
# use wildcards to avoid listing every file
file(GLOB_RECURSE OKAY_PLATFORM_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_PLATFORM_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)
target_sources(okay_platform PRIVATE ${OKAY_PLATFORM_SOURCES} ${OKAY_PLATFORM_HEADERS})
target_include_directories(okay_platform PRIVATE ${CMAKE_CURRENT_LIST_DIR})
