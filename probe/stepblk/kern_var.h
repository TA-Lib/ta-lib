/* TA_VAR_StepImpl's body by the mechanical rule (probe_blocks.h), included
 * twice by inl_var.c: KERN_NAME and KERN_R (empty or restrict) pick the copy. */
void KERN_NAME( TA_VAR_Stream *KERN_R sp, const double *KERN_R in, int n, double *KERN_R out )
{
   double cur_outReal = sp->cur_outReal;
   int optInTimePeriod = sp->optInTimePeriod;
   double shift = sp->shift;
   double periodTotal1 = sp->periodTotal1;
   double periodTotal2 = sp->periodTotal2;
   double invPeriod = sp->invPeriod;
   double peakTotal2 = sp->peakTotal2;
   int trailingIdx = sp->trailingIdx;
   int nbInitialElementNeeded = sp->nbInitialElementNeeded;
   int barsSinceReseed = sp->barsSinceReseed;
   int j = sp->j;
   int windowStart = sp->windowStart;
   int i = sp->i;
   int xMask = sp->xMask;
   double *x_inReal = sp->x_inReal;
   int kernBar;

   for( kernBar = 0; kernBar < n; kernBar++ )
   {
      double inReal = in[kernBar];
      double tempReal;
      double meanValue1;
      double variance;

      x_inReal[i & xMask] = inReal;
      tempReal = x_inReal[i & xMask] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;
      peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
      tempReal = x_inReal[trailingIdx & xMask] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx += 1;
      barsSinceReseed -= 1;
      if( variance < 0.000001 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = i - nbInitialElementNeeded;
         tempReal = 0.0;
         for( j = windowStart; j <= i; j += 1 )
         {
            tempReal += x_inReal[j & xMask];
         }
         shift = tempReal * invPeriod;
         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j = windowStart; j <= i; j += 1 )
         {
            tempReal = x_inReal[j & xMask] - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) )
         {
            shift = x_inReal[i & xMask];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 )
            {
               tempReal = x_inReal[j & xMask] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         }
         peakTotal2 = periodTotal2;
         if( variance < 0.000000000001 * (periodTotal2 * invPeriod) )
         {
            variance = 0.0;
         }
         tempReal = x_inReal[windowStart & xMask] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }
      out[kernBar]= variance;
      i += 1;
      cur_outReal = out[kernBar];
   }
   sp->cur_outReal = cur_outReal;
   sp->shift = shift;
   sp->periodTotal1 = periodTotal1;
   sp->periodTotal2 = periodTotal2;
   sp->peakTotal2 = peakTotal2;
   sp->trailingIdx = trailingIdx;
   sp->barsSinceReseed = barsSinceReseed;
   sp->j = j;
   sp->windowStart = windowStart;
   sp->i = i;
   sp->outRangeCount += n;
}
