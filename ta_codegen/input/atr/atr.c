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

TA_RetCode atr(int startIdx, int endIdx, const double inHigh[], const double inLow[], const double inClose[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   int nbATR;

   double prevATR, periodTotal;
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

   /* Trap the case where no smoothing is needed. */
   if( optInTimePeriod <= 1 )
   {
      /* No smoothing needed. Just do a TRANGE. */
      return trange( startIdx, endIdx,
         inHigh, inLow, inClose,
         outBegIdx, outNBElement, outReal );
   }

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

      prevATR *= optInTimePeriod - 1;
      prevATR += greatest;
      prevATR /= optInTimePeriod;
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

      prevATR *= optInTimePeriod - 1;
      prevATR += greatest;
      prevATR /= optInTimePeriod;
      outReal[outIdx++] = prevATR;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
