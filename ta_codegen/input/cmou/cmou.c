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
 *  071626 MF,CC  Initial version.
 */

int cmou_lookback(int optInTimePeriod)
{
   /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
    * the first output is at index optInTimePeriod.
    *
    * Unlike the shipped CMO, there is NO unstable period and NO Metastock
    * "extra initial bar" adjustment: CMOU is a plain moving-window sum, so its
    * lookback is exactly the period.
    */
   return optInTimePeriod;
}

TA_RetCode cmou(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;
   int today, trailingIdx, lookbackTotal, i;
   double upSum, downSum, sum, diff, tempReal, prevValue, trailingValue;

   /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
    * QuantConnect, pandas-ta default). Over the trailing optInTimePeriod changes
    * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
    * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
    * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
    * smoothing -- hence no unstable period.
    *
    * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
    * precedes this iteration's write (trailingIdx >= outIdx), and the oldest
    * change's older endpoint comes from the `trailingValue` cache, not a re-read.
    */

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = cmou_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* Accumulate the up/down sums over the first window: the optInTimePeriod
    * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
    * startIdx]). `trailingValue` caches the oldest price so the window's oldest
    * change can later be dropped by reading only the newer of its two prices.
    * `trailingIdx` points AT that newer price (one past the cached one).
    */
   today = startIdx - lookbackTotal;
   trailingIdx = today + 1;
   prevValue = inReal[today];
   trailingValue = prevValue;

   upSum = 0.0;
   downSum = 0.0;
   for( i = 0; i < optInTimePeriod; i++ )
   {
      today++;
      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 )
         upSum += diff;
      else if( diff < 0.0 )
         downSum -= diff;
   }

   /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
    * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
    * with TA_IS_ZERO (as TA_CMO does for its own gain+loss) and emit 0.0.
    *
    * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
    * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
    * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
    * which both scale before dividing. The two orders differ by <=1 ULP. */
   outIdx = 0;
   sum = upSum + downSum;
   if( !TA_IS_ZERO(sum) )
      outReal[outIdx++] = (100.0 * (upSum - downSum)) / sum;
   else
      outReal[outIdx++] = 0.0;

   /* Slide the window forward one bar at a time. */
   today++;
   while( today <= endIdx )
   {
      /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
       * inReal[trailingIdx-1] comes from the cache (already overwritten when
       * outReal == inReal); inReal[trailingIdx] is read here, before this
       * iteration writes outReal[outIdx], so it is still the original price. */
      tempReal = inReal[trailingIdx];
      diff = tempReal - trailingValue;
      trailingValue = tempReal;
      trailingIdx++;
      if( diff > 0.0 )
         upSum -= diff;
      else if( diff < 0.0 )
         downSum += diff;

      /* Add the newest change: inReal[today] - inReal[today-1]. */
      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 )
         upSum += diff;
      else if( diff < 0.0 )
         downSum -= diff;

      sum = upSum + downSum;
      if( !TA_IS_ZERO(sum) )
         outReal[outIdx++] = (100.0 * (upSum - downSum)) / sum;
      else
         outReal[outIdx++] = 0.0;

      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
