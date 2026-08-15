/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081426 KL   Creation (synthetic gate: ta_candleaverage values, #216).
 *
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is ta_candleaverage with its VALUE as the output;
 * SYNTH7 is the same idea for ta_candlerange, and carries the rationale for
 * making these intermediates observable at all.
 *
 * Two outputs cover both of the helper's branches and both of its divisors:
 *
 *    outAvgShadows     ShadowShort     avgPeriod 10 -> divides the running
 *                                      sum; rangeType Shadows -> the /2
 *                                      divisor; and the divergent arm
 *    outAvgCurrentBar  ShadowVeryLong  avgPeriod 0 -> reads the CURRENT
 *                                      bar's range instead of the sum, the
 *                                      branch that had no cross-language
 *                                      coverage at all (#216 gap 5);
 *                                      rangeType RealBody -> divisor 1.0
 *
 * The sum passed for outAvgCurrentBar is ignored by that branch by
 * construction. It is passed anyway because that is what a real caller does,
 * and a backend that wrongly consulted it would diverge here.
 *
 * TWO PROPERTIES MAKE THIS SAFE TO ALIAS, and both are load-bearing:
 *
 *   1. The window's per-bar ranges are kept in a circular buffer, so a bar
 *      that has left the window is never read from the input arrays again --
 *      the same reason cmf.c carries its volume in the buffer rather than
 *      re-reading inVolume[] at the trailing index.
 *   2. Every read of the current bar happens BEFORE any output is written.
 *      outReal may alias an input array, and the C backend maps these helpers
 *      onto macros that read inOpen[i]/inHigh[i]/... directly, so storing one
 *      output before computing the next would hand the next helper a
 *      clobbered bar.
 *
 * No shipped candlestick has to satisfy either: their outputs are int, which
 * the alias gate never aliases onto a double input. SYNTH7 and SYNTH8 are the
 * first OHLC consumers with real outputs, so they are the first to be held to
 * it -- and the gate caught both of them the first time round.
 *
 * Nothing is clamped, for the reason given in synth7.c: the divergent region
 * is bars whose low sits below half their high, and clamping those away would
 * hide exactly what the gate measures.
 */

int synth8_lookback(void)
{
   /* Enough history to seed the averaging window from valid bars. */
   return ShadowShort_avgPeriod;
}

TA_RetCode synth8(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outAvgShadows[],
   double outAvgCurrentBar[])
{
   double sumShadow, range, avgShadows, avgCurrentBar;
   int lookbackTotal, outIdx, i, today;

   /* One slot per bar of the averaging window, holding that bar's range. */
   typedef struct { double range; } ShadowRange;
   CIRCBUF_PROLOG_CLASS( sr, ShadowRange, 50 ); /* Id, Type, Static Size */

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = synth8_lookback();

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   CIRCBUF_INIT_CLASS( sr, ShadowRange, ShadowShort_avgPeriod );

   /* Seed the window from the bars preceding startIdx, exactly the convention
    * the candlesticks use: the current bar is NOT part of its own average.
    */
   sumShadow = 0.0;
   today = startIdx - lookbackTotal;
   for( i = ShadowShort_avgPeriod; i > 0; i-- )
   {
      range = ta_candlerange(ShadowShort_rangeType, inOpen[today], inHigh[today], inLow[today], inClose[today]);
      sr_range[sr_Idx] = range;
      sumShadow += range;
      today++;
      CIRCBUF_NEXT(sr);
   }

   outIdx = 0;

   while( today <= endIdx )
   {
      avgShadows = ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, sumShadow, inOpen[today], inHigh[today], inLow[today], inClose[today]);
      avgCurrentBar = ta_candleaverage(ShadowVeryLong_rangeType, ShadowVeryLong_avgPeriod, ShadowVeryLong_factor, sumShadow, inOpen[today], inHigh[today], inLow[today], inClose[today]);
      range = ta_candlerange(ShadowShort_rangeType, inOpen[today], inHigh[today], inLow[today], inClose[today]);

      /* Bar `today` is fully consumed above; only now is it safe to store. */
      outAvgShadows[outIdx] = avgShadows;
      outAvgCurrentBar[outIdx] = avgCurrentBar;
      outIdx++;

      /* Slide the window. The departing value comes from the buffer, never
       * from a re-read of the input -- that is what keeps the outputs safe to
       * alias.
       *
       * The update is written as one `+= new - old` expression because that is
       * what every shipped candlestick does (cdllongline.c). Splitting it into
       * `-= old` then `+= new` is a DIFFERENT floating-point computation: it
       * rounds twice against the running total instead of once against the
       * difference, and measurably re-rounds a per-bar 1-ULP difference away.
       * Matching the library's own accumulation is the point -- the gate is
       * meant to see what the library would see.
       */
      sumShadow += range - sr_range[sr_Idx];
      sr_range[sr_Idx] = range;
      CIRCBUF_NEXT(sr);
      today++;
   }

   CIRCBUF_DESTROY(sr);

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
