cmake_minimum_required(VERSION 3.16)
project(matlab_frame_route_bridge LANGUAGES C)

include("${CMAKE_CURRENT_LIST_DIR}/../../gcc_float_policy.cmake")
file(TO_CMAKE_PATH "${MATLAB_ROOT}" MATLAB_ROOT)
if(NOT EXISTS "${MATLAB_ROOT}/extern/include/mex.h")
    message(FATAL_ERROR "MATLAB_ROOT must select the MATLAB installation used to load these MEX files.")
endif()
set(CODE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../code")
set(COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/../common")
set(MATLAB_LIB_DIR "${MATLAB_ROOT}/extern/lib/win64/mingw64")
set(SOURCES
    ${COMMON_DIR}/sim_sfunc.c
    route_bridge_app.c
    ${CODE_DIR}/sim/comm/sim_tcp.c
    ${CODE_DIR}/sim/protocol/sim_discovery.c
    ${CODE_DIR}/sim/protocol/comm.c
    ${CODE_DIR}/sim/debug/shell/shell.c
    ${CODE_DIR}/sim/debug/shell/shell_core.c
    ${CODE_DIR}/sim/debug/shell/shell_service.c
    ${CODE_DIR}/section/baremetal/section.c
    ${MATLAB_LIB_DIR}/mexFunction.def
)

foreach(node IN ITEMS node02 node03)
    set(target route_bridge_${node})
    set(BSP_NODE_DIR "${CMAKE_CURRENT_LIST_DIR}/bsp/${node}")
    set(INTERFACE_NODE_DIR "${CODE_DIR}/interface/frame_route_bridge/sim/${node}")
    add_library(${target} SHARED ${SOURCES}
        ${BSP_NODE_DIR}/bsp_tcp.c
        ${INTERFACE_NODE_DIR}/comm_link.c)
    target_compile_features(${target} PRIVATE c_std_11)
    target_compile_definitions(${target} PRIVATE
        MATLAB_MEX_FILE PLATFORM_MATLAB TOOLCHAIN_GCC
        S_FUNCTION_NAME=${target})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -Wpedantic
        -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wmissing-prototypes
        -Wstrict-prototypes -Wundef)
    target_include_directories(${target} SYSTEM PRIVATE
        "${MATLAB_ROOT}/extern/include" "${MATLAB_ROOT}/simulink/include")
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR} ${COMMON_DIR}
        ${BSP_NODE_DIR} ${INTERFACE_NODE_DIR}
        ${CODE_DIR}/sim/comm ${CODE_DIR}/sim/protocol
        ${CODE_DIR}/sim/debug/shell ${CODE_DIR}/sim/debug/perf
        ${CODE_DIR}/data_source/comm ${CODE_DIR}/data_source/demo ${CODE_DIR}/lib
        ${CODE_DIR}/section/baremetal ${CODE_DIR}/section)
    target_link_libraries(${target} PRIVATE
        "${MATLAB_LIB_DIR}/libmex.lib" "${MATLAB_LIB_DIR}/libmx.lib" ws2_32)
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".mexw64"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/build"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
