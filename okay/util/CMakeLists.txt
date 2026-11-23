# okay/util

message(STATUS "Compiling util")

file(GLOB_RECURSE OKAY_UTIL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_UTIL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_util PRIVATE
    ${OKAY_UTIL_SOURCES}
    ${OKAY_UTIL_HEADERS}
)
