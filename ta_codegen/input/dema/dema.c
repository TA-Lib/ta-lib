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
 *  010102 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: compute both EMA in a single
 *                lockstep pass (bit-exact, no temporary buffers).
 *
 */

int dema_lookback(int optInTimePeriod)
{
   /* Get lookback for one EMA.
    * Multiply by two (because double smoothing).
    */
   return ema_lookback( optInTimePeriod ) * 2;
}

TA_RetCode dema(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double prevEMA1, prevEMA2, tempReal, optInK_1;
   int i, today, outIdx, lookbackEMA, lookbackTotal;

   /* For an explanation of this function, please read
    *
    * Stocks & Commodities V. 12:1 (11-19):
    *   Smoothing Data With Faster Moving Averages
    * Stocks & Commodities V. 12:2 (72-80):
    *   Smoothing Data With Less Lag
    *
    * Both magazine articles written by Patrick G. Mulloy
    *
    * Essentially, a DEMA of time serie 't' is:
    *   EMA2 = EMA(EMA(t,period),period)
    *   DEMA = 2*EMA(t,period)- EMA2
    *
    * DEMA offers a moving average with less lags then the
    * traditional EMA.
    *
    * Do not confuse a DEMA with the EMA2. Both are called
    * "Double EMA" in the litterature, but EMA2 is a simple
    * EMA of an EMA, while DEMA is a compostie of a single
    * EMA with EMA2.
    *
    * TEMA is very similar (and from the same author).
    */

   /* Will change only on success. */
   *outNBElement = 0;
   *outBegIdx = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackEMA = ema_lookback( optInTimePeriod );
   lookbackTotal = lookbackEMA * 2;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* Both EMA are computed in a single lockstep pass: each new
    * EMA1 value is immediately fed into EMA2. No temporary
    * buffers are needed.
    *
    * The arithmetic order below is the bit-exactness contract
    * (do not reorder or fuse operations):
    *  - EMA recursion: ((x-prev)*k)+prev.
    *  - Default compatibility: each EMA is seeded with the sum
    *    of its first 'period' inputs, accumulated from 0.0 in
    *    input order (0.0+x is not x for x=-0.0), divided by
    *    the period.
    *  - Metastock compatibility: EMA1 is seeded from inReal[0],
    *    EMA2 from the first EMA1 value.
    * Output alignment is identical for all compatibility modes;
    * only the seed values differ.
    *
    * In-place (inReal == outReal) is supported: outReal[outIdx]
    * is written only after inReal[startIdx+outIdx] was read.
    */
   optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));

   if( TA_GetCompatibility() == TA_COMPATIBILITY_DEFAULT )
   {
      /* Seed EMA1 with a simple average of the first
       * 'period' price bars.
       */
      today = startIdx-lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 )
         tempReal += inReal[today++];
      prevEMA1 = tempReal / optInTimePeriod;

      /* Advance EMA1 alone through its unstable period, up to
       * the bar where EMA2 seeding begins.
       */
      while( today <= startIdx-lookbackEMA )
         prevEMA1 = ((inReal[today++]-prevEMA1)*optInK_1) + prevEMA1;

      /* Seed EMA2 with a simple average of the first 'period'
       * EMA1 values, accumulated as EMA1 produces them.
       */
      tempReal = 0.0;
      tempReal += prevEMA1;
      i = optInTimePeriod-1;
      while( i-- > 0 )
      {
         prevEMA1 = ((inReal[today++]-prevEMA1)*optInK_1) + prevEMA1;
         tempReal += prevEMA1;
      }
      prevEMA2 = tempReal / optInTimePeriod;
   }
   else
   {
      /* Metastock/Tradestation: seed each EMA with its first
       * input value: EMA1 from inReal[0], EMA2 from the first
       * EMA1 value.
       */
      prevEMA1 = inReal[0];
      today = 1;
      while( today <= startIdx-lookbackEMA )
         prevEMA1 = ((inReal[today++]-prevEMA1)*optInK_1) + prevEMA1;
      prevEMA2 = prevEMA1;
   }

   /* Advance both EMA in lockstep through the unstable period
    * of EMA2, up to the first output bar.
    */
   while( today <= startIdx )
   {
      prevEMA1 = ((inReal[today++]-prevEMA1)*optInK_1) + prevEMA1;
      prevEMA2 = ((prevEMA1-prevEMA2)*optInK_1) + prevEMA2;
   }

   /* Stable zone: keep advancing both EMA in lockstep and
    * write the DEMA into the output.
    */
   outReal[0] = (2.0*prevEMA1) - prevEMA2;
   outIdx = 1;
   while( today <= endIdx )
   {
      prevEMA1 = ((inReal[today++]-prevEMA1)*optInK_1) + prevEMA1;
      prevEMA2 = ((prevEMA1-prevEMA2)*optInK_1) + prevEMA2;
      outReal[outIdx++] = (2.0*prevEMA1) - prevEMA2;
   }

   /* Succeed. Indicate where the output starts relative to
    * the caller input.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
