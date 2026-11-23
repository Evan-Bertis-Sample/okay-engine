# okay/rhi

add_subdirectory(vk)

file(GLOB OKAY_RHI_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB OKAY_RHI_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_rhi PRIVATE
    ${OKAY_RHI_SOURCES}
    ${OKAY_RHI_HEADERS}
)

target_link_libraries(okay_rhi PRIVATE okay_platform)
