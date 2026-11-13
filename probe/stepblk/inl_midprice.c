#include "ta_MIDPRICE.c"
#include "probe_blocks.h"

void PROBE_MIDPRICE_Block( TA_MIDPRICE_Stream *restrict sp, const double *restrict high,
                           const double *restrict low, int n, double *restrict out )
{
   int i;
   for( i = 0; i < n; i++ )
      TA_MIDPRICE_StepImpl( sp, high[i], low[i], &out[i] );
   sp->outRangeCount += n;
}
