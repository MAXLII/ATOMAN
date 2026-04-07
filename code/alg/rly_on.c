#include "rly_on.h"
#include "my_math.h"
#include <math.h>

/**
 * @brief Initialize relay control module
 *
 * This module manages relay ON/OFF behavior using a state machine.
 *
 * Functional overview:
 *  - External logic sets p_rly_on_trig to request relay ON.
 *  - External logic sets p_rly_off_trig to request relay OFF.
 *  - Relay ON is allowed only when p_is_equal == 1
 *    (e.g., voltage synchronization condition satisfied).
 *  - Relay closing moment is aligned to grid frequency.
 *  - Delay is calculated in control cycles using ctrl_freq.
 *
 * @param p_str             Pointer to relay control instance
 * @param p_rly_on_trig     External ON trigger flag (pulse type)
 * @param p_rly_off_trig    External OFF trigger flag (pulse type)
 * @param p_is_equal        Synchronization condition flag
 * @param p_freq            Pointer to grid frequency (Hz)
 * @param ctrl_freq         Control loop frequency (Hz)
 * @param rly_on_time_def   Desired relay closing time offset (seconds)
 * @param rly_on            Hardware relay ON callback
 * @param rly_off           Hardware relay OFF callback
 */
void rly_on_init(rly_on_t *p_str,
                 uint8_t *p_rly_on_trig,
                 uint8_t *p_rly_off_trig,
                 uint8_t *p_is_equal,
                 float *p_freq,
                 float ctrl_freq,
                 float rly_on_time_def,
                 void (*rly_on)(void),
                 void (*rly_off)(void))
{
    if (p_str == NULL)
    {
        return;
    }

    /* Bind external input signals */
    p_str->input.p_freq = p_freq;
    p_str->input.p_is_equal = p_is_equal;
    p_str->input.p_rly_off_trig = p_rly_off_trig;
    p_str->input.p_rly_on_trig = p_rly_on_trig;

    /* Configuration parameters */
    p_str->cfg.ctrl_freq = ctrl_freq;
    p_str->cfg.rly_on_time_def = rly_on_time_def;

    /* Hardware callback binding */
    p_str->func.rly_off = rly_off;
    p_str->func.rly_on = rly_on;

    /* Reset runtime state */
    p_str->inter.dly_cnt = 0U;
    p_str->inter.dly = 0U;
    p_str->inter.on_cnt = 0U;
    p_str->inter.on_confirm = 0U;
    p_str->inter.sta = RLY_ON_STA_INIT;
    p_str->output.is_closed = 0U;
}

/**
 * @brief Relay control state machine
 *
 * This function must be called periodically at ctrl_freq.
 *
 * State transition flow:
 *
 * INIT -> IDLE -> WAIT -> DLY -> RUN
 *                         ↑
 *                        OFF trigger
 *
 * Description of states:
 *
 * INIT:
 *   Validate hardware callbacks and prepare module.
 *
 * IDLE:
 *   Wait for ON trigger.
 *
 * WAIT:
 *   Wait for synchronization condition (p_is_equal == 1).
 *
 * DLY:
 *   Calculate delay based on grid period and desired closing time.
 *   Wait required control cycles before activating relay.
 *
 * RUN:
 *   Relay is closed. Wait for OFF trigger.
 */
void rly_on_func(rly_on_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->input.p_rly_on_trig == NULL) ||
        (p_str->input.p_rly_off_trig == NULL) ||
        (p_str->input.p_is_equal == NULL) ||
        (p_str->input.p_freq == NULL))
    {
        return;
    }

    switch (p_str->inter.sta)
    {
    case RLY_ON_STA_INIT:
        /**
         * INIT state:
         * Ensure hardware callbacks are valid.
         * If not valid, remain in INIT state.
         */
        if ((p_str->func.rly_off == NULL) ||
            (p_str->func.rly_on == NULL))
        {
            return;
        }
        else
        {
            /* Move to IDLE state */
            p_str->inter.sta = RLY_ON_STA_IDLE;

            /* Clear ON trigger to avoid unintended activation */
            *p_str->input.p_rly_on_trig = 0;
        }
        break;

    case RLY_ON_STA_IDLE:
        /**
         * IDLE state:
         * Wait for external ON trigger.
         * Trigger is edge-like: cleared immediately after detection.
         */
        if (*p_str->input.p_rly_on_trig == 1)
        {
            *p_str->input.p_rly_on_trig = 0;
            p_str->inter.dly_cnt = 0U;
            p_str->inter.dly = 0U;
            p_str->inter.on_cnt = 0U;
            p_str->inter.on_confirm = 0U;
            p_str->output.is_closed = 0U;
            p_str->inter.sta = RLY_ON_STA_WAIT;
        }
        break;

    case RLY_ON_STA_WAIT:
        /**
         * WAIT state:
         * Wait until synchronization condition is satisfied.
         * Example use case:
         *   - Voltage amplitude match
         *   - Phase alignment match
         */
        if (*p_str->input.p_is_equal == 1)
        {
            /* Read grid frequency */
            float freq_temp = *p_str->input.p_freq;

            /* Apply lower limit to avoid division by zero or unrealistic low frequency */
            DN_LMT(freq_temp, 10.0f);

            /* Grid period (seconds) */
            float grid_ts = 1.0f / freq_temp;

            /*
             * Design intent:
             * - detect "equal voltage" as the timing reference;
             * - relay mechanical close time is ton = rly_on_time_def;
             * - issue relay-on command after tdelay so that:
             *       tdelay + ton = N * grid_ts
             *   and the contacts are expected to close near the next
             *   equal-voltage instant.
             *
             * Therefore:
             *   tdelay = (grid_ts - (ton mod grid_ts)) mod grid_ts
             */
            float rly_on_time = p_str->cfg.rly_on_time_def;
            float rly_on_time_mod = fmodf(rly_on_time, grid_ts);
            float dly_s = 0.0f;

            if (rly_on_time_mod < 0.0f)
            {
                rly_on_time_mod += grid_ts;
            }

            if (rly_on_time_mod > 0.0f)
            {
                dly_s = grid_ts - rly_on_time_mod;
            }

            p_str->inter.dly = (uint32_t)(p_str->cfg.ctrl_freq * dly_s + 0.5f);
            p_str->inter.on_confirm = (uint32_t)(p_str->cfg.ctrl_freq * p_str->cfg.rly_on_time_def + 0.5f);
            p_str->inter.dly_cnt = 0U;
            p_str->inter.on_cnt = 0U;
            p_str->output.is_closed = 0U;
            p_str->inter.sta = RLY_ON_STA_DLY;
        }
        break;

    case RLY_ON_STA_DLY:
        /**
         * DLY state:
         * Wait until dly_cnt reaches computed delay.
         * dly_cnt increments once per control call.
         */
        if (p_str->inter.dly_cnt >= p_str->inter.dly)
        {
            /* Execute hardware relay ON */
            p_str->func.rly_on();

            /* Reset delay counter */
            p_str->inter.dly_cnt = 0U;
            p_str->inter.on_cnt = 0U;
            p_str->output.is_closed = 0U;

            /* Enter RUN state */
            p_str->inter.sta = RLY_ON_STA_RUN;

            /* Clear OFF trigger to avoid immediate shutdown */
            *p_str->input.p_rly_off_trig = 0;
        }
        else
        {
            /* Increment delay counter */
            p_str->inter.dly_cnt++;
        }
        break;

    case RLY_ON_STA_RUN:
        /**
         * RUN state:
         * Relay is active.
         * Wait for OFF trigger.
         */
        if (p_str->output.is_closed == 0U)
        {
            if (p_str->inter.on_cnt >= p_str->inter.on_confirm)
            {
                p_str->output.is_closed = 1U;
            }
            else
            {
                p_str->inter.on_cnt++;
            }
        }

        if (*p_str->input.p_rly_off_trig == 1)
        {
            *p_str->input.p_rly_off_trig = 0;

            /* Execute hardware relay OFF */
            p_str->func.rly_off();

            /* Return to IDLE state */
            p_str->inter.sta = RLY_ON_STA_IDLE;
            p_str->inter.dly_cnt = 0U;
            p_str->inter.dly = 0U;
            p_str->inter.on_cnt = 0U;
            p_str->inter.on_confirm = 0U;
            p_str->output.is_closed = 0U;

            /* Clear ON trigger for safety */
            *p_str->input.p_rly_on_trig = 0;
        }
        break;

    case RLY_ON_STA_ERR:
        /**
         * ERR state:
         * Reserved for future error handling.
         * No recovery logic implemented.
         */
        break;

    default:
        /**
         * Undefined state.
         * No recovery mechanism implemented.
         */
        break;
    }
}
