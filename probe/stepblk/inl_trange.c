#include "ta_TRANGE.c"
#include "probe_blocks.h"

void PROBE_TRANGE_Block( TA_TRANGE_Stream *restrict sp, const double *restrict high, const double *restrict low,
                         const double *restrict close, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_TRANGE_StepImpl( sp, high[i], low[i], close[i], &out[i] );
   sp->outRangeCount += n;
}
