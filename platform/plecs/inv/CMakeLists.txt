cmake_minimum_required(VERSION 3.10)
project(plecs_inv LANGUAGES C)

include("${CMAKE_CURRENT_LIST_DIR}/../../gcc_float_policy.cmake")

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../code)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${CODE_DIR}/sim/protocol/comm.c
    ${CODE_DIR}/sim/comm/sim_tcp.c
    ${CODE_DIR}/sim/protocol/sim_discovery.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/bsp_tcp.c
    ${CODE_DIR}/interface/inv/sim/comm_link.c
    ${PLECS_PROJECT_DIR}/app/app.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_pwm.c
    ${CODE_DIR}/sim/debug/shell/shell.c
    ${CODE_DIR}/sim/debug/shell/shell_core.c
    ${CODE_DIR}/sim/debug/shell/shell_service.c
    ${CODE_DIR}/section/baremetal/section.c
    ${CODE_DIR}/lib/apf.c
    ${CODE_DIR}/lib/fll.c
    ${CODE_DIR}/lib/notch.c
    ${CODE_DIR}/lib/pi_tustin.c
    ${CODE_DIR}/lib/pr.c
    ${CODE_DIR}/lib/resonant.c
    ${CODE_DIR}/lib/sogi.c
    ${CODE_DIR}/ctrl/inv/inv_cfg.c
    ${CODE_DIR}/ctrl/inv/inv_ctrl.c
    ${CODE_DIR}/ctrl/inv/inv_fsm.c
    ${CODE_DIR}/ctrl/inv/inv_hal.c
)

add_library(plecs SHARED ${SOURCES})

target_compile_features(plecs PRIVATE c_std_11)
target_compile_definitions(plecs PRIVATE PLATFORM_PLECS IS_PLECS IS_INV TOOLCHAIN_GCC __RAM_FUNC=)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/bsp
    ${PLECS_COMMON_DIR}
    ${CODE_DIR}/sim/protocol
    ${CODE_DIR}/sim/comm
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${CODE_DIR}/sim/debug/shell
    ${CODE_DIR}/data_source/comm ${CODE_DIR}/data_source/demo
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
    ${CODE_DIR}/dbg
    ${CODE_DIR}/lib
    ${CODE_DIR}/ctrl
    ${CODE_DIR}/ctrl/inv
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
