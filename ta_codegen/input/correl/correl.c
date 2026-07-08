/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101003 MF   Initial Coding
 *  062804 MF   Resolve div by zero bug on limit case.
 *
 */

int correl_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode correl(int startIdx, int endIdx,
   const double inReal0[],
   const double inReal1[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumXY, sumX, sumY, sumX2, sumY2, x, y, trailingX, trailingY;
   double tempReal;
   int lookbackTotal, today, trailingIdx, outIdx;

   /* Move up the start index if there is not
    * enough initial data.
    */
   lookbackTotal = optInTimePeriod-1;
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx  = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   /* Calculate the initial values. */
   sumXY = sumX = sumY = sumX2 = sumY2 = 0.0;
   for( today=trailingIdx; today <= startIdx; today++ )
   {
      x = inReal0[today];
      sumX  += x;
      sumX2 += x*x;

      y = inReal1[today];
      sumXY += x*y;
      sumY  += y;
      sumY2 += y*y;
   }

   /* Write the first output.
    * Save first the trailing values since the input
    * and output might be the same array,
    */
   trailingX = inReal0[trailingIdx];
   trailingY = inReal1[trailingIdx++];
   tempReal = (sumX2-((sumX*sumX)/optInTimePeriod)) * (sumY2-((sumY*sumY)/optInTimePeriod));
   if( !TA_IS_ZERO_OR_NEG(tempReal) )
      outReal[0] = (sumXY-((sumX*sumY)/optInTimePeriod)) / sqrt(tempReal);
   else
      outReal[0] = 0.0;

   /* Tight loop to do subsequent values. */
   outIdx = 1;
   while( today <= endIdx )
   {
      /* Remove trailing values */
      sumX  -= trailingX;
      sumX2 -= trailingX*trailingX;

      sumXY -= trailingX*trailingY;
      sumY  -= trailingY;
      sumY2 -= trailingY*trailingY;

      /* Add new values */
      x = inReal0[today];
      sumX  += x;
      sumX2 += x*x;

      y = inReal1[today++];
      sumXY += x*y;
      sumY  += y;
      sumY2 += y*y;

      /* Output new coefficient.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2-((sumX*sumX)/optInTimePeriod)) * (sumY2-((sumY*sumY)/optInTimePeriod));
      if( !TA_IS_ZERO_OR_NEG(tempReal) )
         outReal[outIdx++] = (sumXY-((sumX*sumY)/optInTimePeriod)) / sqrt(tempReal);
      else
         outReal[outIdx++] = 0.0;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
