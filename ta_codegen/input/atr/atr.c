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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 *  090326 MF,CC  #338 Two-coefficient Wilder step; no divide in the
 *                loop-carried chain.
 *
 */

int atr_lookback(int optInTimePeriod)
{
   /* The ATR lookback is the sum of:
    *    1 + (optInTimePeriod - 1)
    *
    * Where 1 is for the True Range, and
    * (optInTimePeriod-1) is for the simple
    * moving average.
    */
   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_ATR);
}

TA_RetCode atr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   int nbATR;

   double prevATR, periodTotal, wAlpha, wBeta;
   double val2, val3, greatest;
   double tempCY, tempLT, tempHT;

   /* Average True Range is the greatest of the following:
    *
    *  val1 = distance from today's high to today's low.
    *  val2 = distance from yesterday's close to today's high.
    *  val3 = distance from yesterday's close to today's low.
    *
    * These value are averaged for the specified period using
    * Wilder method. This method have an unstable period comparable
    * to and Exponential Moving Average (EMA).
    */
   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = atr_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
    * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
    * measures closer to the exact recursion than the 1/period-first spelling
    * at nearly every period. Swapping them reddens nothing.
    * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
    */
   wBeta  = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   /* The True Range of each bar is computed inline in a single
    * pass. No temporary buffer is needed.
    *
    * The arithmetic order below is the bit-exactness contract
    * (do not reorder):
    *  - True Range: start from high-low, then compare/replace
    *    with the two previous-close distances, in that order.
    *  - Seed: the first 'period' True Range values are summed,
    *    accumulated from 0.0 in input order, then divided by
    *    the period.
    *  - Wilder smoothing: ONE statement. Splitting it back
    *    unfuses the multiply-add and puts a second latency on
    *    the recurrence's dependency chain.
    *
    * In-place (outReal being one of the input arrays) is
    * supported: each output is written only after every input
    * read at or before its bar, and the output index is always
    * smaller than the bar index of any remaining read.
    */

   /* The first True Range needs the two price bars at
    * startIdx-lookbackTotal+1 (a previous close is consumed).
    */
   today = startIdx - lookbackTotal + 1;

   /* Seed the ATR with a simple average of the True Range
    * for the first 'period' bars.
    */
   periodTotal = 0.0;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      /* Find the greatest of the 3 values. */
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      periodTotal += greatest;
      today++;
   }
   prevATR = periodTotal / optInTimePeriod;

   /* Skip the unstable period. */
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_ATR);
   while( i != 0 )
   {
      /* Find the greatest of the 3 values. */
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      today++;
      i--;
   }

   /* Now start to write the final ATR in the caller
    * provided outReal.
    */
   outIdx = 1;
   outReal[0] = prevATR;

   /* Now do the number of requested ATR. */
   nbATR = (endIdx - startIdx)+1;

   while( --nbATR != 0 )
   {
      /* Find the greatest of the 3 values. */
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      outReal[outIdx++] = prevATR;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
