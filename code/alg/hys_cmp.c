#include "hys_cmp.h"
#include "my_math.h"
#include "string.h"

/**
 * @brief Initialize hysteresis comparator instance
 *
 * @param p_str   Comparator object
 * @param p_val   Pointer to monitored variable
 * @param thr     Assertion threshold
 * @param thr_hys Release threshold
 * @param time    Required consecutive count to assert
 * @param time_hys Required consecutive count to release
 * @param p_cmp_func Assertion comparator
 * @param p_cmp_hys_func Release comparator
 *
 * Behavior:
 *  - Binds input pointer
 *  - Copies configuration values
 *  - Resets internal counter
 *  - Clears assertion state
 */
void hys_cmp_init(hys_cmp_t *p_str,
                  float *p_val,
                  float thr,
                  float thr_hys,
                  uint32_t time,
                  uint32_t time_hys,
                  uint8_t (*p_cmp_func)(float val, float thr),
                  uint8_t (*p_cmp_hys_func)(float val, float thr))
{
    p_str->input.p_val = p_val;
    p_str->inter.cfg.thr = thr;
    p_str->inter.cfg.thr_hys = thr_hys;
    p_str->inter.cfg.time = time;
    p_str->inter.cfg.time_hys = time_hys;
    p_str->inter.cfg.p_cmp_func = p_cmp_func;
    p_str->inter.cfg.p_cmp_hys_func = p_cmp_hys_func;
    p_str->inter.cnt = 0;
    p_str->output.is_asserted = 0;
}

/**
 * @brief Execute hysteresis comparator logic
 *
 * This function must be called periodically.
 *
 * Functional principle:
 *
 * 1) ASSERT path (state = 0)
 *    If assertion condition holds continuously for cfg->time cycles,
 *    state transitions to 1.
 *
 * 2) RELEASE path (state = 1)
 *    If release condition holds continuously for cfg->time_hys cycles,
 *    state transitions back to 0.
 *
 * Counter behavior:
 *    - Increments when condition is satisfied
 *    - Decrements (via DN_CNT) when condition fails
 *
 * This provides:
 *    - Threshold comparison
 *    - Hysteresis window
 *    - Time qualification
 *
 * Result:
 *    output.is_asserted reflects stable comparator state.
 */
void hys_cmp_func(hys_cmp_t *p_str)
{
    /* Defensive programming: validate pointers */
    if ((p_str == NULL) ||
        (p_str->input.p_val == NULL))
    {
        return;
    }

    hys_cmp_cfg_t *cfg = &p_str->inter.cfg;
    float val = *(p_str->input.p_val);

    uint8_t *state = &p_str->output.is_asserted;
    uint32_t *cnt = &p_str->inter.cnt;

    /* =====================================================
       ASSERT PHASE
       Condition: state == 0
       Goal: detect entry condition
       ===================================================== */
    if (*state == 0U)
    {
        if ((cfg->p_cmp_func != NULL) &&
            cfg->p_cmp_func(val, cfg->thr))
        {
            /* Condition satisfied → increment counter */
            (*cnt)++;

            /* Time qualification satisfied */
            if (*cnt > cfg->time)
            {
                *state = 1U; /* Assert comparator */
                *cnt = 0U;   /* Reset counter */
            }
        }
        else
        {
            /* Condition failed → decay counter */
            DN_CNT(*cnt);
        }
    }

    /* =====================================================
       RELEASE PHASE
       Condition: state == 1
       Goal: detect exit condition
       ===================================================== */
    else
    {
        if ((cfg->p_cmp_hys_func != NULL) &&
            cfg->p_cmp_hys_func(val, cfg->thr_hys))
        {
            /* Release condition satisfied */
            (*cnt)++;

            if (*cnt > cfg->time_hys)
            {
                *state = 0U; /* Deassert comparator */
                *cnt = 0U;
            }
        }
        else
        {
            /* Release condition failed → decay counter */
            DN_CNT(*cnt);
        }
    }
}

/**
 * @brief Greater-than comparator helper
 *
 * Returns 1 if val > thr, otherwise 0.
 */
uint8_t cmp_gt(float val, float thr)
{
    return (uint8_t)(val > thr);
}

/**
 * @brief Less-than comparator helper
 *
 * Returns 1 if val < thr, otherwise 0.
 */
uint8_t cmp_lt(float val, float thr)
{
    return (uint8_t)(val < thr);
}
