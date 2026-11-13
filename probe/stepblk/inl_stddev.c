#include "ta_STDDEV.c"
#include "probe_blocks.h"

/* Codes are accumulated, not checked per bar: any failure fails the block. */
TA_RetCode PROBE_STDDEV_Block( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i, bad = 0;
   for( i = 0; i < n; i++ )
      bad |= TA_STDDEV_StepImpl( sp, in[i], &out[i] ) != TA_SUCCESS;
   if( bad )
      return TA_BAD_PARAM;
   sp->cur_outReal = out[n - 1];
   sp->outRangeCount += n;
   return TA_SUCCESS;
}

/* The map is TA_STDDEV_StepImpl's combine step, copied: keep it in step. */
void PROBE_STDDEV_BlockSub( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   PROBE_VAR_Block( sp->sub0, in, n, out );
   if( sp->optInNbDev != 1.0 )
   {
      for( i = 0; i < n; i++ )
         out[i] = sqrt(out[i]) * sp->optInNbDev;
   }
   else
   {
      for( i = 0; i < n; i++ )
         out[i] = sqrt(out[i]);
   }
   sp->cur_outReal = out[n - 1];
   sp->outRangeCount += n;
}

/* PROBE_STDDEV_BlockSub with VAR's mechanical kernel in place of its block. */
void PROBE_STDDEV_BlockSubK( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out )
{
   int i;
   PROBE_VAR_Kern( sp->sub0, in, n, out );
   if( sp->optInNbDev != 1.0 )
   {
      for( i = 0; i < n; i++ )
         out[i] = sqrt(out[i]) * sp->optInNbDev;
   }
   else
   {
      for( i = 0; i < n; i++ )
         out[i] = sqrt(out[i]);
   }
   sp->cur_outReal = out[n - 1];
   sp->outRangeCount += n;
}
