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
 *  081726 MF,CC  Initial version (#228).
 *
 */

int ac_lookback(int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod)
{
   /* The oscillator's own window, plus the simple moving average taken over
    * the oscillator itself. Both terms are exactly the lookback of the
    * function they come from, so neither is restated here.
    */
   return ao_lookback( optInFastPeriod, optInSlowPeriod ) + sma_lookback( optInSignalPeriod );
}

TA_RetCode ac(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInSignalPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumFast;
   double sumSlow;
   double sumSignal;
   double medianPrice;
   double osc;
   double tempReal;
   int i;
   int outIdx;
   int trailingFastIdx;
   int trailingSlowIdx;
   int oscStartIdx;
   int lookbackTotal;

   /* Bill Williams' Accelerator/Decelerator Oscillator (New Trading
    * Dimensions, 1998): how fast the Awesome Oscillator is itself
    * accelerating, drawn as a zero-centred histogram.
    *
    *    AO_t = SMA(median, fast)_t - SMA(median, slow)_t
    *    AC_t = AO_t - SMA(AO, signal)_t
    *
    * Every leg is a plain SMA, so there is no seeding convention to get wrong
    * and no cross-library divergence of the kind an EMA brings.
    *
    * This is ta_codegen/input/ao/ao.c walking one derived series with a third
    * running sum layered on top: the oscillator is never materialised into an
    * array, it goes straight into a ring of the last `signal` values. The
    * order is the one ta_codegen/input/sma/sma.c uses -- add the new value,
    * snapshot the total, subtract the bar leaving the window, divide by the
    * period -- for all three sums, which is what makes the composed reference
    * in test_composite.c (TA_AO, TA_SMA and TA_SUB) bit-exact rather than
    * merely close. Any future drift in either path is then a hard failure
    * instead of a tolerance argument.
    *
    * Each total is DIVIDED by its period; it is not multiplied by a
    * precomputed 1/period, for the reason spelled out in ao.c and #117/#118.
    */

   /* This ptr will point on a circular buffer of at least
    * "optInSignalPeriod" element.
    */
   CIRCBUF_PROLOG(oscBuffer,double,32);

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = ac_lookback( optInFastPeriod, optInSlowPeriod, optInSignalPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Allocate a circular buffer equal to the requested signal period. */
   CIRCBUF_INIT( oscBuffer, double, optInSignalPeriod );

   /* The first bar the oscillator is evaluated on for this call. It trails
    * the first output by the signal window, because that many oscillator
    * values have to exist before their average does.
    */
   oscStartIdx = startIdx - (optInSignalPeriod-1);

   sumFast = 0.0;
   sumSlow = 0.0;
   sumSignal = 0.0;
   trailingFastIdx = oscStartIdx - (optInFastPeriod-1);
   trailingSlowIdx = oscStartIdx - (optInSlowPeriod-1);

   /* Add-up both initial periods, except for the last value.
    *
    * One pass over the longer warm-up window replaces two overlapping passes.
    * A bar inside the shorter window is added to that total as it is reached,
    * so each total still accumulates exactly the same bars in exactly the same
    * ascending order two separate loops would have given it -- which is what
    * keeps each leg bit-identical to the TA_SMA that TA_AO would have called
    * had it been started at oscStartIdx.
    */
   i = startIdx - lookbackTotal;
   while( i < oscStartIdx )
   {
      medianPrice = (inHigh[i]+inLow[i])/2.0;
      if( i >= trailingFastIdx )
         sumFast += medianPrice;
      if( i >= trailingSlowIdx )
         sumSlow += medianPrice;
      i = i + 1;
   }

   /* Fill the signal ring with the oscillator values that precede the first
    * output. This is the same body as the main loop below minus the store --
    * the window is not full yet, so there is nothing to emit.
    */
   while( i < startIdx )
   {
      medianPrice = (inHigh[i]+inLow[i])/2.0;
      sumFast += medianPrice;
      sumSlow += medianPrice;
      osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
      sumFast -= (inHigh[trailingFastIdx]+inLow[trailingFastIdx])/2.0;
      sumSlow -= (inHigh[trailingSlowIdx]+inLow[trailingSlowIdx])/2.0;
      trailingFastIdx = trailingFastIdx + 1;
      trailingSlowIdx = trailingSlowIdx + 1;
      i = i + 1;

      oscBuffer[oscBuffer_Idx] = osc;
      sumSignal += osc;
      CIRCBUF_NEXT(oscBuffer);
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows outReal to be the same
    * buffer as either input.
    */
   outIdx = 0;
   while( i <= endIdx )
   {
      medianPrice = (inHigh[i]+inLow[i])/2.0;
      sumFast += medianPrice;
      sumSlow += medianPrice;

      /* Snapshot the oscillator before either total drops its trailing bar,
       * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
       */
      osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
      sumFast -= (inHigh[trailingFastIdx]+inLow[trailingFastIdx])/2.0;
      sumSlow -= (inHigh[trailingSlowIdx]+inLow[trailingSlowIdx])/2.0;
      trailingFastIdx = trailingFastIdx + 1;
      trailingSlowIdx = trailingSlowIdx + 1;
      i = i + 1;

      /* Today's oscillator enters the signal window at its own slot, and the
       * bar leaving that window is read only after the ring has advanced onto
       * it -- writing first is what makes the slot the loop is about to
       * overwrite the newest value rather than the oldest one.
       */
      oscBuffer[oscBuffer_Idx] = osc;
      sumSignal += osc;
      tempReal = osc - sumSignal / (double)optInSignalPeriod;
      CIRCBUF_NEXT(oscBuffer);
      sumSignal -= oscBuffer[oscBuffer_Idx];

      /* Every input read for this bar is done above, so the store is safe
       * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
       * there is slack here -- the signal window puts both trailing indices
       * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
       * parameter makes them collide -- but the order is kept anyway, so
       * that admitting a signal period of 1 would not silently reintroduce
       * the collision ao.c has to guard against.
       */
      outReal[outIdx] = tempReal;
      outIdx = outIdx + 1;
   }

   CIRCBUF_DESTROY(oscBuffer);

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
