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
 *  100626 KL,CC  Initial version (#476).
 *  101126 MF,CC  The EMA betas stay as the divide wrote them above a
 *                period of 2. The fixpoint test waits for a seeded
 *                average.
 *
 */

int wavetrend_lookback(int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod)
{
   /* Keep every term a callee's own lookback: that is how WAVETREND inherits
    * TA_FUNC_UNST_EMA, once per exponential stage.
    */
   return ema_lookback( optInChannelPeriod )
   + ema_lookback( optInChannelPeriod )
   + ema_lookback( optInAveragePeriod )
   + sma_lookback( optInSignalPeriod );
}

TA_RetCode wavetrend(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInChannelPeriod,
   int optInAveragePeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outWT1[],
   double outWT2[])
{
   double k1, beta1, k2, beta2;
   double ap, esa, dev, d, ci, wt1;
   double prevAp, prevEsa, num, scaledDev;
   double sumEsa, sumD, sumCi, sumSignal;
   int lookbackTotal, lookbackChannel, lookbackAverage;
   int today, outIdx;
   int nAp, nDev, nCi;

   /* LazyBear's WaveTrend Oscillator (TradingView, 2014), lines 14 to 21: the
    * typical price's distance from its own exponential average, normalised by
    * an exponential average of that distance's absolute value and by Lambert's
    * CCI constant, then smoothed.
    *
    *    ap  = (H + L + C)/3
    *    esa = EMA(ap, n1)
    *    d   = EMA(|ap - esa|, n1)
    *    ci  = (ap - esa) / (0.015 * d)
    *    WT1 = EMA(ci, n2)        WT2 = SMA(WT1, n3)
    *
    * The middle stage is an exponential CCI, not TA_CCI: cci.c averages with
    * an SMA and takes the mean deviation around that window's own SMA, where
    * this uses two exponential averages.
    *
    * Bit-identical to the composed chain on moving data: each stage seeds as
    * ema.c and sma.c do, and each stage boundary is the callee's LOOKBACK,
    * never (period-1), or an EMA unstable period breaks it.
    */

   /* This ptr will point on a circular buffer of at least
    * "optInSignalPeriod" element.
    */
   CIRCBUF_PROLOG(wtBuffer,double,32);

   lookbackTotal = wavetrend_lookback( optInChannelPeriod, optInAveragePeriod,
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

   CIRCBUF_INIT( wtBuffer, double, optInSignalPeriod );

   lookbackChannel = ema_lookback( optInChannelPeriod );
   lookbackAverage = ema_lookback( optInAveragePeriod );

   /* k and beta must sum to exactly 1.0, or a flat input drifts off its
    * level. Each subtraction is exact only from an operand in [0.5,1): at a
    * period of 2 or less that is k, above it beta. Above it beta stays as the
    * divide wrote it: a register last written by a subtraction costs each FMA
    * reading it one more cycle on Intel P-cores.
    */
   beta1 = ((double)(optInChannelPeriod - 1)) / ((double)(optInChannelPeriod + 1));
   k1    = 1.0 - beta1;
   if( beta1 < 0.5 ) beta1 = 1.0 - k1;
   beta2 = ((double)(optInAveragePeriod - 1)) / ((double)(optInAveragePeriod + 1));
   k2    = 1.0 - beta2;
   if( beta2 < 0.5 ) beta2 = 1.0 - k2;

   esa = 0.0;
   d = 0.0;
   ci = 0.0;
   wt1 = 0.0;
   sumEsa = 0.0;
   sumD = 0.0;
   sumCi = 0.0;
   sumSignal = 0.0;
   nAp = 0;

   prevAp = 0.0;
   prevEsa = 0.0;

   today = startIdx - lookbackTotal;

   /* Warm-up. Runs through startIdx inclusive: the last pass is the one that
    * completes the signal window, so it leaves the first output in the state.
    */
   while( today <= startIdx )
   {
      ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;

      /* Stage 1: the exponential average of the typical price. */
      if( nAp < optInChannelPeriod )
      {
         sumEsa = sumEsa + ap;
         if( nAp == optInChannelPeriod - 1 )
            esa = sumEsa / optInChannelPeriod;
      }
      else
         esa = k1 * ap + beta1 * esa;

      /* Stage 2: the exponential average of the absolute distance, over what
       * stage 1 publishes. The counter is compared before it is subtracted,
       * never after: the Rust backend renders these as usize.
       */
      if( nAp >= lookbackChannel )
      {
         nDev = nAp - lookbackChannel;

         /* GUARD 1, the fixpoint test. With a constant input the exponential
          * step stops moving once k*|ap - esa| falls under half an ulp, and
          * esa then FREEZES up to (n1+1)/4 ulps away from the price. That
          * frozen residue is a real non-zero distance, so the naive form
          * divides it by its own exponential average and walks to
          * +/-1/0.015 = +/-66.67 -- an extreme reading produced by nothing
          * but rounding. When the pair has not moved, the average has reached
          * its fixpoint and the distance is exactly that residue, so the
          * numerator is taken as zero. The test is exact, so it is
          * independent of scale and period.
          *
          * Only once prevEsa is an average: on the bar that seeds esa it is
          * still the 0.0 placeholder, which a seed of exactly 0.0 equals.
          */
         num = ap - esa;
         if( nAp >= optInChannelPeriod && ap == prevAp && esa == prevEsa )
            num = 0.0;

         dev = fabs( num );

         if( nDev < optInChannelPeriod )
         {
            sumD = sumD + dev;
            if( nDev == optInChannelPeriod - 1 )
               d = sumD / optInChannelPeriod;
         }
         else
            d = k1 * dev + beta1 * d;

         /* Stage 3: the oscillator, then its own smoothing. */
         if( nDev >= lookbackChannel )
         {
            nCi = nDev - lookbackChannel;

            /* GUARD 2, the exact divisor. Test the PRODUCT the division uses,
             * not the deviation: 0.015*d underflows to zero while d is still
             * non-zero (#395). A zero divisor is 0/0 -- no distance against no
             * average distance -- so the oscillator reads its neutral 0.0
             * (#112), as tsi.c does.
             */
            scaledDev = 0.015 * d;
            if( scaledDev > 0.0 )
               ci = num / scaledDev;
            else
               ci = 0.0;

            if( nCi < optInAveragePeriod )
            {
               sumCi = sumCi + ci;
               if( nCi == optInAveragePeriod - 1 )
                  wt1 = sumCi / optInAveragePeriod;
            }
            else
               wt1 = k2 * ci + beta2 * wt1;

            if( nCi >= lookbackAverage )
            {
               wtBuffer[wtBuffer_Idx] = wt1;
               sumSignal = sumSignal + wt1;
               CIRCBUF_NEXT(wtBuffer);
            }
         }
      }

      prevAp = ap;
      prevEsa = esa;
      nAp = nAp + 1;
      today = today + 1;
   }

   /* The first output. The warm-up's last pass stored this bar's WT1 into the
    * ring and added it to the running sum, so the average is ready here.
    */
   outWT1[0] = wt1;
   outWT2[0] = sumSignal / (double)optInSignalPeriod;
   outIdx = 1;
   sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];

   /* Stable zone. One bar past the first output every stage is past its seed,
    * at every reachable parameter triple: the shortest case, 2/1/1, arrives
    * with the deviation average and the oscillator smoothing both one bar
    * into their recursions. So nothing here branches on a counter, and the
    * stores always run -- which is what keeps the managed backends' peek
    * frames from carrying a seeded output local.
    */
   while( today <= endIdx )
   {
      ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
      esa = k1 * ap + beta1 * esa;

      num = ap - esa;
      if( ap == prevAp && esa == prevEsa )
         num = 0.0;

      dev = fabs( num );

      d = k1 * dev + beta1 * d;

      scaledDev = 0.015 * d;
      if( scaledDev > 0.0 )
         ci = num / scaledDev;
      else
         ci = 0.0;

      wt1 = k2 * ci + beta2 * wt1;

      wtBuffer[wtBuffer_Idx] = wt1;
      sumSignal = sumSignal + wt1;
      outWT1[outIdx] = wt1;
      outWT2[outIdx] = sumSignal / (double)optInSignalPeriod;
      outIdx = outIdx + 1;
      CIRCBUF_NEXT(wtBuffer);
      sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];

      prevAp = ap;
      prevEsa = esa;
      today = today + 1;
   }

   CIRCBUF_DESTROY(wtBuffer);

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
