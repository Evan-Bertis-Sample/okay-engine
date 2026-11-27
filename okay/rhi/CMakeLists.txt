# okay/rhi

file(GLOB OKAY_RHI_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB OKAY_RHI_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_rhi PRIVATE
    ${OKAY_RHI_SOURCES}
    ${OKAY_RHI_HEADERS}
)

target_include_directories(okay_rhi
  PUBLIC ${OKAY_PARENT_DIR}
  PUBLIC ${OKAY_PARENT_DIR}/include
)


target_compile_definitions(okay_rhi PUBLIC
    ${VULKAN_COMPILE_FLAGS}
)

target_link_libraries(okay_rhi
    PRIVATE okay_platform okay_util
    PUBLIC Vulkan::Headers
)
