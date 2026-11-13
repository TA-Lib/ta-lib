#include "ta_LINEARREG.c"
#include "probe_blocks.h"

/* A target_clones pair like TA_LINEARREG_Update: the fused step is
 * always_inline and runs at the ISA of the clone it lands in. */
TA_FMA_MULTIVERSION
void PROBE_LINEARREG_Block( TA_LINEARREG_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_LINEARREG_StepImpl( sp, in[i], &out[i] );
   sp->outRangeCount += n;
}

#define KERN_R
#define KERN_NAME PROBE_LINEARREG_Kern
#include "kern_linearreg.h"
#undef KERN_R
#undef KERN_NAME
#define KERN_R restrict
#define KERN_NAME PROBE_LINEARREG_KernR
#include "kern_linearreg.h"
#undef KERN_R
#undef KERN_NAME
