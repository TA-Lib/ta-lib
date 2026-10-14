/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100726 KL,CC  Initial version (#484).
 *  101426 MF,CC  The EMA step is TA_EMA's; the trend and the range are selects.
 *
 */

int kvo_lookback(int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   int longestPeriod;

   /* The three EMAs seed on one bar, so one EMA lookback of the longest
    * covers all three. The 1 is the reference bar before it.
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
   double kFast, kSlow, kSignal, betaFast, betaSlow, betaSignal;
   double hlc, prevHlc, dmToday, prevDm, cm, cmBase, vf;
   double emaFast, emaSlow, kvoValue, signalValue, trend, prevTrend;
   int lookbackTotal;
   int today, outIdx;

   /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
    * Analysis of Stocks & Commodities V15:12 (December 1997), with the 1997
    * factor |2*(dm/cm) - 1| (#484, form A; LEAN's differs).
    *
    *    trend = +1 if H+L+C rose, -1 if it fell, unchanged on a tie
    *    cm    = previous + current range on a trend change, else cm + range
    *    vf    = volume * |2*(dm/cm) - 1| * 100 * trend
    *
    * The committed corpus holds a single tie in H+L+C, and reading it any other
    * way moves KVO by up to 1e7 from the next bar on: the tie rule is gated by
    * a built series in test_kvo.c, not by the corpus.
    *
    * All three EMAs seed raw on the first volume force, not on an SMA as
    * TA_EMA does. Each k + beta pair must sum to exactly 1.0, as in ema.c.
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

   betaFast = ((double)(optInFastPeriod - 1)) / ((double)(optInFastPeriod + 1));
   kFast = 1.0 - betaFast;
   if( betaFast < 0.5 ) betaFast = 1.0 - kFast;

   betaSlow = ((double)(optInSlowPeriod - 1)) / ((double)(optInSlowPeriod + 1));
   kSlow = 1.0 - betaSlow;
   if( betaSlow < 0.5 ) betaSlow = 1.0 - kSlow;

   betaSignal = ((double)(optInSignalPeriod - 1)) / ((double)(optInSignalPeriod + 1));
   kSignal = 1.0 - betaSignal;
   if( betaSignal < 0.5 ) betaSignal = 1.0 - kSignal;

   /* The anchor: only its H+L+C and range are read, and the trend starts at +1. */
   today = startIdx - lookbackTotal;
   prevHlc = inHigh[today] + inLow[today] + inClose[today];
   prevDm  = inHigh[today] - inLow[today];
   prevTrend = 1.0;
   today = today + 1;

   /* The seed bar: cm opens on both ranges whatever the trend, and both
    * averages start at this volume force, so KVO and the trigger are exactly 0.
    */
   hlc = inHigh[today] + inLow[today] + inClose[today];
   dmToday = inHigh[today] - inLow[today];
   trend = ( hlc > prevHlc ) ? 1.0 : prevTrend;
   trend = ( hlc < prevHlc ) ? -1.0 : trend;
   cm = prevDm + dmToday;
   if( cm == 0.0 )
      vf = 0.0;
   else
      vf = inVolume[today] * fabs( 2.0 * (dmToday / cm) - 1.0 ) * 100.0 * trend;
   emaFast = vf;
   emaSlow = vf;
   kvoValue = emaFast - emaSlow;
   signalValue = kvoValue;
   prevHlc = hlc;
   prevDm = dmToday;
   prevTrend = trend;
   today = today + 1;

   while( today < startIdx )
   {
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      trend = ( hlc > prevHlc ) ? 1.0 : prevTrend;
      trend = ( hlc < prevHlc ) ? -1.0 : trend;
      cmBase = ( trend != prevTrend ) ? prevDm : cm;
      cm = cmBase + dmToday;

      if( cm == 0.0 )
         vf = 0.0;
      else
         vf = inVolume[today] * fabs( 2.0 * (dmToday / cm) - 1.0 ) * 100.0 * trend;

      emaFast = (kFast * vf) + (betaFast * emaFast);
      emaSlow = (kSlow * vf) + (betaSlow * emaSlow);
      kvoValue = emaFast - emaSlow;
      signalValue = (kSignal * kvoValue) + (betaSignal * signalValue);

      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
   }

   outIdx = 0;
   while( today <= endIdx )
   {
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      trend = ( hlc > prevHlc ) ? 1.0 : prevTrend;
      trend = ( hlc < prevHlc ) ? -1.0 : trend;
      cmBase = ( trend != prevTrend ) ? prevDm : cm;
      cm = cmBase + dmToday;

      if( cm == 0.0 )
         vf = 0.0;
      else
         vf = inVolume[today] * fabs( 2.0 * (dmToday / cm) - 1.0 ) * 100.0 * trend;

      emaFast = (kFast * vf) + (betaFast * emaFast);
      emaSlow = (kSlow * vf) + (betaSlow * emaSlow);
      kvoValue = emaFast - emaSlow;
      signalValue = (kSignal * kvoValue) + (betaSignal * signalValue);

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
