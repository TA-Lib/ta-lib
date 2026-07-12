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

   double prevATR, periodTotal, tempValue;
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

   /* Period 1 needs no smoothing: the Wilder recursion below degenerates
    * to the raw True Range at every bar (prevATR = (prevATR*0 + TR)/1 = TR).
    * At period 1 the output is left as that raw True Range (unnormalized),
    * matching the historical TRANGE-delegation behavior; every period > 1 is
    * normalized by the close. The single general path handles all period >= 1.
    */

   /* The True Range of each bar is computed inline in a single
    * pass. No temporary buffer is needed.
    *
    * The arithmetic order below is the bit-exactness contract
    * (do not reorder or fuse operations):
    *  - True Range: start from high-low, then compare/replace
    *    with the two previous-close distances, in that order.
    *  - Seed: the first 'period' True Range values are summed,
    *    accumulated from 0.0 in input order, then divided by
    *    the period.
    *  - Wilder smoothing: multiply by period-1, add the True
    *    Range, divide by period, as three separate statements.
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

   /* Subsequent value are smoothed using the
    * previous ATR value (Wilder's approach).
    *  1) Multiply the previous ATR by 'period-1'.
    *  2) Add today TR value.
    *  3) Divide by 'period'.
    */

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

      prevATR *= optInTimePeriod - 1;
      prevATR += greatest;
      prevATR /= optInTimePeriod;
      today++;
      i--;
   }

   /* Now start to write the final NATR in the caller
    * provided outReal.
    */
   outIdx = 1;
   if( optInTimePeriod <= 1 )
   {
      /* No smoothing: emit the raw True Range (unnormalized). */
      outReal[0] = prevATR;
   }
   else
   {
      tempValue = inClose[startIdx];
      if( !TA_IS_ZERO(tempValue) )
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

      prevATR *= optInTimePeriod - 1;
      prevATR += greatest;
      prevATR /= optInTimePeriod;
      if( optInTimePeriod <= 1 )
      {
         /* No smoothing: emit the raw True Range (unnormalized). */
         outReal[outIdx] = prevATR;
      }
      else
      {
         tempValue = inClose[today];
         if( !TA_IS_ZERO(tempValue) )
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
