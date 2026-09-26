cmake_minimum_required(VERSION 3.16)
project(plecs_chb LANGUAGES C)

include("${CMAKE_CURRENT_LIST_DIR}/../../gcc_float_policy.cmake")

set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../code")
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")
set(CHB_SOURCES
    ${COMMON_DIR}/plecs.c
    app/app.c
    bsp/bsp_pwm.c
    bsp/bsp_tcp.c
    ${CODE_DIR}/interface/chb/common/pwm.c
    ${CODE_DIR}/interface/chb/sim/comm_link.c
    ${CODE_DIR}/app/chb/chb_protect.c
    ${CODE_DIR}/ctrl/chb/chb_cfg.c
    ${CODE_DIR}/ctrl/chb/chb_hal.c
    ${CODE_DIR}/ctrl/chb/chb_fsm.c
    ${CODE_DIR}/ctrl/chb/chb_ctrl.c
    ${CODE_DIR}/lib/rly_on.c
    ${CODE_DIR}/lib/sogi.c
    ${CODE_DIR}/lib/notch.c
    ${CODE_DIR}/section/baremetal/section.c
    ${CODE_DIR}/sim/protocol/comm.c
    ${CODE_DIR}/sim/protocol/sim_discovery.c
    ${CODE_DIR}/sim/comm/sim_tcp.c
    ${CODE_DIR}/sim/debug/shell/shell.c
    ${CODE_DIR}/sim/debug/shell/shell_core.c
    ${CODE_DIR}/sim/debug/shell/shell_service.c
)
set(CHB_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${COMMON_DIR}
    ${CODE_DIR}/ctrl/chb
    ${CODE_DIR}/app/chb
    ${CODE_DIR}/interface/chb/common
    ${CODE_DIR}/sim/protocol
    ${CODE_DIR}/sim/comm
    ${CODE_DIR}/sim/debug/shell
    ${CODE_DIR}/data_source/comm
    ${CODE_DIR}/data_source/demo
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
    ${CODE_DIR}/dbg
    ${CODE_DIR}/lib
)
set(CHB_WARNINGS -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef)

add_library(plecs_chb SHARED ${CHB_SOURCES})
target_compile_features(plecs_chb PRIVATE c_std_11)
target_compile_definitions(plecs_chb PRIVATE PLATFORM_PLECS HOST_ADDR=0x02)
target_compile_options(plecs_chb PRIVATE ${CHB_WARNINGS})
target_include_directories(plecs_chb PRIVATE ${CHB_INCLUDES})
target_link_options(plecs_chb PRIVATE "-Wl,-Map,${CMAKE_BINARY_DIR}/plecs_chb.map")
target_link_libraries(plecs_chb PRIVATE m ws2_32)
set_target_properties(plecs_chb PROPERTIES PREFIX "" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
