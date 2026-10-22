get_filename_component(BASE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/private")
set(BASE_SCOPE_QUEUE_BYTES 16384 CACHE STRING "Internal scope sample queue size in bytes")
set(BASE_SCOPE_BYTE_BUDGET 1700000 CACHE STRING "Scope upload budget in bytes per second")
set(BASE_WIFI_UPLOAD_BYTES 3072 CACHE STRING "Wi-Fi upload staging size in bytes")
option(BASE_SRTOS_PROBE "Run two calibrated 650 us tasks to verify real preemption" OFF)
set(BASE_SOURCES
    "${BASE_ROOT}/code/section/srtos_riscv/section.c"
    "${BASE_ROOT}/code/section/srtos_riscv/section_riscv_port.S"
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
set(APP_SOURCES
    "${BASE_ROOT}/code/business/demo/esp32c5_srtos/demo_comm.c"
    "${BASE_ROOT}/code/business/demo/esp32c5_srtos/wifi_demo.c"
    "${BASE_ROOT}/code/business/demo/esp32c5_srtos/demo_scope.c"
    "${BASE_ROOT}/code/business/demo/esp32c5_srtos/demo_scheduler.c")
set(BSP_SOURCES
    "../bsp/bsp_usart.c" "../bsp/bsp_wifi.c" "../bsp/bsp_wifi_socket.c"
    "../bsp/bsp_wifi_channels.c" "../bsp/bsp_comm.c" "../bsp/bsp_system.c" "../bsp/bsp_timer.c"
    "../bsp/bsp_srtos.c" "../bsp/bsp_srtos_hooks.S" "../bsp/comm_discovery.c")
set(INTERFACE_SOURCES
    "${BASE_ROOT}/code/interface/demo/esp32c5_srtos/comm_link.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5_srtos/wifi_interface.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5_srtos/platform_bindings.c"
    "${BASE_ROOT}/code/interface/demo/esp32c5_srtos/realtime_scope_bindings.c")
if(BASE_SRTOS_PROBE)
    list(APPEND BSP_SOURCES "../tests/srtos_probe.c")
endif()
idf_component_register(SRCS ${BASE_SOURCES} ${APP_SOURCES} ${BSP_SOURCES} ${INTERFACE_SOURCES} "../src/app_main.c"
    INCLUDE_DIRS "${CMAKE_BINARY_DIR}/private" "../inc" "../bsp"
        "${BASE_ROOT}/code/section/srtos_riscv" "${BASE_ROOT}/code/section"
        "${BASE_ROOT}/code/dbg" "${BASE_ROOT}/code/lib"
        "${BASE_ROOT}/code/data_source/comm" "${BASE_ROOT}/code/interface/demo/esp32c5_srtos"
        "${BASE_ROOT}/code/business/demo/esp32c5_srtos" "${BASE_ROOT}/code/business/demo" "${BASE_ROOT}/code/data_pool/demo"
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
target_compile_definitions(${COMPONENT_LIB} PUBLIC PLATFORM_ESP32C5_SRTOS=1 TOOLCHAIN_GCC=1 REALTIME_SCOPE_ENABLE=1 DEMO_REALTIME_SCOPE_ENABLE=1)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    DEMO_TASK_DEAD_LOOP_ENABLE=0
    BASE_SCOPE_QUEUE_BYTES=${BASE_SCOPE_QUEUE_BYTES}
    BASE_SCOPE_BYTE_BUDGET=${BASE_SCOPE_BYTE_BUDGET}
    BSP_WIFI_UPLOAD_BUFFER_SIZE=${BASE_WIFI_UPLOAD_BYTES})
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=rtos_int_enter" "-Wl,--wrap=rtos_int_exit")
# IDF headers use GNU asm; keep SDK-facing adapters on GNU C11 and portable sources on C11.
set_source_files_properties(${BASE_SOURCES} ${APP_SOURCES} ${INTERFACE_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=c11")
target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion
    -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef)
