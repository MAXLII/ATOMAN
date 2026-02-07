#pragma once

#include <stdint.h>
#include <stddef.h>

// 平台相关配置
#ifdef IS_PLECS
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us ///< 系统时钟（100us单位）
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section ///< 段起始地址
#define SECTION_STOP __stop_section   ///< 段结束地址
#define SYSTEM_RESET ;                ///< 系统复位（PLECS中为空）
#else
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us() ///< 系统时钟（100us单位）
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start            ///< 段起始地址
#define SECTION_STOP __section_end               ///< 段结束地址
#define SYSTEM_RESET NVIC_SystemReset()          ///< 系统复位
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#endif
