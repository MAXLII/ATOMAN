cmake_minimum_required(VERSION 3.10)
project(plecs_frame_bridge LANGUAGES C)

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../code)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${PLECS_COMMON_DIR}/comm.c
    ${CODE_DIR}/app/demo/demo_shell.c
    ${PLECS_PROJECT_DIR}/app/frame_bridge_app.c
    ${PLECS_PROJECT_DIR}/comm/frame_tcp_server.c
    ${PLECS_COMMON_DIR}/dbg/scope.c
    ${PLECS_COMMON_DIR}/dbg/scope_core.c
    ${PLECS_COMMON_DIR}/dbg/scope_service.c
    ${PLECS_COMMON_DIR}/dbg/section_list_service.c
    ${PLECS_COMMON_DIR}/dbg/sfra.c
    ${PLECS_COMMON_DIR}/dbg/sfra_core.c
    ${PLECS_COMMON_DIR}/dbg/sfra_service.c
    ${PLECS_COMMON_DIR}/dbg/perf.c
    ${PLECS_COMMON_DIR}/dbg/perf_core.c
    ${PLECS_COMMON_DIR}/dbg/perf_service.c
    ${PLECS_COMMON_DIR}/dbg/shell.c
    ${PLECS_COMMON_DIR}/dbg/shell_core.c
    ${PLECS_COMMON_DIR}/dbg/shell_service.c
    ${PLECS_COMMON_DIR}/dbg/trace.c
    ${PLECS_COMMON_DIR}/dbg/trace_core.c
    ${PLECS_COMMON_DIR}/dbg/trace_service.c
    ${CODE_DIR}/lib/record_dict.c
    ${CODE_DIR}/lib/dft.c
    ${CODE_DIR}/section/baremetal/section.c
)

add_library(plecs SHARED ${SOURCES})

target_compile_features(plecs PRIVATE c_std_11)
target_compile_definitions(plecs PRIVATE IS_PLECS TOOLCHAIN_GCC __RAM_FUNC=)
target_compile_options(plecs PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wcast-align
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wundef
)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/comm
    ${PLECS_COMMON_DIR}
    ${PLECS_COMMON_DIR}/dbg
    ${CODE_DIR}/comm
    ${CODE_DIR}/interface/common
    ${CODE_DIR}/lib
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
)

target_link_libraries(plecs PRIVATE ws2_32)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(plecs PROPERTIES
    OUTPUT_NAME "plecs"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(WIN32)
    set_target_properties(plecs PROPERTIES
        LINK_FLAGS "-Wl,-Map,${CMAKE_BINARY_DIR}/bin/plecs.map"
    )
endif()
