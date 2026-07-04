message(STATUS "Using Editor Runtime!")

file(GLOB_RECURSE OKAY_EDITOR_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_EDITOR_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_runtime PRIVATE
    ${OKAY_EDITOR_SOURCES}
    ${OKAY_EDITOR_HEADERS}
)

set(SOCKPP_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_STATIC OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SOCKPP_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)

add_subdirectory(
    ${OKAY_VENDOR_DIR}/sockpp
    ${CMAKE_CURRENT_BINARY_DIR}/sockpp_build
)

target_link_libraries(okay_runtime PUBLIC sockpp-shared)

if(WIN32)
    set_target_properties(sockpp-shared PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:okay_runtime>
    )
endif()

message(STATUS "Editor sources: ${OKAY_EDITOR_SOURCES}")
