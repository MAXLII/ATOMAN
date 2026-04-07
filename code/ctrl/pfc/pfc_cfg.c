#include "pfc_cfg.h"
#include <stddef.h>
#include "section.h"

static pfc_ctrl_setpoint_t setpoint_active = {0};
static pfc_ctrl_setpoint_t setpoint_building = {
    .run_allowed = 0,
    .vbus_ref_v = PFC_CFG_DEFAULT_VBUS_REF_V,
    .vbus_slew_vps = PFC_CFG_DEFAULT_VBUS_SLEW_VPS,
};

static pfc_ctrl_setpoint_mgr_t setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0,
    },
};

void pfc_cfg_set_p_building(pfc_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        setpoint_mgr.building.p_data = p_data;
        PLECS_LOG("pfc_cfg set building buffer: %p\n", p_data);
    }
}

pfc_ctrl_setpoint_t *pfc_cfg_get_p_active(void)
{
    return setpoint_mgr.active.p_data;
}

pfc_ctrl_setpoint_t *pfc_cfg_get_p_building(void)
{
    return setpoint_mgr.building.p_data;
}

void pfc_cfg_set_vbus_ref_v(float vbus_ref_v)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->vbus_ref_v != vbus_ref_v)
    {
        setpoint_mgr.building.p_data->vbus_ref_v = vbus_ref_v;
        PLECS_LOG("pfc_cfg set vbus_ref_v: %.3f\n", vbus_ref_v);
    }
}

void pfc_cfg_set_vbus_slew_vps(float vbus_slew_vps)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->vbus_slew_vps != vbus_slew_vps)
    {
        setpoint_mgr.building.p_data->vbus_slew_vps = vbus_slew_vps;
        PLECS_LOG("pfc_cfg set vbus_slew_vps: %.3f\n", vbus_slew_vps);
    }
}

void pfc_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    if (setpoint_mgr.building.p_data->run_allowed != run_allowed)
    {
        setpoint_mgr.building.p_data->run_allowed = run_allowed;
        PLECS_LOG("pfc_cfg set run_allowed: %u\n", run_allowed);
    }
}

void pfc_cfg_publish_building(void)
{
    if ((setpoint_mgr.building.p_data == NULL) ||
        (setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    setpoint_mgr.building.version++;
    *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
    setpoint_mgr.active.version = setpoint_mgr.building.version;
    PLECS_LOG("pfc_cfg publish version:%u run:%u vbus_ref:%.3f slew:%.3f\n",
              setpoint_mgr.active.version,
              setpoint_mgr.active.p_data->run_allowed,
              setpoint_mgr.active.p_data->vbus_ref_v,
              setpoint_mgr.active.p_data->vbus_slew_vps);
}

void pfc_cfg_building_version_inc(void)
{
    setpoint_mgr.building.version++;
    PLECS_LOG("pfc_cfg building version inc: %u\n", setpoint_mgr.building.version);
}

uint8_t pfc_cfg_is_ready(void)
{
    return (setpoint_mgr.active.p_data != NULL) &&
           (setpoint_mgr.building.p_data != NULL);
}

void pfc_cfg_sync_building_to_active(void)
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

const pfc_ctrl_setpoint_mgr_t *pfc_cfg_get_mgr(void)
{
    PLECS_LOG("pfc_cfg get manager\n");
    return &setpoint_mgr;
}
