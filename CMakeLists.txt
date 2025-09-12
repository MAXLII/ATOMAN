cmake_minimum_required(VERSION 3.10)
project(plecs LANGUAGES C)

# 强制使用 64 位 GCC 工具链
set(CMAKE_C_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe")
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/x86_64-w64-mingw32-g++.exe")

# 包含路径
include_directories(${CMAKE_SOURCE_DIR}/code/app)
include_directories(${CMAKE_SOURCE_DIR}/code/interface)
include_directories(${CMAKE_SOURCE_DIR}/plecs)
include_directories(${CMAKE_SOURCE_DIR}/plecs/bsp)
include_directories(${CMAKE_SOURCE_DIR}/section)
include_directories("D:/Documents/Plexim/PLECS 4.7 (64 bit)/include/plecs")

# 源文件
set(SOURCES
code/app/led.c
code/interface/gpio.c
plecs/bsp/bsp_gpio.c
plecs/plecs.c
section/section.c
)

# 添加全局宏定义
add_definitions(-DIS_PLECS)

# 生成共享库（DLL）
add_library(plecs SHARED ${SOURCES})

# 确保输出目录存在
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置输出路径
set_target_properties(plecs PROPERTIES
OUTPUT_NAME "plecs"
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 生成 MAP 文件（可选）
if(WIN32)
set_target_properties(plecs PROPERTIES
LINK_FLAGS "-Wl,-Map,${CMAKE_BINARY_DIR}/bin/mydll.map"
)
endif()