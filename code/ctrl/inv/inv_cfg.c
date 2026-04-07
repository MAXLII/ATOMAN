#include "inv_cfg.h"
#include <stddef.h>
#include "section.h"

static inv_ctrl_setpoint_t setpoint_active = {0};
static inv_ctrl_setpoint_t setpoint_building = {
    .run_allowed = INV_CFG_DEFAULT_RUN_ALLOWED,
    .freq_hz = INV_CFG_DEFAULT_FREQ_HZ,
    .freq_slew_hzps = INV_CFG_DEFAULT_FREQ_SLEW_HZPS,
    .rms_ref_v = INV_CFG_DEFAULT_RMS_REF_V,
    .rms_slew_vps = INV_CFG_DEFAULT_RMS_SLEW_VPS,
};

static inv_ctrl_setpoint_mgr_t setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0,
    },
};

void inv_cfg_set_p_building(inv_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        setpoint_mgr.building.p_data = p_data;
        PLECS_LOG("inv_cfg set building buffer: %p\n", p_data);
    }
}

inv_ctrl_setpoint_t *inv_cfg_get_p_active(void)
{
    return setpoint_mgr.active.p_data;
}

inv_ctrl_setpoint_t *inv_cfg_get_p_building(void)
{
    return setpoint_mgr.building.p_data;
}

void inv_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->run_allowed != run_allowed)
    {
        setpoint_mgr.building.p_data->run_allowed = run_allowed;
        PLECS_LOG("inv_cfg set run_allowed: %u\n", run_allowed);
    }
}

void inv_cfg_set_freq_hz(float freq_hz)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->freq_hz != freq_hz)
    {
        setpoint_mgr.building.p_data->freq_hz = freq_hz;
        PLECS_LOG("inv_cfg set freq_hz: %.3f\n", freq_hz);
    }
}

void inv_cfg_set_freq_slew_hzps(float freq_slew_hzps)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->freq_slew_hzps != freq_slew_hzps)
    {
        setpoint_mgr.building.p_data->freq_slew_hzps = freq_slew_hzps;
        PLECS_LOG("inv_cfg set freq_slew_hzps: %.3f\n", freq_slew_hzps);
    }
}

void inv_cfg_set_rms_ref_v(float rms_ref_v)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->rms_ref_v != rms_ref_v)
    {
        setpoint_mgr.building.p_data->rms_ref_v = rms_ref_v;
        PLECS_LOG("inv_cfg set rms_ref_v: %.3f\n", rms_ref_v);
    }
}

void inv_cfg_set_rms_slew_vps(float rms_slew_vps)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->rms_slew_vps != rms_slew_vps)
    {
        setpoint_mgr.building.p_data->rms_slew_vps = rms_slew_vps;
        PLECS_LOG("inv_cfg set rms_slew_vps: %.3f\n", rms_slew_vps);
    }
}

void inv_cfg_publish_building(void)
{
    if ((setpoint_mgr.building.p_data == NULL) ||
        (setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    setpoint_mgr.building.version++;
    *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
    setpoint_mgr.active.version = setpoint_mgr.building.version;
    PLECS_LOG("inv_cfg publish version:%u run:%u freq:%.3f freq_slew:%.3f rms:%.3f rms_slew:%.3f\n",
              setpoint_mgr.active.version,
              setpoint_mgr.active.p_data->run_allowed,
              setpoint_mgr.active.p_data->freq_hz,
              setpoint_mgr.active.p_data->freq_slew_hzps,
              setpoint_mgr.active.p_data->rms_ref_v,
              setpoint_mgr.active.p_data->rms_slew_vps);
}

void inv_cfg_building_version_inc(void)
{
    setpoint_mgr.building.version++;
    PLECS_LOG("inv_cfg building version inc: %u\n", setpoint_mgr.building.version);
}

uint8_t inv_cfg_is_ready(void)
{
    return (setpoint_mgr.active.p_data != NULL) &&
           (setpoint_mgr.building.p_data != NULL);
}

void inv_cfg_sync_building_to_active(void)
{
    if ((setpoint_mgr.building.p_data == NULL) ||
        (setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    if (setpoint_mgr.active.version != setpoint_mgr.building.version)
    {
        *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
        setpoint_mgr.active.version = setpoint_mgr.building.version;
    }
}

const inv_ctrl_setpoint_mgr_t *inv_cfg_get_mgr(void)
{
    PLECS_LOG("inv_cfg get manager\n");
    return &setpoint_mgr;
}
