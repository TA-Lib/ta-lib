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
 *  100626 MF,CC  Initial version (#479).
 *
 */

int dosc_lookback(int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod)
{
   /* Wilder's RSI, the two exponential smoothings stacked on it and the
    * simple average taken over the result. Every term is exactly the lookback
    * of the function it comes from, so none of them is restated here -- which
    * is what makes DOSC inherit TA_FUNC_UNST_RSI and TA_FUNC_UNST_EMA from its
    * callees rather than take an id of its own, and what carries the Auto
    * warm-up levels of #492 through all three of them.
    *
    * The EMA term appears TWICE, once per smoothing stage, so a warm
    * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, k) moves the lookback by 2k and
    * not by k.
    */
   return rsi_lookback( optInTimePeriod )
   + ema_lookback( optInFirstPeriod )
   + ema_lookback( optInSecondPeriod )
   + sma_lookback( optInSignalPeriod );
}

TA_RetCode dosc(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInFirstPeriod,
   int optInSecondPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double k1, beta1, k2, beta2;
   double invPeriod, prevGain, prevLoss, prevValue;
   double gainDelta, tempValue1, tempValue2;
   double rsiValue, ema1, ema2, sum1, sum2, sumSignal;
   int lookbackTotal, lookbackRSI, lookbackEMA1, lookbackEMA2, skipRSI;
   int today, i, outIdx, rsiBar;
   int nRsi, n1, n2;

   /* Constance Brown's "triple smoothed derivative of RSI plotted as a
    * histogram" (MTA Journal, 1994): MACD's histogram construction applied to
    * a double-smoothed RSI.
    *
    *    S1_t   = EMA(RSI(x, t), f)_t
    *    DS_t   = EMA(S1, s)_t
    *    DOSC_t = DS_t - SMA(DS, g)_t
    *
    * This walks the chain in one pass, with each stage's arithmetic spelled
    * exactly as its callee spells it: rsi.c's Wilder recursion, ema.c's seed
    * and step, and sma.c's add-new / snapshot / subtract-old running sum. The
    * intermediate series are never materialised -- the double-smoothed line
    * goes straight into a ring of the last `signal` values -- and the result
    * is bit-identical to TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB
    * rather than merely close, which is what the composition gate holds.
    *
    * Every stage boundary below is the callee's LOOKBACK, not (period-1), so
    * each stage seeds on the values its predecessor would have published and a
    * warm unstable period folds in. The counters are compared BEFORE they are
    * subtracted, never after: written as `n = nRsi - skipRSI; if( n >= 0 )`
    * this is correct in C, where the counters are signed, and broken in the
    * Rust backend, which renders them usize (the lesson smi.c records).
    */

   /* This ptr will point on a circular buffer of at least
    * "optInSignalPeriod" element.
    */
   CIRCBUF_PROLOG(dsBuffer,double,32);

   lookbackTotal = dosc_lookback( optInTimePeriod, optInFirstPeriod,
      optInSecondPeriod, optInSignalPeriod );

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

   CIRCBUF_INIT( dsBuffer, double, optInSignalPeriod );

   lookbackRSI  = rsi_lookback( optInTimePeriod );
   lookbackEMA1 = ema_lookback( optInFirstPeriod );
   lookbackEMA2 = ema_lookback( optInSecondPeriod );

   /* The RSI values the composed chain never publishes: TA_RSI entered at the
    * first bar the first smoothing needs has already consumed its own
    * unstable period by then. Taking the difference of the two lookbacks
    * rather than reading TA_GetUnstablePeriod is what keeps this correct under
    * the Auto levels, where the count is a function of the period.
    */
   skipRSI = lookbackRSI - optInTimePeriod;

   /* ema.c's constants: k and beta must sum to exactly 1.0, or a flat input
    * drifts off its level.
    */
   beta1 = ((double)(optInFirstPeriod - 1)) / ((double)(optInFirstPeriod + 1));
   k1    = 1.0 - beta1;
   beta1 = 1.0 - k1;
   beta2 = ((double)(optInSecondPeriod - 1)) / ((double)(optInSecondPeriod + 1));
   k2    = 1.0 - beta2;
   beta2 = 1.0 - k2;

   ema1 = 0.0;
   ema2 = 0.0;
   sum1 = 0.0;
   sum2 = 0.0;
   sumSignal = 0.0;
   nRsi = 0;

   /* Wilder's seed, exactly as rsi.c accumulates it: one simple sum of the
    * first optInTimePeriod changes, each side taken unconditionally, then
    * both scaled by 1/period.
    */
   invPeriod = 1.0 / (double)optInTimePeriod;
   today = startIdx - lookbackTotal;
   rsiBar = today + optInTimePeriod;
   prevValue = inReal[today];
   prevGain = 0.0;
   prevLoss = 0.0;
   today = today + 1;
   for( i = optInTimePeriod; i > 0; i-- )
   {
      tempValue1 = inReal[today];
      today = today + 1;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
      prevGain += gainDelta;
      prevLoss += gainDelta - tempValue2;
   }
   prevLoss *= invPeriod;
   prevGain *= invPeriod;

   /* rsi.c answers the neutral 50 when neither a gain nor a loss has been seen
    * since the seed, the 0/0 case of issue #480; the one-sided cases are 0 and
    * 100 and reach the division.
    */
   tempValue1 = prevGain+prevLoss;
   if( tempValue1 > 0.0 )
      rsiValue = 100.0*(prevGain/tempValue1);
   else
      rsiValue = 50.0;

   /* Warm-up. Feeds every RSI value before startIdx through the chain and
    * leaves rsiValue holding startIdx's own. Nothing is emitted here: the
    * first bar whose signal window is full is startIdx, by construction of
    * the lookback.
    */
   while( rsiBar < startIdx )
   {
      if( nRsi >= skipRSI )
      {
         n1 = nRsi - skipRSI;
         if( n1 < optInFirstPeriod )
         {
            sum1 = sum1 + rsiValue;
            if( n1 == optInFirstPeriod - 1 )
               ema1 = sum1 / optInFirstPeriod;
         }
         else
            ema1 = k1 * rsiValue + beta1 * ema1;

         if( n1 >= lookbackEMA1 )
         {
            n2 = n1 - lookbackEMA1;
            if( n2 < optInSecondPeriod )
            {
               sum2 = sum2 + ema1;
               if( n2 == optInSecondPeriod - 1 )
                  ema2 = sum2 / optInSecondPeriod;
            }
            else
               ema2 = k2 * ema1 + beta2 * ema2;

            if( n2 >= lookbackEMA2 )
            {
               dsBuffer[dsBuffer_Idx] = ema2;
               sumSignal = sumSignal + ema2;
               CIRCBUF_NEXT(dsBuffer);
            }
         }
      }
      nRsi = nRsi + 1;

      tempValue1 = inReal[today];
      today = today + 1;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      prevLoss *= (double)(optInTimePeriod-1);
      prevGain *= (double)(optInTimePeriod-1);
      gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
      prevGain += gainDelta;
      prevLoss += gainDelta - tempValue2;
      prevLoss *= invPeriod;
      prevGain *= invPeriod;
      tempValue1 = prevGain+prevLoss;
      if( tempValue1 > 0.0 )
         rsiValue = 100.0*(prevGain/tempValue1);
      else
         rsiValue = 50.0;
      rsiBar = rsiBar + 1;
   }

   /* The first output. Every stage is past its seed here -- the shortest
    * reachable case, periods 2/2/2/2, still arrives with n1 = 3, n2 = 2 and a
    * signal window one short of full -- so from this bar on the chain is three
    * pure recursions and a running sum, with nothing left to branch on. That
    * is what keeps the managed peek frames from carrying a seeded output
    * local: the store below and the one in the stable loop always run.
    */
   ema1 = k1 * rsiValue + beta1 * ema1;
   ema2 = k2 * ema1 + beta2 * ema2;
   dsBuffer[dsBuffer_Idx] = ema2;
   sumSignal = sumSignal + ema2;
   outReal[0] = ema2 - sumSignal / (double)optInSignalPeriod;
   outIdx = 1;
   CIRCBUF_NEXT(dsBuffer);
   sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];

   /* Stable zone. */
   while( today <= endIdx )
   {
      tempValue1 = inReal[today];
      today = today + 1;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;

      prevLoss *= (double)(optInTimePeriod-1);
      prevGain *= (double)(optInTimePeriod-1);
      gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
      prevGain += gainDelta;
      prevLoss += gainDelta - tempValue2;
      prevLoss *= invPeriod;
      prevGain *= invPeriod;

      tempValue1 = prevGain+prevLoss;
      if( tempValue1 > 0.0 )
         rsiValue = 100.0*(prevGain/tempValue1);
      else
         rsiValue = 50.0;

      ema1 = k1 * rsiValue + beta1 * ema1;
      ema2 = k2 * ema1 + beta2 * ema2;

      dsBuffer[dsBuffer_Idx] = ema2;
      sumSignal = sumSignal + ema2;
      outReal[outIdx] = ema2 - sumSignal / (double)optInSignalPeriod;
      outIdx = outIdx + 1;
      CIRCBUF_NEXT(dsBuffer);
      sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
   }

   CIRCBUF_DESTROY(dsBuffer);

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
