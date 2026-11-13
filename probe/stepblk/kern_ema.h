/* TA_EMA_StepImpl's body by the mechanical rule (probe_blocks.h), included
 * twice by inl_ema.c: KERN_NAME and KERN_R (empty or restrict) pick the copy.
 * The step's early `return` (period 1) ends one bar, so it is `continue`. */
TA_FMA_MULTIVERSION
void KERN_NAME( TA_EMA_Stream *KERN_R sp, const double *KERN_R in, int n, double *KERN_R out )
{
   double cur_outReal = sp->cur_outReal;
   int optInTimePeriod = sp->optInTimePeriod;
   double emaBeta = sp->emaBeta;
   double optInK_1 = sp->optInK_1;
   double prevMA = sp->prevMA;
   int kernBar;

   for( kernBar = 0; kernBar < n; kernBar++ )
   {
      double inReal = in[kernBar];

      if( optInTimePeriod == 1 )
      {
         out[kernBar]= inReal;
         cur_outReal = out[kernBar];
         continue;
      }
      prevMA = fma(emaBeta, prevMA, optInK_1 * inReal);
      out[kernBar]= prevMA;
      cur_outReal = out[kernBar];
   }
   sp->cur_outReal = cur_outReal;
   sp->prevMA = prevMA;
   sp->outRangeCount += n;
}
