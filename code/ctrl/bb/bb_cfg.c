/* bb_cfg.c
 * Buck-boost controller setpoint buffering and publish/sync utilities.
 */

#include "bb_cfg.h"
#include <stddef.h>

static bb_ctrl_setpoint_t setpoint_active = {0};   /* setpoint_active: snapshot consumed by control logic */
static bb_ctrl_setpoint_t setpoint_building = {0}; /* setpoint_building: scratch image updated by upper layers */

static bb_ctrl_setpoint_mgr_t setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0U,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0U,
    },
};

/**
 * @brief Replace the staged building buffer pointer.
 * @param p_data Pointer to the replacement building buffer.
 * @return None.
 */
void bb_cfg_set_p_building(bb_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        setpoint_mgr.building.p_data = p_data;
    }
}

/**
 * @brief Return the active setpoint pointer.
 * @param None.
 * @return Pointer to the active setpoint payload.
 */
bb_ctrl_setpoint_t *bb_cfg_get_p_active(void)
{
    return setpoint_mgr.active.p_data;
}

/**
 * @brief Return the building setpoint pointer.
 * @param None.
 * @return Pointer to the building setpoint payload.
 */
bb_ctrl_setpoint_t *bb_cfg_get_p_building(void)
{
    return setpoint_mgr.building.p_data;
}

/**
 * @brief Write the staged run-allow flag.
 * @param run_allowed 1 enables run, 0 disables run.
 * @return None.
 */
void bb_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->run_allowed = run_allowed;
}

/**
 * @brief Write the staged power limit.
 * @param pwr_lmt Input power limit in watts.
 * @return None.
 */
void bb_cfg_set_pwr_lmt(float pwr_lmt)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->pwr_lmt = pwr_lmt;
}

/**
 * @brief Write the staged output-voltage reference.
 * @param out_volt_ref Output-voltage reference.
 * @return None.
 */
void bb_cfg_set_out_volt_ref(float out_volt_ref)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->out_volt_ref = out_volt_ref;
}

/**
 * @brief Write the staged input-voltage limit.
 * @param in_volt_lmt Input-voltage limit.
 * @return None.
 */
void bb_cfg_set_in_volt_lmt(float in_volt_lmt)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->in_volt_lmt = in_volt_lmt;
}

/**
 * @brief Write the staged input-current limit.
 * @param in_curr_lmt Input-current limit.
 * @return None.
 */
void bb_cfg_set_in_curr_lmt(float in_curr_lmt)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->in_curr_lmt = in_curr_lmt;
}

/**
 * @brief Write the staged output-current limit.
 * @param out_curr_lmt Output-current limit.
 * @return None.
 */
void bb_cfg_set_out_curr_lmt(float out_curr_lmt)
{
    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    setpoint_mgr.building.p_data->out_curr_lmt = out_curr_lmt;
}

/**
 * @brief Copy the building buffer into the active buffer and bump version.
 * @param None.
 * @return None.
 */
void bb_cfg_publish_building(void)
{
    if ((setpoint_mgr.building.p_data == NULL) ||
        (setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    setpoint_mgr.building.version++;
    *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
    setpoint_mgr.active.version = setpoint_mgr.building.version;
}

/**
 * @brief Increment the building-buffer version counter.
 * @param None.
 * @return None.
 */
void bb_cfg_building_version_inc(void)
{
    setpoint_mgr.building.version++;
}

/**
 * @brief Check whether both active and building buffers are available.
 * @param None.
 * @return 1 when the manager is ready, otherwise 0.
 */
uint8_t bb_cfg_is_ready(void)
{
    return (setpoint_mgr.active.p_data != NULL) &&
           (setpoint_mgr.building.p_data != NULL);
}

/**
 * @brief Synchronize active data to the latest building version when needed.
 * @param None.
 * @return None.
 */
void bb_cfg_sync_building_to_active(void)
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

/**
 * @brief Return the address of the setpoint manager.
 * @param None.
 * @return Pointer to the manager object.
 */
const bb_ctrl_setpoint_mgr_t *bb_cfg_get_mgr(void)
{
    return &setpoint_mgr;
}
