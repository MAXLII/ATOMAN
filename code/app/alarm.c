#include "alarm.h"

#include "shell.h"

typedef struct
{
    uint32_t active;
    uint32_t record;
} alarm_group_t;

static alarm_group_t s_alarm_fault = {0};
static alarm_group_t s_alarm_warn = {0};

REG_SHELL_VAR(ALARM_FAULT_STA, s_alarm_fault.active, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ALARM_FAULT_STA_RECORD, s_alarm_fault.record, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ALARM_WARN_STA, s_alarm_warn.active, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ALARM_WARN_STA_RECORD, s_alarm_warn.record, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)

static uint32_t alarm_group_mask(uint32_t alarm, uint32_t alarm_max)
{
    assert(alarm < alarm_max);
    assert(alarm < 32u);

    return UINT32_C(1) << alarm;
}

static void alarm_group_set(alarm_group_t *group, uint32_t alarm, uint32_t alarm_max)
{
    const uint32_t mask = alarm_group_mask(alarm, alarm_max);

    group->active |= mask;
    group->record |= mask;
}

static void alarm_group_clr(alarm_group_t *group, uint32_t alarm, uint32_t alarm_max)
{
    const uint32_t mask = alarm_group_mask(alarm, alarm_max);

    group->active &= ~mask;
}

static void alarm_group_clr_all(alarm_group_t *group)
{
    group->active = 0u;
}

static uint8_t alarm_group_get(const alarm_group_t *group, uint32_t alarm, uint32_t alarm_max)
{
    const uint32_t mask = alarm_group_mask(alarm, alarm_max);

    return (group->active & mask) ? 1u : 0u;
}

static uint32_t alarm_group_get_all(const alarm_group_t *group)
{
    return group->active;
}

static uint32_t alarm_group_get_record_all(const alarm_group_t *group)
{
    return group->record;
}

static void alarm_group_record_clr_all(alarm_group_t *group)
{
    group->record = 0u;
}

static uint8_t alarm_group_check(const alarm_group_t *group)
{
    return group->active ? 1u : 0u;
}

void alarm_fault_set(alarm_fault_e alarm)
{
    alarm_group_set(&s_alarm_fault, (uint32_t)alarm, (uint32_t)ALARM_FAULT_MAX);
}

void alarm_fault_clr(alarm_fault_e alarm)
{
    alarm_group_clr(&s_alarm_fault, (uint32_t)alarm, (uint32_t)ALARM_FAULT_MAX);
}

void alarm_fault_clr_all(void)
{
    alarm_group_clr_all(&s_alarm_fault);
}

uint8_t alarm_fault_get(alarm_fault_e alarm)
{
    return alarm_group_get(&s_alarm_fault, (uint32_t)alarm, (uint32_t)ALARM_FAULT_MAX);
}

uint32_t alarm_fault_get_all(void)
{
    return alarm_group_get_all(&s_alarm_fault);
}

uint32_t alarm_fault_get_record_all(void)
{
    return alarm_group_get_record_all(&s_alarm_fault);
}

void alarm_fault_record_clr_all(void)
{
    alarm_group_record_clr_all(&s_alarm_fault);
}

uint8_t alarm_fault_check(void)
{
    return alarm_group_check(&s_alarm_fault);
}

void alarm_warn_set(alarm_warn_e alarm)
{
    alarm_group_set(&s_alarm_warn, (uint32_t)alarm, (uint32_t)ALARM_WARN_MAX);
}

void alarm_warn_clr(alarm_warn_e alarm)
{
    alarm_group_clr(&s_alarm_warn, (uint32_t)alarm, (uint32_t)ALARM_WARN_MAX);
}

void alarm_warn_clr_all(void)
{
    alarm_group_clr_all(&s_alarm_warn);
}

uint8_t alarm_warn_get(alarm_warn_e alarm)
{
    return alarm_group_get(&s_alarm_warn, (uint32_t)alarm, (uint32_t)ALARM_WARN_MAX);
}

uint32_t alarm_warn_get_all(void)
{
    return alarm_group_get_all(&s_alarm_warn);
}

uint32_t alarm_warn_get_record_all(void)
{
    return alarm_group_get_record_all(&s_alarm_warn);
}

void alarm_warn_record_clr_all(void)
{
    alarm_group_record_clr_all(&s_alarm_warn);
}

uint8_t alarm_warn_check(void)
{
    return alarm_group_check(&s_alarm_warn);
}
