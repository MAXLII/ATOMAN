#ifndef __LLC_HARDWARE_H
#define __LLC_HARDWARE_H

#define CAP_BAT_VALUE 56e-3f
#define CAP_BUS_VALUE 200e-6f
#define TF_TURNS_RATIO 64.0f

#define LLC_CR 100e-9f
#define LLC_LR 60e-6f
#define LLC_LM 245e-6f

#define LLC_CR_LR_SQ 2.4494898e-6f    // sqrt(CR*LR)
#define LLC_CR_LR_LM_SQ 5.5226806e-6f // sqrt(CR*(LR+LM))

#define LLC_FR (1.0f / (2.0f * M_PI * LLC_CR_LR_SQ))
#define LLC_FM (1.0f / (2.0f * M_PI * LLC_CR_LR_LM_SQ))

#endif
