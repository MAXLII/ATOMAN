cmake_minimum_required(VERSION 3.10)
project(plecs_cllc LANGUAGES C)

set(PLECS_PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PLECS_COMMON_DIR ${CMAKE_CURRENT_LIST_DIR}/../common)
set(CODE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../code)

set(CLLC_PROJECT_SOURCES
    ${PLECS_PROJECT_DIR}/app/app.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_adc.c
    ${PLECS_PROJECT_DIR}/bsp/bsp_pwm.c
    ${CODE_DIR}/interface/cllc/adc.c
    ${CODE_DIR}/interface/cllc/pwm.c
    ${CODE_DIR}/lib/pi_dual_compete.c
    ${CODE_DIR}/lib/pi_tustin.c
    ${CODE_DIR}/lib/pr.c
    ${CODE_DIR}/ctrl/cllc/cllc_cfg.c
    ${CODE_DIR}/ctrl/cllc/cllc_ctrl.c
    ${CODE_DIR}/ctrl/cllc/cllc_fsm.c
    ${CODE_DIR}/ctrl/cllc/cllc_hal.c
)

set(SOURCES
    ${PLECS_COMMON_DIR}/plecs.c
    ${CODE_DIR}/section/baremetal/section.c
    ${CLLC_PROJECT_SOURCES}
)

add_library(plecs SHARED ${SOURCES})

target_compile_features(plecs PRIVATE c_std_11)
target_compile_definitions(plecs PRIVATE
    IS_PLECS
    IS_CLLC
    TOOLCHAIN_GCC
    __RAM_FUNC=
)

target_include_directories(plecs PRIVATE
    ${PLECS_PROJECT_DIR}
    ${PLECS_PROJECT_DIR}/app
    ${PLECS_PROJECT_DIR}/bsp
    ${PLECS_COMMON_DIR}
    ${CODE_DIR}/interface/cllc
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/section
    ${CODE_DIR}/dbg
    ${CODE_DIR}/lib
    ${CODE_DIR}/ctrl
    ${CODE_DIR}/ctrl/cllc
)

# Apply the repository strict-warning policy to all newly added CLLC-owned sources.
set_source_files_properties(${CLLC_PROJECT_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-Wpedantic;-Wconversion;-Wsign-conversion;-Wshadow;-Wcast-align;-Wstrict-prototypes;-Wmissing-prototypes;-Wundef"
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
