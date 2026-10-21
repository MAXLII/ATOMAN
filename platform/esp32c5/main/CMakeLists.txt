get_filename_component(BASE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/private")
set(BASE_SOURCES
    "${BASE_ROOT}/code/section/baremetal/section.c"
    "${BASE_ROOT}/code/data_source/comm/comm.c"
    "${BASE_ROOT}/code/data_source/comm/comm_v1.c"
    "${BASE_ROOT}/code/dbg/shell.c"
    "${BASE_ROOT}/code/dbg/realtime_scope.c"
    "${BASE_ROOT}/code/dbg/realtime_scope_service.c"
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
    "${BASE_ROOT}/code/business/demo/demo_realtime_scope.c"
    "${BASE_ROOT}/code/data_pool/demo/demo_data_pool.c")
set(BSP_SOURCES
    "../bsp/bsp_usart.c" "../bsp/bsp_wifi.c" "../bsp/bsp_wifi_socket.c"
    "../bsp/bsp_wifi_channels.c" "../bsp/bsp_comm.c" "../bsp/bsp_system.c" "../bsp/bsp_timer.c")
set(INTERFACE_SOURCES
    "${BASE_ROOT}/code/interface/demo/esp32c5/comm_link.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5/comm_discovery.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5/wifi_demo.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5/platform_bindings.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5/realtime_scope_bindings.c")
idf_component_register(SRCS ${BASE_SOURCES} ${BSP_SOURCES} ${INTERFACE_SOURCES} "../src/app_main.c"
    INCLUDE_DIRS "${CMAKE_BINARY_DIR}/private" "../inc" "../bsp"
        "${BASE_ROOT}/code/section/baremetal" "${BASE_ROOT}/code/section"
        "${BASE_ROOT}/code/dbg" "${BASE_ROOT}/code/lib"
        "${BASE_ROOT}/code/data_source/comm" "${BASE_ROOT}/code/interface/demo/esp32c5"
        "${BASE_ROOT}/code/business/demo" "${BASE_ROOT}/code/data_pool/demo"
    REQUIRES esp_driver_usb_serial_jtag esp_driver_gptimer esp_app_format esp_wifi esp_netif esp_timer esp_system lwip vfs
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
target_compile_definitions(${COMPONENT_LIB} PUBLIC PLATFORM_ESP32C5=1 TOOLCHAIN_GCC=1 REALTIME_SCOPE_ENABLE=1 DEMO_REALTIME_SCOPE_ENABLE=1)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef)
