#include "ctrl_llc_dsg.h"
#include "section.h"
#include "my_math.h"
#include "adc_chk.h"
#include "pwm.h"
#include "llc_hardware.h"
#include "linear.h"
#include "llc_fsm.h"

static uint8_t enable = 0;
static float llc_pfm = 0.0f;

static float llc_pfm_table[][2] = {
    {0.0f, 0.033f},
    {0.1f, 0.042f},
    {0.2f, 0.054f},
    {0.3f, 0.078f},
    {0.38f, 0.142f},
    {0.4f, 1.0f},
};

static float llc_dsg_time = 0.0f;

static linear_t llc_pfm_linear = {0};

void ctrl_llc_dsg_init(void)
{
   linear_init(&llc_pfm_linear,
               &llc_dsg_time,
               llc_pfm_table,
               ARRAY_SIZE(llc_pfm_table));
   llc_pfm = 0.0f;
   if (llc_fsm_get_is_ups_trig())
   {
      llc_dsg_time = 0.39f;
   }
   else
   {
      llc_dsg_time = 0.0f;
   }
}

REG_INIT(1, ctrl_llc_dsg_init)

void ctlr_llc_dsg_func(void)
{
   if (enable == 0)
   {
      return;
   }
   llc_dsg_time += CTRL_TS;

   linear_func(&llc_pfm_linear);

   pwm_llc_dsg_func(llc_pfm_linear.out);
}

REG_INTERRUPT(2, ctlr_llc_dsg_func)

void ctrl_llc_dsg_enable(void)
{
   ctrl_llc_dsg_init();
   pwm_llc_dsg_enable();
   enable = 1;
}

void ctrl_llc_dsg_disable(void)
{
   pwm_llc_dsg_disable();
   enable = 0;
}

uint8_t ctrl_llc_dsg_get_enable(void)
{
   return enable;
}

#ifdef IS_PLECS

#include "plecs.h"

void ctrl_llc_dsg_scope(void)
{
}
REG_INTERRUPT(8, ctrl_llc_dsg_scope)

#endif
