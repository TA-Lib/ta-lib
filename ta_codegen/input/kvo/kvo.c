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
 *  100726 MF,CC  Initial version (#484).
 *
 */

int kvo_lookback(int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   int longestPeriod;

   /* The three smoothings share ONE anchor -- they are all seeded from the
    * volume force of the same bar -- so the warm-up is one EMA lookback of the
    * longest of them, not the sum of three. The extra 1 is the reference bar:
    * the trend needs a previous H+L+C to compare against, and the cumulative
    * range needs a previous bar's range to open with.
    */
   longestPeriod = optInFastPeriod;
   if( optInSlowPeriod > longestPeriod )
      longestPeriod = optInSlowPeriod;
   if( optInSignalPeriod > longestPeriod )
      longestPeriod = optInSignalPeriod;

   return 1 + ema_lookback( longestPeriod )
   + TA_UNSTABLE_AUTO( TA_FUNC_UNST_EMA,
      ta_auto_stabilization_kvo( K,
      max( optInFastPeriod, max( optInSlowPeriod, optInSignalPeriod ) ) ) );
}

TA_RetCode kvo(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outKVO[],
   double outKVOSignal[])
{
   double kFast, kSlow, kSignal;
   double hlc, prevHlc, dmToday, prevDm, cm, vf, factor;
   double emaFast, emaSlow, kvoValue, signalValue;
   int lookbackTotal, trend, prevTrend;
   int today, outIdx;

   /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
    * Analysis of Stocks & Commodities V15:12 (December 1997).
    *
    * Each bar's volume is signed by the trend of H+L+C and weighted by where
    * the bar's own range sits inside the cumulative range of the current trend
    * run. KVO is the difference of two exponential averages of that volume
    * force; the trigger is a third exponential average of KVO.
    *
    *    trend  = +1 if H+L+C rose, -1 if it fell, UNCHANGED if it repeated
    *    cm     = the previous and current ranges on a trend change,
    *             otherwise the running sum plus this bar's range
    *    vf     = volume * |2*(dm/cm) - 1| * 100 * trend
    *
    * The tie rule is the article's own tenet -- "When equality occurs, the
    * existing trend is maintained" -- and it is one line that a corpus with
    * few ties barely exercises. MEASURED on the committed 252-bar corpus:
    * exactly one tie (bar 187), and reading it as +1, or as a reversal, moves
    * KVO by up to 1.0e7 from the next bar on. So it is cheap to get wrong and
    * not cheap to notice.
    *
    * The factor is the 1997 form |2*(dm/cm) - 1| (#484, form A), which is what
    * the December 1997 Traders' Tips all print. It is not LEAN's.
    *
    * The three averages are seeded RAW, from the volume force of the anchor
    * bar, not from a simple average of their first `period` inputs the way
    * ema.c seeds a standalone TA_EMA. That is what every 1997 transcription
    * does, and it is why one lookback covers all three.
    */

   lookbackTotal = kvo_lookback( optInFastPeriod, optInSlowPeriod,
      optInSignalPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   kFast   = 2.0 / ((double)optInFastPeriod + 1.0);
   kSlow   = 2.0 / ((double)optInSlowPeriod + 1.0);
   kSignal = 2.0 / ((double)optInSignalPeriod + 1.0);

   /* The anchor. Its H+L+C and its range are read, nothing else: the trend
    * starts at +1 there by convention and the first volume force is the bar
    * after it.
    */
   today = startIdx - lookbackTotal;
   prevHlc = inHigh[today] + inLow[today] + inClose[today];
   prevDm  = inHigh[today] - inLow[today];
   prevTrend = 1;
   cm = 0.0;
   emaFast = 0.0;
   emaSlow = 0.0;
   signalValue = 0.0;
   kvoValue = 0.0;
   today = today + 1;

   /* The seed bar. Both averages start at this bar's volume force, so KVO is
    * exactly zero here and the trigger seeds on that exact zero.
    */
   hlc = inHigh[today] + inLow[today] + inClose[today];
   dmToday = inHigh[today] - inLow[today];
   if( hlc > prevHlc )
      trend = 1;
   else if( hlc < prevHlc )
      trend = -1;
   else
      trend = prevTrend;
   cm = prevDm + dmToday;
   if( cm == 0.0 )
      vf = 0.0;
   else
   {
      factor = 2.0 * (dmToday / cm) - 1.0;
      if( factor < 0.0 )
         factor = -factor;
      vf = inVolume[today] * factor * 100.0 * (double)trend;
   }
   emaFast = vf;
   emaSlow = vf;
   kvoValue = emaFast - emaSlow;
   signalValue = kvoValue;
   prevHlc = hlc;
   prevDm = dmToday;
   prevTrend = trend;
   today = today + 1;

   /* Warm-up. Every bar from here is a pure recursion; only the cumulative
    * range branches, and it branches on the data, not on a counter.
    */
   while( today < startIdx )
   {
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      if( hlc > prevHlc )
         trend = 1;
      else if( hlc < prevHlc )
         trend = -1;
      else
         trend = prevTrend;

      if( trend != prevTrend )
         cm = prevDm + dmToday;
      else
         cm = cm + dmToday;

      if( cm == 0.0 )
         vf = 0.0;
      else
      {
         factor = 2.0 * (dmToday / cm) - 1.0;
         if( factor < 0.0 )
            factor = -factor;
         vf = inVolume[today] * factor * 100.0 * (double)trend;
      }

      emaFast = ((vf - emaFast) * kFast) + emaFast;
      emaSlow = ((vf - emaSlow) * kSlow) + emaSlow;
      kvoValue = emaFast - emaSlow;
      signalValue = ((kvoValue - signalValue) * kSignal) + signalValue;

      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
   }

   /* The requested range. */
   outIdx = 0;
   while( today <= endIdx )
   {
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      if( hlc > prevHlc )
         trend = 1;
      else if( hlc < prevHlc )
         trend = -1;
      else
         trend = prevTrend;

      if( trend != prevTrend )
         cm = prevDm + dmToday;
      else
         cm = cm + dmToday;

      if( cm == 0.0 )
         vf = 0.0;
      else
      {
         factor = 2.0 * (dmToday / cm) - 1.0;
         if( factor < 0.0 )
            factor = -factor;
         vf = inVolume[today] * factor * 100.0 * (double)trend;
      }

      emaFast = ((vf - emaFast) * kFast) + emaFast;
      emaSlow = ((vf - emaSlow) * kSlow) + emaSlow;
      kvoValue = emaFast - emaSlow;
      signalValue = ((kvoValue - signalValue) * kSignal) + signalValue;

      outKVO[outIdx] = kvoValue;
      outKVOSignal[outIdx] = signalValue;
      outIdx = outIdx + 1;

      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
