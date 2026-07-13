/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070203 JP     Initial.
 *  071326 MF,CC  O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                (numerics-changing). See issue #103.
 */

int linearreg_intercept_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode linearreg_intercept(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;

   int today, lookbackTotal, trailingIdx;
   double SumX, SumXY, SumY, SumXSqr, Divisor;

   double m;
   int i;

   double tempValue1, trailingValue;

   /* Linear Regression is a concept also known as the
    * "least squares method" or "best fit." Linear
    * Regression attempts to fit a straight line between
    * several data points in such a way that distance
    * between each data point and the line is minimized.
    *
    * For each point, a straight line over the specified
    * previous bar period is determined in terms
    * of y = b + m*x:
    *
    * TA_LINEARREG          : Returns b+m*(period-1)
    * TA_LINEARREG_SLOPE    : Returns 'm'
    * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
    * TA_LINEARREG_INTERCEPT: Returns 'b'
    * TA_TSF                : Returns b+m*(period)
    */

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = linearreg_intercept_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0; /* Index into the output. */
   today = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   SumX = optInTimePeriod * ( optInTimePeriod - 1 ) * 0.5;
   SumXSqr = optInTimePeriod * ( optInTimePeriod - 1 ) * ( 2 * optInTimePeriod - 1 ) / 6;
   Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

   /* Prime the two data-dependent window sums for the first output with a
    * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
    * SumY = sum of the window, SumXY = sum of i*value (i the reversed
    * 0..period-1 position). */
   SumXY = 0;
   SumY = 0;
   for( i = optInTimePeriod; i-- != 0; )
   {
      SumY += tempValue1 = inReal[today - i];
      SumXY += (double)i * tempValue1;
   }
   m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
   outReal[outIdx++] = ( SumY - m * SumX ) / (double)optInTimePeriod;
   today++;

   /* Slide the window one bar at a time, keeping both sums in O(1): advancing
    * the window raises every retained value's weight by 1 (adds SumY) and drops
    * the departing value at full weight (subtracts period*trailingValue). Same
    * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
    * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.) */
   while( today <= endIdx )
   {
      trailingValue = inReal[trailingIdx++];
      SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
      SumY = SumY - trailingValue + inReal[today];
      m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      outReal[outIdx++] = ( SumY - m * SumX ) / (double)optInTimePeriod;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
