#include "ta_VAR.c"
#include "probe_blocks.h"

void PROBE_VAR_Block( TA_VAR_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_VAR_StepImpl( sp, in[i], &out[i] );
   sp->outRangeCount += n;
}

#define KERN_R
#define KERN_NAME PROBE_VAR_Kern
#include "kern_var.h"
#undef KERN_R
#undef KERN_NAME
#define KERN_R restrict
#define KERN_NAME PROBE_VAR_KernR
#include "kern_var.h"
#undef KERN_R
#undef KERN_NAME
