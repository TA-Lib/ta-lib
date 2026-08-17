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
 *  081626 MF,CC  Initial version (#227).
 *
 */

int ao_lookback(int optInFastPeriod, int optInSlowPeriod)
{
   /* The longer of the two windows drives the lookback, and it is the lookback
    * of that window's SMA. There is no swap of an inverted pair, so the max is
    * taken over the periods exactly as the caller gave them.
    */
   return sma_lookback( max(optInFastPeriod, optInSlowPeriod) );
}

TA_RetCode ao(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumFast;
   double sumSlow;
   double medianPrice;
   double tempReal;
   size_t i;
   size_t outIdx;
   size_t trailingFastIdx;
   size_t trailingSlowIdx;
   size_t lookbackTotal;

   /* Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): the
    * spread between a short and a long simple moving average of the median
    * price, drawn as a zero-centred histogram.
    *
    *    median_t = (high_t + low_t) / 2
    *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t
    *
    * Both legs are plain SMAs, so there is no seeding convention to get wrong
    * and no cross-library divergence of the kind an EMA brings.
    *
    * This is two copies of ta_codegen/input/sma/sma.c walking one derived
    * series, sharing a single pass: the same running-sum order, the same
    * snapshot of each total before the trailing bar leaves it, and the same
    * divide by the period. Keeping the arithmetic identical is what makes the
    * composed reference in test_composite.c -- TA_MEDPRICE, two TA_SMA calls
    * and a TA_SUB -- bit-exact rather than merely close, so any future drift
    * in either path is a hard failure instead of a tolerance argument.
    *
    * In particular each total is DIVIDED by its period; it is not multiplied
    * by a precomputed 1/period. Tulip's ao.c multiplies by per5/per34, which
    * costs it up to one ULP against TA_SMA. Dividing buys the memcmp
    * differential, which is the stronger of the two gates (#117, #118).
    *
    * An inverted pair (fast > slow) is NOT swapped, unlike MACD and APO. The
    * lookback is well defined either way and the result is simply -AO, so the
    * swap would buy nothing and would have to be duplicated in ao_lookback.
    */

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = (size_t)ao_lookback( optInFastPeriod, optInSlowPeriod );

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

   sumFast = 0.0;
   sumSlow = 0.0;
   trailingFastIdx = startIdx - (size_t)(optInFastPeriod-1);
   trailingSlowIdx = startIdx - (size_t)(optInSlowPeriod-1);

   /* Add-up both initial periods, except for the last value.
    *
    * One pass over the longer warm-up window replaces two overlapping passes.
    * A bar inside the shorter window is added to that total as it is reached,
    * so each total still accumulates exactly the same bars in exactly the same
    * ascending order two separate loops would have given it -- which is what
    * keeps each leg bit-identical to the TA_SMA called with this same startIdx.
    */
   i = startIdx - lookbackTotal;
   while( i < startIdx )
   {
      medianPrice = (inHigh[i]+inLow[i])/2.0;
      if( i >= trailingFastIdx )
         sumFast += medianPrice;
      if( i >= trailingSlowIdx )
         sumSlow += medianPrice;
      i = i + 1;
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
      i = i + 1;

      /* Snapshot the oscillator before either total drops its trailing bar,
       * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
       */
      tempReal = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;

      /* Read both trailing bars before writing the output. When startIdx is
       * clamped to the lookback the longer window's trailing index equals
       * outIdx exactly, so a store hoisted above this would read back the
       * value it had just overwritten whenever the caller aliases outReal
       * over inHigh or inLow.
       */
      sumFast -= (inHigh[trailingFastIdx]+inLow[trailingFastIdx])/2.0;
      sumSlow -= (inHigh[trailingSlowIdx]+inLow[trailingSlowIdx])/2.0;
      trailingFastIdx = trailingFastIdx + 1;
      trailingSlowIdx = trailingSlowIdx + 1;

      outReal[outIdx] = tempReal;
      outIdx = outIdx + 1;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
