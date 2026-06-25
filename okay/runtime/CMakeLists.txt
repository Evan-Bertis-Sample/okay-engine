# okay/runtime/CMakeLists.txt

message(STATUS "Compiling runtime")

if (NOT DEFINED OKAY_RUNTIME)
    message(STATUS "OKAY_RUNTIME Not set! Defaulting to 'Editor'")
    set(OKAY_RUNTIME "editor")
endif()

target_include_directories(okay_runtime
  PUBLIC ${OKAY_PARENT_DIR}
  PUBLIC ${OKAY_PARENT_DIR}/include
  PUBLIC ${OKAY_VENDOR_DIR}/dynalo/include
  PUBLIC ${OKAY_INCLUDE_DIRS}
)

file(GLOB_RECURSE OKAY_RUNTIME_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
file(GLOB_RECURSE OKAY_RUNTIME_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.hpp)

target_sources(okay_runtime PRIVATE
    ${OKAY_RUNTIME_SOURCES}
    ${OKAY_RUNTIME_HEADERS}
)

target_compile_definitions(okay_runtime PUBLIC -DOKAY_GAME_NAME="${PROJECT}")

add_subdirectory(${OKAY_RUNTIME})
