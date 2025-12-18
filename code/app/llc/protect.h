#ifndef __PROTECT_H
#define __PROTECT_H

#include "stdint.h"
#include "fault.h"
#include "my_math.h"

#define MPPT_IN_OVP_THR 65.0f
#define MPPT_IN_OVP_THR_HYS 59.0f
#define MPPT_IN_OVP_TIME TIME_CNT_100MS_IN_1MS
#define MPPT_IN_OVP_TIME_HYS TIME_CNT_100MS_IN_1MS

#define MPPT_L_OCP_THR 168.0f
#define MPPT_L_OCP_THR_HYS 150.0f
#define MPPT_L_OCP_TIME TIME_CNT_1MS_IN_1MS
#define MPPT_L_OCP_TIME_HYS TIME_CNT_100MS_IN_1MS

#endif
