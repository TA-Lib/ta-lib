/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090426 MF,CC  First version. See issue #345.
 */

int fosc_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode fosc(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;

   int today, lookbackTotal, trailingIdx;
   double SumX, SumXY, SumY, SumXSqr, Divisor;

   double m, b, closeValue;
   int i, j, windowStart, barsSinceReseed;

   double tempValue1, tempValue2, trailingValue, weightedTrailing, sumAbs;

   /* The forecast compared against the close is the one made one bar EARLIER:
    * FOSC[t] = 100*(inReal[t] - TSF[t-1])/inReal[t]. The regression window
    * ends at t-1, so the lookback is period, not period-1.
    *
    * The window arithmetic below is TA_TSF's verbatim -- priming scan, O(1)
    * recurrence, both re-anchor triggers (#254) and the fused
    * `b + m*(double)optInTimePeriod` -- which is what makes FOSC bit-identical
    * to a TA_TSF call anchored one bar earlier. Reshaping any of it breaks
    * that silently.
    *
    * trailingValue -- the value the NEXT bar's window drops -- is read before
    * the output write because with outReal==inReal (#130) that write lands on
    * exactly that cell whenever startIdx is the clamped minimum. closeValue
    * carries no such constraint: it sits startIdx bars ahead of the cursor.
    */

   lookbackTotal = fosc_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   SumX = (double)optInTimePeriod * ( optInTimePeriod - 1 ) * 0.5;
   SumXSqr = (double)optInTimePeriod * ( optInTimePeriod - 1 ) * ( 2 * optInTimePeriod - 1 ) / 6.0;
   Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

   SumXY = 0;
   SumY = 0;
   sumAbs = 0;
   for( i = optInTimePeriod; i-- != 0; )
   {
      SumY += tempValue1 = inReal[today - 1 - i];
      SumXY += (double)i * tempValue1;
      sumAbs += fabs(tempValue1);
   }
   m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
   b = ( SumY - m * SumX ) / (double)optInTimePeriod;
   barsSinceReseed = 32 * optInTimePeriod;
   trailingValue = inReal[trailingIdx];
   trailingIdx++;
   closeValue = inReal[today];
   if( closeValue != 0.0 )
      outReal[outIdx++] = 100.0 * ( closeValue - ( b + m * (double)optInTimePeriod ) ) / closeValue;
   else
      outReal[outIdx++] = 0.0;
   today++;

   while( today <= endIdx )
   {
      weightedTrailing = (double)optInTimePeriod * trailingValue;
      SumXY = SumXY + SumY - weightedTrailing;
      SumY = SumY - trailingValue + inReal[today - 1];
      sumAbs = sumAbs - fabs(trailingValue) + fabs(inReal[today - 1]);

      barsSinceReseed--;
      if( barsSinceReseed <= 0 || fabs(weightedTrailing) > 100.0 * sumAbs )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;
         SumY = 0;
         SumXY = 0;
         sumAbs = 0;
         tempValue2 = (double)( optInTimePeriod - 1 );
         for( j = windowStart; j < today; j++ )
         {
            tempValue1 = inReal[j];
            SumY += tempValue1;
            SumXY += tempValue2 * tempValue1;
            sumAbs += fabs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = ( SumY - m * SumX ) / (double)optInTimePeriod;
      trailingValue = inReal[trailingIdx];
      trailingIdx++;
      closeValue = inReal[today];
      if( closeValue != 0.0 )
         outReal[outIdx++] = 100.0 * ( closeValue - ( b + m * (double)optInTimePeriod ) ) / closeValue;
      else
         outReal[outIdx++] = 0.0;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
