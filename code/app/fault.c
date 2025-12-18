#include "fault.h"
#include "stdint.h"
#include "stdbool.h"
#include "section.h"

static uint32_t fault_sta = 0;
static uint32_t fault_sta_record = 0;

REG_SHELL_VAR(FAULT_STA, fault_sta, SHELL_UINT32, 0xFFFFFFFF, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FAULT_STA_RECORD, fault_sta_record, SHELL_UINT32, 0xFFFFFFFF, 0, NULL, SHELL_STA_NULL)

void fault_set_bit(FAULT_STA_E sta)
{
    fault_sta |= 1 << sta;
    fault_sta_record |= 1 << sta;
}

void fault_clr_bit(FAULT_STA_E sta)
{
    fault_sta &= ~(1 << sta);
}

uint8_t fault_get_bit(FAULT_STA_E sta)
{
    if (fault_sta & (1 << sta))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t fault_get_all(void)
{
    return fault_sta;
}

uint8_t fault_check(void)
{
    return fault_sta ? 1 : 0;
}
