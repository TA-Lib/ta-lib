/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120404 AC   Creation
 *
 */

int cdl3whitesoldiers_lookback(void)
{
   return max( max( ShadowVeryShort_avgPeriod, BodyShort_avgPeriod ),
      max( Far_avgPeriod, Near_avgPeriod )
   ) + 2;
}

TA_RetCode cdl3whitesoldiers(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowVeryShortPeriodTotal[3];
   double NearPeriodTotal[3];
   double FarPeriodTotal[3];
   double BodyShortPeriodTotal;
   int i, outIdx, totIdx, ShadowVeryShortTrailingIdx, NearTrailingIdx, FarTrailingIdx, BodyShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdl3whitesoldiers_lookback();

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

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   ShadowVeryShortPeriodTotal[2] = 0;
   ShadowVeryShortPeriodTotal[1] = 0;
   ShadowVeryShortPeriodTotal[0] = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   NearPeriodTotal[2] = 0;
   NearPeriodTotal[1] = 0;
   NearPeriodTotal[0] = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;
   FarPeriodTotal[2] = 0;
   FarPeriodTotal[1] = 0;
   FarPeriodTotal[0] = 0;
   FarTrailingIdx = startIdx - Far_avgPeriod;
   BodyShortPeriodTotal = 0;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;

   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal[2] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      ShadowVeryShortPeriodTotal[1] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowVeryShortPeriodTotal[0] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal[2] += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      NearPeriodTotal[1] += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = FarTrailingIdx;
   while( i < startIdx ) {
      FarPeriodTotal[2] += ta_candlerange(Far_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      FarPeriodTotal[1] += ta_candlerange(Far_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three white candlesticks with consecutively higher closes
    * - Greg Morris wants them to be long, Steve Nison doesn't; anyway they should not be short
    * - each candle opens within or near the previous white real body
    * - each candle must have no or very short upper shadow
    * - to differentiate this pattern from advance block, each candle must not be far shorter than the prior candle
    * The meanings of "not short", "very short shadow", "far" and "near" are specified with TA_SetCandleSettings;
    * here the 3 candles must be not short, if you want them to be long use TA_SetCandleSettings on BodyShort;
    * outInteger is positive (1 to 100): advancing 3 white soldiers is always bullish;
    * the user should consider that 3 white soldiers is significant when it appears in downtrend, while this function
    * does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                                     // 1st white
         ta_uppershadow(inHigh[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         // very short upper shadow
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                                     // 2nd white
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         // very short upper shadow
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                       // 3rd white
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) &&
         // very short upper shadow
         inClose[i] > inClose[i-1] && inClose[i-1] > inClose[i-2] &&                     // consecutive higher closes
         inOpen[i-1] > inOpen[i-2] &&                                                    // 2nd opens within/near 1st real body
         inOpen[i-1] <= inClose[i-2] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         inOpen[i] > inOpen[i-1] &&                                                      // 3rd opens within/near 2nd real body
         inOpen[i] <= inClose[i-1] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_realbody(inClose[i-2], inOpen[i-2]) - ta_candleaverage(Far_rangeType, Far_avgPeriod, Far_factor, FarPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         // 2nd not far shorter than 1st
         ta_realbody(inClose[i], inOpen[i]) > ta_realbody(inClose[i-1], inOpen[i-1]) - ta_candleaverage(Far_rangeType, Far_avgPeriod, Far_factor, FarPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         // 3rd not far shorter than 2nd
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])      // not short real body
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 2; totIdx >= 0; --totIdx)
         ShadowVeryShortPeriodTotal[totIdx] += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-totIdx], inHigh[ShadowVeryShortTrailingIdx-totIdx], inLow[ShadowVeryShortTrailingIdx-totIdx], inClose[ShadowVeryShortTrailingIdx-totIdx]);
      for (totIdx = 2; totIdx >= 1; --totIdx) {
         FarPeriodTotal[totIdx] += ta_candlerange(Far_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(Far_rangeType, inOpen[FarTrailingIdx-totIdx], inHigh[FarTrailingIdx-totIdx], inLow[FarTrailingIdx-totIdx], inClose[FarTrailingIdx-totIdx]);
         NearPeriodTotal[totIdx] += ta_candlerange(Near_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-totIdx], inHigh[NearTrailingIdx-totIdx], inLow[NearTrailingIdx-totIdx], inClose[NearTrailingIdx-totIdx]);
      }
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      i++;
      ShadowVeryShortTrailingIdx++;
      NearTrailingIdx++;
      FarTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
