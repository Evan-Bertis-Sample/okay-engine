# okay/runtime/CMakeLists.txt

if (NOT DEFINED OKAY_RUNTIME)
    message(STATUS "OKAY_RUNTIME Not set! Defaulting to 'Editor'")
    set(OKAY_RUNTIME "editor")
endif()

target_include_directories(okay_runtime
  PUBLIC ${OKAY_PARENT_DIR}
  PUBLIC ${OKAY_PARENT_DIR}/include
  PUBLIC ${OKAY_VENDOR_DIR}/dynalo/include
  PUBLIC ${OKAY_INCLUDE_LIST}
)

set(OKAY_RUNTIME_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR})

target_sources(okay_runtime PRIVATE
    ${OKAY_RUNTIME_COMMON_DIR}/runtime.cpp
    ${OKAY_RUNTIME_COMMON_DIR}/runtime.hpp
)

target_compile_definitions(okay_runtime PUBLIC -DOKAY_GAME_NAME="${PROJECT}")

add_subdirectory(${OKAY_RUNTIME})
