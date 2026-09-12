cmake_minimum_required(VERSION 3.16)
project(plecs_npc LANGUAGES C)

set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../code")
set(COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")
# Source order defines cross-file registration order in the linked DLL.
set(NPC_SOURCES
    app/app.c
    app/npc_platform.c
    bsp/bsp_pwm.c
    ${CODE_DIR}/interface/npc/common/pwm.c
    ${CODE_DIR}/ctrl/npc/npc_ctrl.c
    ${CODE_DIR}/ctrl/npc/npc_cfg.c
    ${CODE_DIR}/ctrl/npc/npc_fsm.c
    ${CODE_DIR}/ctrl/npc/npc_hal.c
    ${CODE_DIR}/lib/dsogi.c
    ${CODE_DIR}/lib/sogi.c
    ${CODE_DIR}/lib/svpwm_3level.c
    ${CODE_DIR}/section/baremetal/section.c
    ${COMMON_DIR}/comm/comm.c
    ${COMMON_DIR}/comm/frame_tcp_server.c
    ${COMMON_DIR}/comm/plecs_dispatch_lock.c
    ${COMMON_DIR}/dbg/shell/shell.c
    ${COMMON_DIR}/dbg/shell/shell_core.c
    ${COMMON_DIR}/dbg/shell/shell_service.c
)
set(NPC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${COMMON_DIR}
    ${COMMON_DIR}/comm
    ${COMMON_DIR}/dbg/shell
    ${CODE_DIR}/section/baremetal
    ${CODE_DIR}/data_source/comm
    ${CODE_DIR}/dbg
    ${CODE_DIR}/interface/npc/common
    ${CODE_DIR}/ctrl/npc
    ${CODE_DIR}/lib
    ${CODE_DIR}/section
)
set(NPC_WARNINGS -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef)

add_library(npc_core OBJECT ${NPC_SOURCES})
target_compile_features(npc_core PRIVATE c_std_11)
target_compile_definitions(npc_core PRIVATE PLATFORM_PLECS)
target_compile_options(npc_core PRIVATE ${NPC_WARNINGS})
target_include_directories(npc_core PRIVATE ${NPC_INCLUDES})
add_library(plecs_npc SHARED $<TARGET_OBJECTS:npc_core>)
target_link_options(plecs_npc PRIVATE "-Wl,-Map,${CMAKE_BINARY_DIR}/plecs_npc.map")
target_link_libraries(plecs_npc PRIVATE m ws2_32)
set_target_properties(plecs_npc PROPERTIES PREFIX "" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(CTest)
if(BUILD_TESTING)
    add_executable(npc_section_order_test tests/npc_section_order_test.c $<TARGET_OBJECTS:npc_core>)
    target_compile_features(npc_section_order_test PRIVATE c_std_11)
    target_compile_options(npc_section_order_test PRIVATE ${NPC_WARNINGS})
    target_compile_definitions(npc_section_order_test PRIVATE PLATFORM_PLECS)
    target_include_directories(npc_section_order_test PRIVATE ${NPC_INCLUDES})
    target_link_libraries(npc_section_order_test PRIVATE m ws2_32)
    add_test(NAME npc_section_order COMMAND npc_section_order_test)
    add_executable(npc_pwm_test tests/npc_pwm_test.c $<TARGET_OBJECTS:npc_core>)
    target_compile_features(npc_pwm_test PRIVATE c_std_11)
    target_compile_options(npc_pwm_test PRIVATE ${NPC_WARNINGS})
    target_compile_definitions(npc_pwm_test PRIVATE PLATFORM_PLECS)
    target_include_directories(npc_pwm_test PRIVATE ${NPC_INCLUDES})
    target_link_libraries(npc_pwm_test PRIVATE m ws2_32)
    set_target_properties(npc_pwm_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_test(NAME npc_pwm_chain COMMAND npc_pwm_test)
endif()
