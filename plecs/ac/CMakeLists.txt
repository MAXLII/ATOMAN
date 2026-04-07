cmake_minimum_required(VERSION 3.10)
project(plecs_ac LANGUAGES C)

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-g++.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../code)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${PLECS_PROJECT_DIR}/app/app.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_adc.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_gpio.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_pwm.c
    ${CODE_DIR}/section/section.c
    ${CODE_DIR}/lib/ac_loss_det.c
    ${CODE_DIR}/lib/cal_rms.c
    ${CODE_DIR}/lib/chk_grid.c
    ${CODE_DIR}/lib/fll.c
    ${CODE_DIR}/lib/hys_cmp.c
    ${CODE_DIR}/lib/linear.c
    ${CODE_DIR}/lib/notch.c
    ${CODE_DIR}/lib/pi_tustin.c
    ${CODE_DIR}/lib/pid.c
    ${CODE_DIR}/lib/pid_inc.c
    ${CODE_DIR}/lib/pr.c
    ${CODE_DIR}/lib/rly_on.c
    ${CODE_DIR}/lib/sogi.c
    ${CODE_DIR}/lib/z2p2.c
    ${CODE_DIR}/app/alarm.c
    ${CODE_DIR}/app/ac/adc_proc.c
    ${CODE_DIR}/interface/ac/adc.c
    ${CODE_DIR}/interface/ac/gpio.c
    ${CODE_DIR}/interface/ac/pwm.c
    ${CODE_DIR}/ctrl/pfc/pfc_cfg.c
    ${CODE_DIR}/ctrl/pfc/pfc_ctrl.c
    ${CODE_DIR}/ctrl/pfc/pfc_fsm.c
    ${CODE_DIR}/ctrl/pfc/pfc_hal.c
    ${CODE_DIR}/ctrl/inv/inv_cfg.c
    ${CODE_DIR}/ctrl/inv/inv_ctrl.c
    ${CODE_DIR}/ctrl/inv/inv_fsm.c
    ${CODE_DIR}/ctrl/inv/inv_hal.c
)

add_library(plecs SHARED ${SOURCES})

target_compile_definitions(plecs PRIVATE IS_PLECS IS_AC)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/bsp
    ${PLECS_COMMON_DIR}
    ${CODE_DIR}/lib
    ${CODE_DIR}/app
    ${CODE_DIR}/app/ac
    ${CODE_DIR}/comm
    ${CODE_DIR}/ctrl
    ${CODE_DIR}/ctrl/pfc
    ${CODE_DIR}/ctrl/inv
    ${CODE_DIR}/interface/ac
    ${CODE_DIR}/section
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties(plecs PROPERTIES
    OUTPUT_NAME "plecs"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(WIN32)
    set_target_properties(plecs PROPERTIES
        LINK_FLAGS "-Wl,-Map,${CMAKE_BINARY_DIR}/bin/mydll.map"
    )
endif()
