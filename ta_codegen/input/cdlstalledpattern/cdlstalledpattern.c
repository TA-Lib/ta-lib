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
 *  120804 AC   Creation
 *
 */

int cdlstalledpattern_lookback(void)
{
   return max( max( BodyLong_avgPeriod, BodyShort_avgPeriod ),
      max( ShadowVeryShort_avgPeriod, Near_avgPeriod )
   ) + 2;
}

TA_RetCode cdlstalledpattern(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal[3];
   double NearPeriodTotal[3];
   double BodyShortPeriodTotal, ShadowVeryShortPeriodTotal;
   int i, outIdx, totIdx, BodyLongTrailingIdx, BodyShortTrailingIdx, ShadowVeryShortTrailingIdx, NearTrailingIdx,
   lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlstalledpattern_lookback();

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
   BodyLongPeriodTotal[2] = 0;
   BodyLongPeriodTotal[1] = 0;
   BodyLongPeriodTotal[0] = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
   BodyShortPeriodTotal = 0;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
   NearPeriodTotal[2] = 0;
   NearPeriodTotal[1] = 0;
   NearPeriodTotal[0] = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal[2] += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      BodyLongPeriodTotal[1] += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal[2] += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      NearPeriodTotal[1] += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three white candlesticks with consecutively higher closes
    * - first candle: long white
    * - second candle: long white with no or very short upper shadow opening within or near the previous white real body
    * and closing higher than the prior candle
    * - third candle: small white that gaps away or "rides on the shoulder" of the prior long real body (= it's at
    * the upper end of the prior real body)
    * The meanings of "long", "very short", "short", "near" are specified with TA_SetCandleSettings;
    * outInteger is negative (-1 to -100): stalled pattern is always bearish;
    * the user should consider that stalled pattern is significant when it appears in uptrend, while this function
    * does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                             // 1st white
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                             // 2nd white
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                               // 3rd white
         inClose[i] > inClose[i-1] && inClose[i-1] > inClose[i-2] &&             // consecutive higher closes
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&  // 1st: long real body
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&  // 2nd: long real body
         // very short upper shadow
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) < ta_candleaverage(ShadowVeryShort_rangeType, ShadowVeryShort_avgPeriod, ShadowVeryShort_factor, ShadowVeryShortPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         // opens within/near 1st real body
         inOpen[i-1] > inOpen[i-2] &&
         inOpen[i-1] <= inClose[i-2] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_realbody(inClose[i], inOpen[i]) < ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&       // 3rd: small real body
         // rides on the shoulder of 2nd real body
         inOpen[i] >= inClose[i-1] - ta_realbody(inClose[i], inOpen[i]) - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 2; totIdx >= 1; --totIdx) {
         BodyLongPeriodTotal[totIdx] += ta_candlerange(BodyLong_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-totIdx], inHigh[BodyLongTrailingIdx-totIdx], inLow[BodyLongTrailingIdx-totIdx], inClose[BodyLongTrailingIdx-totIdx]);
         NearPeriodTotal[totIdx] += ta_candlerange(Near_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-totIdx], inHigh[NearTrailingIdx-totIdx], inLow[NearTrailingIdx-totIdx], inClose[NearTrailingIdx-totIdx]);
      }
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      ShadowVeryShortPeriodTotal += ta_candlerange(ShadowVeryShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      - ta_candlerange(ShadowVeryShort_rangeType, inOpen[ShadowVeryShortTrailingIdx-1], inHigh[ShadowVeryShortTrailingIdx-1], inLow[ShadowVeryShortTrailingIdx-1], inClose[ShadowVeryShortTrailingIdx-1]);
      i++;
      BodyLongTrailingIdx++;
      BodyShortTrailingIdx++;
      ShadowVeryShortTrailingIdx++;
      NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
