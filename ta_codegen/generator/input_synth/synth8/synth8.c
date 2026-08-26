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
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth8.md — one copy, so there is one thing to keep true.
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
