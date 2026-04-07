#include "adc_proc.h"

/*
 * ADC check module responsibilities:
 * 1) Acquire raw ADC channels in ISR context.
 * 2) Run RMS calculators for key voltage/current signals.
 * 3) Drive main relay timing logic through rly_on.
 * 4) Classify bus voltage stage (precharge / nominal regulation) using
 *    hysteresis comparators and publish vbus_sta for FSM consumption.
 * 5) Evaluate grid validity by combining chk_grid window check and
 *    ac_loss_det waveform-loss detector.
 */

/* Instantaneous sampled signals (updated by adc_proc_isr). */
float v_g = 0.0f;   /* Grid/input AC voltage sample */
float v_cap = 0.0f; /* PFC capacitor voltage sample */
float v_bus = 0.0f; /* DC bus voltage sample */
float i_l = 0.0f;   /* PFC inductor current sample */
float v_out = 0.0f; /* Output AC voltage sample */
float i_out = 0.0f; /* Output AC current sample */

/* RMS results (kept for cross-module visibility). */
float v_g_rms = 0.0f;
float v_cap_rms = 0.0f;
float i_l_rms = 0.0f;
float i_out_rms = 0.0f;

/* Frequency observers (fed by RMS blocks where available). */
float v_g_freq = 0.0f;
float v_cap_freq = 0.0f;

/* RMS calculator instances. */
static cal_rms_t cal_rms_v_cap = {0};
static cal_rms_t cal_rms_i_l = {0};
static cal_rms_t cal_rms_v_g = {0};
static cal_rms_t cal_rms_i_out = {0};
static cal_rms_t cal_rms_v_out = {0};

/* Main relay helper state machine. */
static rly_on_t main_rly_on = {0};

/* Relay trigger flags consumed by rly_on_func(). */
static uint8_t main_rly_on_trig = 0;
static uint8_t main_rly_off_trig = 0;

/* Phase/voltage-equality indicator used by rly_on algorithm. */
static uint8_t cap_g_is_requal = 0;

/* Public bus voltage stage for PFC FSM decisions. */
pfc_vbus_sta_e vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
uint8_t main_rly_is_closed = 0;

/*
 * AC quality detectors:
 * - ac_loss_det: waveform-loss detector (fast disturbance/loss detection)
 * - chk_grid:    RMS/frequency window validator with debounce counters
 * - ac_is_ok:    latched final AC availability state used in this module
 */
static ac_loss_det_t ac_loss_det;
static uint8_t ac_is_ok = 0;
static chk_grid_t chk_grid;

uint8_t adc_proc_get_ac_is_ok(void)
{
    return ac_is_ok;
}

/*
 * Hysteresis comparators:
 * - pfc_cmp_vbus_precharge: detects "bus reached input peak".
 * - pfc_cmp_vbus_nom:       detects "bus entered regulation window".
 */
static hys_cmp_t pfc_cmp_vbus_precharge = {0};
static hys_cmp_t pfc_cmp_vbus_nom = {0};

/*
 * Main relay requests are edge-triggered by setting flags.
 * Physical relay switching is executed inside rly_on_func() timing logic.
 */
void adc_proc_main_rly_on(void)
{
    main_rly_on_trig = 1;
}

void adc_proc_main_rly_off(void)
{
    main_rly_off_trig = 1;
}

/* Low-level callbacks provided to rly_on_init(). */
static void main_rly_on_func(void)
{
    gpio_set_main_rly_sta(1);
}

static void main_rly_off_func(void)
{
    gpio_set_main_rly_sta(0);
}

/*
 * Module initialization:
 * - Configure all RMS calculators.
 * - Configure relay on/off arbitration and delay behavior.
 * - Configure hysteresis comparators for bus stage classification.
 * - Initialize ac_loss_det and chk_grid with dedicated thresholds/timing.
 */
static void adc_proc_init(void)
{
    /* v_cap RMS is master; it drives timing for i_l RMS slave. */
    cal_rms_init(&cal_rms_v_cap,
                 CAL_RMS_MASTER,
                 CTRL_TS,
                 10.0f,
                 &v_cap,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_i_l,
                 CAL_RMS_SLAVE,
                 CTRL_TS,
                 1.0f,
                 &i_l,
                 &cal_rms_v_cap.output.is_cal,
                 &cal_rms_v_cap.output.is_run);

    /* Slow RMS channels for grid/output monitoring (100 us loop). */
    cal_rms_init(&cal_rms_v_g,
                 CAL_RMS_MASTER,
                 0.0001f,
                 5.0f,
                 &v_g,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_v_out,
                 CAL_RMS_MASTER,
                 0.0001f,
                 5.0f,
                 &v_out,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_i_out,
                 CAL_RMS_SLAVE,
                 0.0001f,
                 1.0f,
                 &i_out,
                 &cal_rms_v_out.output.is_cal,
                 &cal_rms_v_out.output.is_run);

    /*
     * Main relay control block:
     * - Triggers: main_rly_on_trig / main_rly_off_trig
     * - Synchronous condition source: cap_g_is_requal + grid frequency
     * - Delay: MAIN_RLY_ON_DLY
     */
    rly_on_init(&main_rly_on,
                &main_rly_on_trig,
                &main_rly_off_trig,
                &cap_g_is_requal,
                &cal_rms_v_g.output.freq,
                CTRL_FREQ,
                MAIN_RLY_ON_DLY,
                main_rly_on_func,
                main_rly_off_func);

    /* Comparator #1: detect bus reaching rectified-input peak region. */
    hys_cmp_cfg_t hys_cmp_cfg = {
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
        .thr = PFC_VBUS_PRECHARGE_ENTER,
        .thr_hys = PFC_VBUS_PRECHARGE_EXIT,
        .time = PFC_VBUS_PRECHARGE_ENTER_TIME,
        .time_hys = PFC_VBUS_PRECHARGE_EXIT_TIME,
    };
    hys_cmp_init(&pfc_cmp_vbus_precharge,
                 &v_bus,
                 hys_cmp_cfg.thr,
                 hys_cmp_cfg.thr_hys,
                 hys_cmp_cfg.time,
                 hys_cmp_cfg.time_hys,
                 hys_cmp_cfg.p_cmp_func,
                 hys_cmp_cfg.p_cmp_hys_func);

    /* Comparator #2: detect bus entering nominal regulation window. */
    hys_cmp_cfg.thr = PFC_VBUS_NOM_ENTER;
    hys_cmp_cfg.thr_hys = PFC_VBUS_NOM_EXIT;
    hys_cmp_cfg.time = PFC_VBUS_NOM_ENTER_TIME;
    hys_cmp_cfg.time_hys = PFC_VBUS_NOM_EXIT_TIME;
    hys_cmp_init(&pfc_cmp_vbus_nom,
                 &v_bus,
                 hys_cmp_cfg.thr,
                 hys_cmp_cfg.thr_hys,
                 hys_cmp_cfg.time,
                 hys_cmp_cfg.time_hys,
                 hys_cmp_cfg.p_cmp_func,
                 hys_cmp_cfg.p_cmp_hys_func);

    /* AC-loss detector uses instantaneous grid voltage and grid-valid bit. */
    ac_loss_det_init(&ac_loss_det,
                     &v_g,
                     &chk_grid.output.is_ok);

    /* Grid validator uses RMS + frequency with debounced threshold windows. */
    chk_grid_init(&chk_grid,
                  &v_g_rms,
                  &v_g_freq,
                  PFC_GRID_JUDGE_TIME,
                  PFC_GRID_ABNORMAL_TIME,
                  PFC_GRID_RMS_NORMAL_MAX_V,
                  PFC_GRID_RMS_NORMAL_MIN_V,
                  PFC_GRID_RMS_ABNORMAL_MAX_V,
                  PFC_GRID_RMS_ABNORMAL_MIN_V,
                  PFC_GRID_FREQ_NORMAL_MAX_HZ,
                  PFC_GRID_FREQ_NORMAL_MIN_HZ,
                  PFC_GRID_FREQ_ABNORMAL_MAX_HZ,
                  PFC_GRID_FREQ_ABNORMAL_MIN_HZ);
}

REG_INIT(1, adc_proc_init)

/*
 * Highest-priority acquisition ISR:
 * waits for ADC conversion-ready flag, clears it, then latches all channels.
 */
static void adc_proc_isr(void)
{
    while (adc_get_sample_is_ok() == 0)
        ;
    adc_clr_sample_is_ok();

    v_g = adc_get_v_g();
    v_cap = adc_get_v_cap();
    v_bus = adc_get_v_bus();
    i_l = adc_get_i_l();
    i_out = adc_get_i_out();
    v_out = adc_get_v_out();
}

REG_INTERRUPT(0, adc_proc_isr)

/*
 * Fast RMS/relay service ISR:
 * - Update fast RMS channels.
 * - Execute relay state machine once per control tick.
 */
static void cal_rms_isr(void)
{
    cal_rms_master_run(&cal_rms_v_cap);
    cal_rms_slave_run(&cal_rms_i_l);
    v_cap_rms = cal_rms_v_cap.output.rms;
    v_cap_freq = cal_rms_v_cap.output.freq;
    i_l_rms = cal_rms_i_l.output.rms;
    rly_on_func(&main_rly_on);
    main_rly_is_closed = main_rly_on.output.is_closed;
}

REG_INTERRUPT(1, cal_rms_isr)

/*
 * Background RMS service task for slower channels.
 * Registered with REG_TASK(1): period is in 100 us units.
 */
static void cal_rms_task(void)
{
    cal_rms_master_run(&cal_rms_v_out);
    cal_rms_master_run(&cal_rms_v_g);
    cal_rms_slave_run(&cal_rms_i_out);

    v_g_rms = cal_rms_v_g.output.rms;
    v_g_freq = cal_rms_v_g.output.freq;
}

REG_TASK(1, cal_rms_task);

/*
 * Bus stage classifier task (1 ms):
 * Priority of states is:
 *   IN_REGULATION > AT_INPUT_PEAK > BELOW_INPUT_PEAK
 * This state is consumed by pfc_fsm to decide transitions.
 */
static void adc_proc_hys_cmp_task(void)
{
    hys_cmp_func(&pfc_cmp_vbus_precharge);
    hys_cmp_func(&pfc_cmp_vbus_nom);

    if (pfc_cmp_vbus_nom.output.is_asserted == 1)
    {
        vbus_sta = PFC_VBUS_STA_IN_REGULATION;
    }
    else if (pfc_cmp_vbus_precharge.output.is_asserted == 1)
    {
        vbus_sta = PFC_VBUS_STA_AT_INPUT_PEAK;
    }
    else
    {
        vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
    }
}

REG_TASK_MS(1, adc_proc_hys_cmp_task)

/*
 * Helper task for relay synchronization condition:
 * set cap_g_is_requal when capacitor and grid instantaneous voltages are close.
 */
static void adc_proc_g_cap_is_requal_task(void)
{
    if (fabsf(v_cap - v_g) < 5.0f)
    {
        cap_g_is_requal = 1;
    }
    else
    {
        cap_g_is_requal = 0;
    }
}

REG_TASK_MS(1, adc_proc_g_cap_is_requal_task)

/*
 * Final AC-availability decision task (100 us period):
 * 1) Update chk_grid window debouncer.
 * 2) If AC is currently invalid, wait until chk_grid reports valid.
 * 3) If AC is currently valid, drop to invalid when either
 *    - chk_grid leaves valid window, or
 *    - ac_loss_det reports waveform loss.
 * 4) On drop, reset both detectors to restart cleanly.
 */
static void adc_proc_ac_is_ok(void)
{
    chk_grid_func(&chk_grid);

    if (ac_is_ok == 0)
    {
        ac_is_ok = chk_grid.output.is_ok;
    }
    else
    {
        if (chk_grid.output.is_ok == 0)
        {
            chk_grid_reset(&chk_grid);
            ac_loss_det_reset(&ac_loss_det);
            ac_is_ok = 0;
        }
    }
}

REG_TASK(1, adc_proc_ac_is_ok)
