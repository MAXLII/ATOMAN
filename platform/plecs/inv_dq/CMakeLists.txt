cmake_minimum_required(VERSION 3.10)
project(plecs_inv_dq LANGUAGES C)

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../code)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${PLECS_COMMON_DIR}/comm/comm.c
    ${PLECS_COMMON_DIR}/comm/frame_tcp_server.c
    ${PLECS_PROJECT_DIR}/app/app.c
    ${PLECS_PROJECT_DIR}/app/inv_dq_platform.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_pwm.c
    ${PLECS_COMMON_DIR}/dbg/shell/shell.c
    ${PLECS_COMMON_DIR}/dbg/shell/shell_core.c
    ${PLECS_COMMON_DIR}/dbg/shell/shell_service.c
    ${CODE_DIR}/section/baremetal/section.c
    ${CODE_DIR}/lib/apf.c
    ${CODE_DIR}/lib/fll.c
    ${CODE_DIR}/lib/notch.c
    ${CODE_DIR}/lib/pi_tustin.c
    ${CODE_DIR}/lib/pr.c
    ${CODE_DIR}/lib/resonant.c
    ${CODE_DIR}/lib/sogi.c
    ${CODE_DIR}/ctrl/inv_dq/inv_cfg.c
    ${CODE_DIR}/ctrl/inv_dq/inv_ctrl.c
    ${CODE_DIR}/ctrl/inv_dq/inv_fsm.c
    ${CODE_DIR}/ctrl/inv_dq/inv_hal.c
)

add_library(plecs SHARED ${SOURCES})

target_compile_features(plecs PRIVATE c_std_11)
target_compile_definitions(plecs PRIVATE IS_PLECS IS_INV_DQ TOOLCHAIN_GCC __RAM_FUNC=)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/bsp
    ${PLECS_COMMON_DIR}
    ${PLECS_COMMON_DIR}/comm
    ${PLECS_COMMON_DIR}/dbg/shell
    ${CODE_DIR}/comm
    ${CODE_DIR}/interface/common
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
    ${CODE_DIR}/dbg
    ${CODE_DIR}/lib
    ${CODE_DIR}/ctrl
    ${CODE_DIR}/ctrl/inv_dq
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
