#include "ta_SMA.c"
#include "probe_blocks.h"

void PROBE_SMA_Block( TA_SMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_SMA_StepImpl( sp, in[i], &out[i] );
   sp->outRangeCount += n;
}

#define KERN_R
#define KERN_NAME PROBE_SMA_Kern
#include "kern_sma.h"
#undef KERN_R
#undef KERN_NAME
#define KERN_R restrict
#define KERN_NAME PROBE_SMA_KernR
#include "kern_sma.h"
#undef KERN_R
#undef KERN_NAME
