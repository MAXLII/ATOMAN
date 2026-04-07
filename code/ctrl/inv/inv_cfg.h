#ifndef __INV_CFG_H
#define __INV_CFG_H

#include <stdint.h>

#define INV_CFG_DEFAULT_RUN_ALLOWED (0U)
#define INV_CFG_DEFAULT_FREQ_HZ (50.0f)
#define INV_CFG_DEFAULT_FREQ_SLEW_HZPS (10.0f)
#define INV_CFG_DEFAULT_RMS_REF_V (230.0f)
#define INV_CFG_DEFAULT_RMS_SLEW_VPS (212.0f)

typedef struct
{
    uint8_t run_allowed;
    float freq_hz;
    float freq_slew_hzps;
    float rms_ref_v;
    float rms_slew_vps;
} inv_ctrl_setpoint_t;

typedef struct
{
    inv_ctrl_setpoint_t *p_data;
    unsigned int version;
} inv_ctrl_setpoint_buf_t;

typedef struct
{
    inv_ctrl_setpoint_buf_t active;
    inv_ctrl_setpoint_buf_t building;
} inv_ctrl_setpoint_mgr_t;

void inv_cfg_set_p_building(inv_ctrl_setpoint_t *p_data);

inv_ctrl_setpoint_t *inv_cfg_get_p_active(void);
inv_ctrl_setpoint_t *inv_cfg_get_p_building(void);

void inv_cfg_set_run_allowed(uint8_t run_allowed);
void inv_cfg_set_freq_hz(float freq_hz);
void inv_cfg_set_freq_slew_hzps(float freq_slew_hzps);
void inv_cfg_set_rms_ref_v(float rms_ref_v);
void inv_cfg_set_rms_slew_vps(float rms_slew_vps);

void inv_cfg_publish_building(void);
void inv_cfg_building_version_inc(void);

uint8_t inv_cfg_is_ready(void);
void inv_cfg_sync_building_to_active(void);

const inv_ctrl_setpoint_mgr_t *inv_cfg_get_mgr(void);

#endif
