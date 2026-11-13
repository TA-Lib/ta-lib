#include "ta_EMA.c"
#include "probe_blocks.h"

/* A target_clones pair like TA_EMA_Update: the fused step is always_inline
 * and runs at the ISA of the clone it lands in. */
TA_FMA_MULTIVERSION
void PROBE_EMA_Block( TA_EMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_EMA_StepImpl( sp, in[i], &out[i] );
   sp->outRangeCount += n;
}

#define KERN_R
#define KERN_NAME PROBE_EMA_Kern
#include "kern_ema.h"
#undef KERN_R
#undef KERN_NAME
#define KERN_R restrict
#define KERN_NAME PROBE_EMA_KernR
#include "kern_ema.h"
#undef KERN_R
#undef KERN_NAME
