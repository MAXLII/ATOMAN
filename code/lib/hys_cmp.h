#ifndef __HYS_CMP_H
#define __HYS_CMP_H

#include "stdint.h"

/**
 * @brief Input structure for hysteresis comparator
 *
 * p_val:
 *   Pointer to the monitored value.
 *   The comparator reads this value each time hys_cmp_func() is executed.
 *
 * NOTE:
 *   The pointed value must remain valid during the lifetime of the module.
 */
typedef struct
{
    float *p_val;
} hys_cmp_input_t;

/**
 * @brief Configuration structure for hysteresis comparator
 *
 * thr:
 *   Assertion threshold.
 *
 * thr_hys:
 *   Release threshold (hysteresis threshold).
 *
 * time:
 *   Required consecutive count to assert.
 *
 * time_hys:
 *   Required consecutive count to release.
 *
 * p_cmp_func:
 *   Comparator function used during assertion phase.
 *   Example: cmp_gt or cmp_lt.
 *
 * p_cmp_hys_func:
 *   Comparator function used during release phase.
 *   Allows directional flexibility.
 */
typedef struct
{
    float thr;
    float thr_hys;
    uint32_t time;
    uint32_t time_hys;
    uint8_t (*p_cmp_func)(float val, float thr);
    uint8_t (*p_cmp_hys_func)(float val, float thr);
} hys_cmp_cfg_t;

/**
 * @brief Internal runtime state
 *
 * cfg:
 *   Local copy of the configuration structure.
 *
 * cnt:
 *   Internal counter used for time qualification.
 */
typedef struct
{
    hys_cmp_cfg_t cfg;
    uint32_t cnt;
} hys_cmp_inter_t;

/**
 * @brief Comparator output state
 *
 * is_asserted:
 *   0 -> comparator not asserted
 *   1 -> comparator asserted
 *
 * This represents the stable output state of the hysteresis comparator.
 */
typedef struct
{
    uint8_t is_asserted;
} hys_cmp_output_t;

/**
 * @brief Complete hysteresis comparator object
 *
 * input  -> monitored value
 * inter  -> runtime state
 * output -> stable comparator result
 */
typedef struct
{
    hys_cmp_input_t input;
    hys_cmp_inter_t inter;
    hys_cmp_output_t output;
} hys_cmp_t;

/* Initialization */
void hys_cmp_init(hys_cmp_t *p_str,
                  float *p_val,
                  float thr,
                  float thr_hys,
                  uint32_t time,
                  uint32_t time_hys,
                  uint8_t (*p_cmp_func)(float val, float thr),
                  uint8_t (*p_cmp_hys_func)(float val, float thr));

/* Periodic execution function */
void hys_cmp_func(hys_cmp_t *p_str);

/* Basic comparator helpers */
uint8_t cmp_gt(float val, float thr);

uint8_t cmp_lt(float val, float thr);

#endif
