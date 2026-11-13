/* TA_LINEARREG_StepImpl's body by the mechanical rule (probe_blocks.h),
 * included twice by inl_linearreg.c: KERN_NAME and KERN_R (empty or restrict)
 * pick the copy. The step is TA_FMA_STEP_INLINE, so the kernel takes the
 * attribute of the step's own callers, TA_FMA_MULTIVERSION. */
TA_FMA_MULTIVERSION
void KERN_NAME( TA_LINEARREG_Stream *KERN_R sp, const double *KERN_R in, int n, double *KERN_R out )
{
   double cur_outReal = sp->cur_outReal;
   int optInTimePeriod = sp->optInTimePeriod;
   int lookbackTotal = sp->lookbackTotal;
   int trailingIdx = sp->trailingIdx;
   double SumX = sp->SumX;
   double SumXY = sp->SumXY;
   double SumY = sp->SumY;
   double Divisor = sp->Divisor;
   int barsSinceReseed = sp->barsSinceReseed;
   double trailingValue = sp->trailingValue;
   double sumAbs = sp->sumAbs;
   int j = sp->j;
   int today = sp->today;
   int xMask = sp->xMask;
   double *x_inReal = sp->x_inReal;
   int kernBar;

   for( kernBar = 0; kernBar < n; kernBar++ )
   {
      double inReal = in[kernBar];
      double m;
      double b;
      int windowStart;
      double tempValue1;
      double tempValue2;
      double weightedTrailing;

      x_inReal[today & xMask] = inReal;
      weightedTrailing = (double)optInTimePeriod * trailingValue;
      SumXY = SumXY + SumY - weightedTrailing;
      SumY = SumY - trailingValue + x_inReal[today & xMask];
      sumAbs = sumAbs - fabs(trailingValue) + fabs(x_inReal[today & xMask]);
      barsSinceReseed -= 1;
      if( barsSinceReseed <= 0 || fabs(weightedTrailing) > 100.0 * sumAbs )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;
         SumY = 0;
         SumXY = 0;
         sumAbs = 0;
         tempValue2 = (double)lookbackTotal;
         for( j = windowStart; j <= today; j += 1 )
         {
            tempValue1 = x_inReal[j & xMask];
            SumY += tempValue1;
            SumXY += tempValue2 * tempValue1;
            sumAbs += fabs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      trailingValue = x_inReal[trailingIdx & xMask];
      trailingIdx += 1;
      out[kernBar]= fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
      cur_outReal = out[kernBar];
   }
   sp->cur_outReal = cur_outReal;
   sp->trailingIdx = trailingIdx;
   sp->SumXY = SumXY;
   sp->SumY = SumY;
   sp->barsSinceReseed = barsSinceReseed;
   sp->trailingValue = trailingValue;
   sp->sumAbs = sumAbs;
   sp->j = j;
   sp->today = today;
   sp->outRangeCount += n;
}
