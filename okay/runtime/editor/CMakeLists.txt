# runtime/editor/CMakeLists.txt

message(STATUS "Using Editor Runtime!")

file(GLOB_RECURSE OKAY_EDITOR_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_EDITOR_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_runtime PRIVATE
    ${OKAY_EDITOR_SOURCES}
    ${OKAY_EDITOR_HEADERS}
)

message(STATUS "Editor sources: ${OKAY_EDITOR_SOURCES}")
