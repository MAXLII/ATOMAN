get_filename_component(BASE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/private")
set(DEMO_SOURCES
    "${BASE_ROOT}/code/section/baremetal/section.c"
    "${BASE_ROOT}/code/data_source/comm/comm.c"
    "${BASE_ROOT}/code/data_source/comm/comm_v1.c"
    "${BASE_ROOT}/code/dbg/shell.c"
    "${BASE_ROOT}/code/dbg/shell_core.c"
    "${BASE_ROOT}/code/dbg/shell_service.c"
    "${BASE_ROOT}/code/dbg/perf.c"
    "${BASE_ROOT}/code/dbg/perf_core.c"
    "${BASE_ROOT}/code/dbg/perf_service.c"
    "${BASE_ROOT}/code/dbg/section_list_service.c"
    "${BASE_ROOT}/code/lib/record_dict.c"
    "${BASE_ROOT}/code/lib/codec_dict.c"
    "${BASE_ROOT}/code/lib/codec_rle.c"
    "${BASE_ROOT}/code/lib/codec_lzss.c"
    "${BASE_ROOT}/code/lib/codec_zero.c"
    "${BASE_ROOT}/code/business/demo/demo_shell.c"
    "${BASE_ROOT}/code/business/demo/demo_task.c"
    "${BASE_ROOT}/code/business/demo/demo_perf.c"
    "${BASE_ROOT}/code/data_pool/demo/demo_data_pool.c"
    "../bsp/bsp_usart.c" "../bsp/bsp_wifi.c" "../bsp/bsp_wifi_socket.c"
    "../src/app_main.c" "../src/platform_port.c" "../src/comm_link.c" "../src/platform_demo.c")
idf_component_register(SRCS ${DEMO_SOURCES}
    INCLUDE_DIRS "${CMAKE_BINARY_DIR}/private" "../inc" "../bsp"
        "${BASE_ROOT}/code/section/baremetal" "${BASE_ROOT}/code/section"
        "${BASE_ROOT}/code/dbg" "${BASE_ROOT}/code/lib"
        "${BASE_ROOT}/code/data_source/comm" "${BASE_ROOT}/code/interface/demo/common"
        "${BASE_ROOT}/code/business/demo" "${BASE_ROOT}/code/data_pool/demo"
    REQUIRES esp_driver_usb_serial_jtag esp_wifi esp_netif esp_timer esp_system lwip freertos vfs
    LDFRAGMENTS "section.lf" WHOLE_ARCHIVE)
target_compile_features(${COMPONENT_LIB} PUBLIC c_std_11)
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
idf_build_get_property(sdk_components BUILD_COMPONENTS)
foreach(sdk_component IN LISTS sdk_components)
    if(NOT sdk_component STREQUAL "main")
        idf_component_get_property(sdk_library ${sdk_component} COMPONENT_LIB)
        get_target_property(sdk_includes ${sdk_library} INTERFACE_INCLUDE_DIRECTORIES)
        if(sdk_includes)
            target_include_directories(${COMPONENT_LIB} SYSTEM PRIVATE ${sdk_includes})
        endif()
    endif()
endforeach()
target_compile_definitions(${COMPONENT_LIB} PUBLIC PLATFORM_ESP32C5=1 TOOLCHAIN_GCC=1)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef)
