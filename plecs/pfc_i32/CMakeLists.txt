cmake_minimum_required(VERSION 3.10)
project(plecs_pfc_i32 LANGUAGES C)

set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-g++.exe")

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../code)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${PLECS_PROJECT_DIR}/app/app.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_pwm.c
    ${CODE_DIR}/section/section.c
    ${CODE_DIR}/lib/cal_rms.c
    ${CODE_DIR}/lib/pi_tustin_i32.c
    ${CODE_DIR}/ctrl/pfc_i32/pfc_cfg.c
    ${CODE_DIR}/ctrl/pfc_i32/pfc_ctrl.c
    ${CODE_DIR}/ctrl/pfc_i32/pfc_hal.c
)

add_library(plecs SHARED ${SOURCES})

target_compile_features(plecs PRIVATE c_std_11)
target_compile_definitions(plecs PRIVATE IS_PLECS IS_PFC_I32 TOOLCHAIN_GCC __RAM_FUNC=)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/bsp
    ${PLECS_COMMON_DIR}
    ${CODE_DIR}/section
    ${CODE_DIR}/lib
    ${CODE_DIR}/ctrl
    ${CODE_DIR}/ctrl/pfc_i32
)

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
