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
 *  071626 MF,CC  Initial version (#124).
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars, so the
 *                0/0 is guarded exactly instead of against the fixed
 *                TA_IS_ZERO band -- which zeroed the oscillator for any
 *                instrument quoted small enough to fall under it.
 *  102326 MF,CC  #554 branch-free up/down split and zero run, rare reset.
 */

int cmou_lookback(int optInTimePeriod)
{
   /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
    * the first output is at index optInTimePeriod.
    *
    * Unlike the shipped CMO, there is NO unstable period: CMOU is a plain
    * moving-window sum, so its lookback is exactly the period.
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
   int nullRun, live;
   double upSum, downSum, sum, diff, up, tempReal, prevValue, trailingValue;

   /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
    * pandas-ta default). Over the trailing optInTimePeriod changes
    * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
    * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
    * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
    * smoothing -- hence no unstable period.
    *
    * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
    * is of a slot not yet written (trailingIdx >= outIdx), and the oldest
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
   /* Consecutive changes of exactly zero, counted so that an empty window can
    * be recognized exactly (the shape #244 needed for MFI). The sums cannot
    * answer that question themselves once the window starts sliding: they are
    * maintained by add-then-subtract, so an emptied window leaves them holding
    * rounding residue of arbitrary sign rather than zero.
    *
    * Both the split and the run are branch-free, as an if/else on the sign or
    * on a zero mispredicts on random or flat-heavy prices. `up` is max(diff, 0)
    * spelled with fabs, which no backend turns back into a branch; adding its
    * +0.0 is exact because neither sum is ever -0.0.
    */
   nullRun = 0;
   for( i = 0; i < optInTimePeriod; i++ )
   {
      today++;
      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      up = (diff + fabs(diff)) * 0.5;
      upSum += up;
      downSum += up - diff;
      live = fabs(diff) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
   }

   /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
    * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
    * exactly and emit 0.0. Not against a fixed band: a price change carries the
    * quote unit, so a constant put against the total zeroed the oscillator for
    * any instrument quoted below it (issue #253).
    *
    * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
    * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
    * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
    * which both scale before dividing. The two orders differ by <=1 ULP. */
   outIdx = 0;
   sum = upSum + downSum;
   if( sum > 0.0 )
      outReal[outIdx++] = (100.0 * (upSum - downSum)) / sum;
   else
      outReal[outIdx++] = 0.0;

   /* Slide the window forward one bar at a time. Each bar drops the change
    * that leaves the window AFTER its output, ready for the next bar: the
    * sums see the same operations in the same order, and a peek, which keeps
    * nothing past its output, skips that half of the work. The oldest
    * change's older price comes from the cache.
    */
   tempReal = inReal[trailingIdx];
   diff = tempReal - trailingValue;
   trailingValue = tempReal;
   trailingIdx++;
   up = (diff + fabs(diff)) * 0.5;
   upSum -= up;
   downSum -= up - diff;

   today++;
   while( today <= endIdx )
   {
      /* Add the newest change: inReal[today] - inReal[today-1]. */
      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      up = (diff + fabs(diff)) * 0.5;
      upSum += up;
      downSum += up - diff;

      /* Once a whole period of flat bars has gone by, every change in the
       * window is exactly zero, so both sums are known to be exactly zero and
       * the residue can be dropped.
       */
      live = fabs(diff) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
      if( TA_UNLIKELY(nullRun >= optInTimePeriod) )
      {
         nullRun = optInTimePeriod;
         upSum = 0.0;
         downSum = 0.0;
         outReal[outIdx++] = 0.0;
      }
      else
      {
         sum = upSum + downSum;
         if( sum > 0.0 )
            outReal[outIdx++] = (100.0 * (upSum - downSum)) / sum;
         else
            outReal[outIdx++] = 0.0;
      }

      tempReal = inReal[trailingIdx];
      diff = tempReal - trailingValue;
      trailingValue = tempReal;
      trailingIdx++;
      up = (diff + fabs(diff)) * 0.5;
      upSum -= up;
      downSum -= up - diff;

      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
