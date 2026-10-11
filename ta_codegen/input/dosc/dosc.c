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
 *  100626 KL,CC  Initial version (#479).
 *  101026 MF,CC  The EMA betas stay as the divide wrote them above a
 *                period of 2.
 *
 */

int dosc_lookback(int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod)
{
   /* Keep every term a callee's lookback call: that is how DOSC inherits
    * TA_FUNC_UNST_RSI and TA_FUNC_UNST_EMA with no id of its own. The EMA
    * term counts twice, once per smoothing stage.
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
    * The output must stay bit-identical to
    * TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB, so every stage keeps its
    * callee's operation order, and every stage boundary is the callee's
    * LOOKBACK, not (period-1): a warm unstable period then folds in.
    *
    * Compare the counters BEFORE subtracting them: the Rust backend renders
    * them usize, where `n = nRsi - skipRSI; if( n >= 0 )` underflows.
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

   /* The RSI values the composed chain never publishes. Derive the count
    * from the lookback, never from TA_GetUnstablePeriod: under the Auto
    * levels it is a function of the period.
    */
   skipRSI = lookbackRSI - optInTimePeriod;

   /* k and beta must sum to exactly 1.0, or a flat input drifts off its
    * level. Each subtraction is exact only from an operand in [0.5,1): at a
    * period of 2 that is k, above it beta. Above it beta stays as the divide
    * wrote it: a register last written by a subtraction costs each FMA
    * reading it one more cycle on Intel P-cores.
    */
   beta1 = ((double)(optInFirstPeriod - 1)) / ((double)(optInFirstPeriod + 1));
   k1    = 1.0 - beta1;
   if( beta1 < 0.5 ) beta1 = 1.0 - k1;
   beta2 = ((double)(optInSecondPeriod - 1)) / ((double)(optInSecondPeriod + 1));
   k2    = 1.0 - beta2;
   if( beta2 < 0.5 ) beta2 = 1.0 - k2;

   ema1 = 0.0;
   ema2 = 0.0;
   sum1 = 0.0;
   sum2 = 0.0;
   sumSignal = 0.0;
   nRsi = 0;

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

   /* The first output. Every stage is past its seed here, even at periods
    * 2/2/2/2, so the store below and the one in the stable loop are
    * unconditional. Keep them so: under a guard, the managed peek frames
    * seed a dead output local.
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
