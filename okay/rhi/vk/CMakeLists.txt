# okay/rhi/vk

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE OKAY_VK_RHI_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_VK_RHI_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_rhi PRIVATE
    ${OKAY_VK_RHI_SOURCES}
    ${OKAY_VK_RHI_HEADERS}
)

target_link_libraries(okay_rhi PUBLIC
    Vulkan::Vulkan
)
