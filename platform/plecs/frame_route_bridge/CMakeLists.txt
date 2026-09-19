cmake_minimum_required(VERSION 3.10)
project(plecs_frame_route_bridge LANGUAGES C)

include("${CMAKE_CURRENT_LIST_DIR}/../../gcc_float_policy.cmake")

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../code)

set(ROUTE_NODE_COMMON_SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${CODE_DIR}/sim/protocol/comm.c
    ${CODE_DIR}/sim/comm/sim_tcp.c
    ${CODE_DIR}/sim/protocol/sim_discovery.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/bsp_tcp.c
    ${CODE_DIR}/interface/frame_route_bridge/sim/comm_link.c
    ${CODE_DIR}/sim/debug/shell/shell.c
    ${CODE_DIR}/sim/debug/shell/shell_core.c
    ${CODE_DIR}/sim/debug/shell/shell_service.c
    ${PLECS_PROJECT_DIR}/app/route_bridge_app.c
    ${PLECS_PROJECT_DIR}/app/route_bridge_platform.c
    ${CODE_DIR}/section/baremetal/section.c
)

set(ROUTE_NODE_INCLUDE_DIRS
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_COMMON_DIR}
    ${CODE_DIR}/sim/protocol
    ${CODE_DIR}/sim/comm
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${CODE_DIR}/sim/debug/perf
    ${CODE_DIR}/sim/debug/shell
    ${CODE_DIR}/data_source/comm ${CODE_DIR}/data_source/demo
    ${CODE_DIR}/lib
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
)

set(ROUTE_NODE_WARNING_FLAGS
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

function(configure_route_node
         target_name
         output_name
         node_addr
         peer_role
         frame_port
         discovery_enabled
         discovery_peer_addr
         discovery_peer_port)
    target_compile_features(${target_name} PRIVATE c_std_11)
    target_compile_definitions(${target_name} PRIVATE
        PLATFORM_PLECS
        IS_PLECS
        TOOLCHAIN_GCC
        __RAM_FUNC=
        HOST_ADDR=${node_addr}
        BSP_TCP_NODE_ADDR=${node_addr}
        BSP_TCP_PEER_ROLE=${peer_role}
        BSP_TCP_FRAME_PORT=${frame_port}
        BSP_TCP_DISCOVERY_ENABLED=${discovery_enabled}
        BSP_TCP_DISCOVERY_PEER_ADDR=${discovery_peer_addr}
        BSP_TCP_DISCOVERY_PEER_PORT=${discovery_peer_port}
    )
    target_compile_options(${target_name} PRIVATE ${ROUTE_NODE_WARNING_FLAGS})
    target_include_directories(${target_name} PRIVATE ${ROUTE_NODE_INCLUDE_DIRS})
    target_link_libraries(${target_name} PRIVATE ws2_32)
    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${output_name}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${output_name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${output_name}
        LINK_FLAGS "-Wl,-Map,${CMAKE_BINARY_DIR}/bin/${output_name}/${output_name}.map"
    )
endfunction()

add_library(plecs_node02 SHARED ${ROUTE_NODE_COMMON_SOURCES})
configure_route_node(plecs_node02 plecs_node02 0x02 1 5000 1 0x03 5002)

add_library(plecs_node03 SHARED ${ROUTE_NODE_COMMON_SOURCES})
configure_route_node(plecs_node03 plecs_node03 0x03 2 5002 0 0 0)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plecs_node02)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plecs_node03)
