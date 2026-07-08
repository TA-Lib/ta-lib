/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070203 JP   Initial.
 *
 */

int linearreg_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode linearreg(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int outIdx;

   int today, lookbackTotal;
   double SumX, SumXY, SumY, SumXSqr, Divisor;

   double m, b;
   int i;

   double tempValue1;

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
   lookbackTotal = linearreg_lookback( optInTimePeriod );

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

   SumX = optInTimePeriod * ( optInTimePeriod - 1 ) * 0.5;
   SumXSqr = optInTimePeriod * ( optInTimePeriod - 1 ) * ( 2 * optInTimePeriod - 1 ) / 6;
   Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

   while( today <= endIdx )
   {
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0; )
      {
         SumY += tempValue1 = inReal[today - i];
         SumXY += (double)i * tempValue1;
      }
      m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = ( SumY - m * SumX ) / (double)optInTimePeriod;
      outReal[outIdx++] = b + m * (double)(optInTimePeriod-1);
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
