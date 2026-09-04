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
 *  060306 MF     Initial Version
 *  070526 MF,CC  Fix #98: partial-range calls normalized with a close
 *                from the wrong bar (TR-buffer-relative index).
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 *  082326 MF,CC  Fix #253. Test the close exactly instead of against the fixed
 *                TA_IS_ZERO band, which zeroed the output for any instrument
 *                quoted small enough to fall under it.
 *  090326 MF,CC  #338 Two-coefficient Wilder step; no divide in the
 *                loop-carried chain.
 */

int natr_lookback(int optInTimePeriod)
{
   /* The ATR lookback is the sum of:
    *    1 + (optInTimePeriod - 1)
    *
    * Where 1 is for the True Range, and
    * (optInTimePeriod-1) is for the simple
    * moving average.
    */
   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_NATR);
}

TA_RetCode natr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   int nbATR;

   double prevATR, periodTotal, tempValue, wAlpha, wBeta;
   double val2, val3, greatest;
   double tempCY, tempLT, tempHT;

   /* This function is very similar as ATR, except
    * it is being normalized as follow:
    *
    *    NATR = (ATR(period) / Close) * 100
    *
    *
    * Normalization make the ATR function more relevant
    * in the folllowing scenario:
    *    - Long term analysis where the price changes drastically.
    *    - Cross-market or cross-security ATR comparison.
    *
    * More Info:
    *      Technical Analysis of Stock & Commodities (TASC)
    *      May 2006 by John Forman
    */

   /* Average True Range is the greatest of the following:
    *
    *  val1 = distance from today's high to today's low.
    *  val2 = distance from yesterday's close to today's high.
    *  val3 = distance from yesterday's close to today's low.
    *
    * These value are averaged for the specified period using
    * Wilder method. This method have an unstable period comparable
    * to an Exponential Moving Average (EMA).
    */
   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = natr_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
    * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
    * measures closer to the exact recursion than the 1/period-first spelling
    * at nearly every period. The order is a gated contract, not a preference:
    * swapping it reddens the frozen v0.6.4 comparison.
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
    * Each output is normalized by the close of its own bar; a
    * close of zero yields 0.0.
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
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_NATR);
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

   /* Now start to write the final NATR in the caller
    * provided outReal.
    */
   outIdx = 1;
   if( optInTimePeriod <= 1 )
   {
      /* Period 1 is the raw True Range and is deliberately NOT normalized,
       * which is the TRANGE delegation this path replaced. */
      outReal[0] = prevATR;
   }
   else
   {
      /* NATR is the ATR as a percentage of the close, so it is scale-free and
       * the divisor only has to be non-zero. An exact test, not the fixed
       * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
       * band zeroed the whole output for any instrument quoted below it (#253).
       */
      tempValue = inClose[startIdx];
      if( tempValue != 0.0 )
         outReal[0] = (prevATR/tempValue)*100.0;
      else
         outReal[0] = 0.0;
   }

   /* Now do the number of requested NATR. */
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
      if( optInTimePeriod <= 1 )
      {
         outReal[outIdx] = prevATR;
      }
      else
      {
         tempValue = inClose[today];
         if( tempValue != 0.0 )
            outReal[outIdx] = (prevATR/tempValue)*100.0;
         else
            outReal[outIdx] = 0.0;
      }
      outIdx++;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
