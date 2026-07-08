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

int cdladvanceblock_lookback(void)
{
   return max( max( max( ShadowLong_avgPeriod, ShadowShort_avgPeriod ),
      max( Far_avgPeriod, Near_avgPeriod ) ),
      BodyLong_avgPeriod
   ) + 2;
}

TA_RetCode cdladvanceblock(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double ShadowShortPeriodTotal[3];
   double ShadowLongPeriodTotal[2];
   double NearPeriodTotal[3];
   double FarPeriodTotal[3];
   double BodyLongPeriodTotal;
   int i, outIdx, totIdx, BodyLongTrailingIdx, ShadowShortTrailingIdx, ShadowLongTrailingIdx, NearTrailingIdx,
   FarTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdladvanceblock_lookback();

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
   ShadowShortPeriodTotal[2] = 0;
   ShadowShortPeriodTotal[1] = 0;
   ShadowShortPeriodTotal[0] = 0;
   ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
   ShadowLongPeriodTotal[1] = 0;
   ShadowLongPeriodTotal[0] = 0;
   ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
   NearPeriodTotal[2] = 0;
   NearPeriodTotal[1] = 0;
   NearPeriodTotal[0] = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;
   FarPeriodTotal[2] = 0;
   FarPeriodTotal[1] = 0;
   FarPeriodTotal[0] = 0;
   FarTrailingIdx = startIdx - Far_avgPeriod;
   BodyLongPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = ShadowShortTrailingIdx;
   while( i < startIdx ) {
      ShadowShortPeriodTotal[2] += ta_candlerange(ShadowShort_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      ShadowShortPeriodTotal[1] += ta_candlerange(ShadowShort_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowShortPeriodTotal[0] += ta_candlerange(ShadowShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowLongTrailingIdx;
   while( i < startIdx ) {
      ShadowLongPeriodTotal[1] += ta_candlerange(ShadowLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      ShadowLongPeriodTotal[0] += ta_candlerange(ShadowLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three white candlesticks with consecutively higher closes
    * - each candle opens within or near the previous white real body
    * - first candle: long white with no or very short upper shadow (a short shadow is accepted too for more flexibility)
    * - second and third candles, or only third candle, show signs of weakening: progressively smaller white real bodies
    * and/or relatively long upper shadows; see below for specific conditions
    * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings;
    * outInteger is negative (-1 to -100): advance block is always bearish;
    * the user should consider that advance block is significant when it appears in uptrend, while this function
    * does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                                     // 1st white
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                                     // 2nd white
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                       // 3rd white
         inClose[i] > inClose[i-1] && inClose[i-1] > inClose[i-2] &&                     // consecutive higher closes
         inOpen[i-1] > inOpen[i-2] &&                                                    // 2nd opens within/near 1st real body
         inOpen[i-1] <= inClose[i-2] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         inOpen[i] > inOpen[i-1] &&                                                      // 3rd opens within/near 2nd real body
         inOpen[i] <= inClose[i-1] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) && // 1st: long real body
         ta_uppershadow(inHigh[i-2], inClose[i-2], inOpen[i-2]) < ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, ShadowShortPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         // 1st: short upper shadow
         (
         // ( 2 far smaller than 1 && 3 not longer than 2 )
         // advance blocked with the 2nd, 3rd must not carry on the advance
         (
         ta_realbody(inClose[i-1], inOpen[i-1]) < ta_realbody(inClose[i-2], inOpen[i-2]) - ta_candleaverage(Far_rangeType, Far_avgPeriod, Far_factor, FarPeriodTotal[2], inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&
         ta_realbody(inClose[i], inOpen[i]) < ta_realbody(inClose[i-1], inOpen[i-1]) + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) ||
         // 3 far smaller than 2
         // advance blocked with the 3rd
         (
         ta_realbody(inClose[i], inOpen[i]) < ta_realbody(inClose[i-1], inOpen[i-1]) - ta_candleaverage(Far_rangeType, Far_avgPeriod, Far_factor, FarPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) ||
         // ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) )
         // advance blocked with progressively smaller real bodies and some upper shadows
         (
         ta_realbody(inClose[i], inOpen[i]) < ta_realbody(inClose[i-1], inOpen[i-1]) &&
         ta_realbody(inClose[i-1], inOpen[i-1]) < ta_realbody(inClose[i-2], inOpen[i-2]) &&
         (
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, ShadowShortPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i]) ||
         ta_uppershadow(inHigh[i-1], inClose[i-1], inOpen[i-1]) > ta_candleaverage(ShadowShort_rangeType, ShadowShort_avgPeriod, ShadowShort_factor, ShadowShortPeriodTotal[1], inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      )
      ) ||
         // ( 3 smaller than 2 && 3 long upper shadow )
         // advance blocked with 3rd candle's long upper shadow and smaller body
         (
         ta_realbody(inClose[i], inOpen[i]) < ta_realbody(inClose[i-1], inOpen[i-1]) &&
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowLong_rangeType, ShadowLong_avgPeriod, ShadowLong_factor, ShadowLongPeriodTotal[0], inOpen[i], inHigh[i], inLow[i], inClose[i])
      )
      )
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for (totIdx = 2; totIdx >= 0; --totIdx)
         ShadowShortPeriodTotal[totIdx] += ta_candlerange(ShadowShort_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowShort_rangeType, inOpen[ShadowShortTrailingIdx-totIdx], inHigh[ShadowShortTrailingIdx-totIdx], inLow[ShadowShortTrailingIdx-totIdx], inClose[ShadowShortTrailingIdx-totIdx]);
      for (totIdx = 1; totIdx >= 0; --totIdx)
         ShadowLongPeriodTotal[totIdx] += ta_candlerange(ShadowLong_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
      - ta_candlerange(ShadowLong_rangeType, inOpen[ShadowLongTrailingIdx-totIdx], inHigh[ShadowLongTrailingIdx-totIdx], inLow[ShadowLongTrailingIdx-totIdx], inClose[ShadowLongTrailingIdx-totIdx]);
      for (totIdx = 2; totIdx >= 1; --totIdx) {
         FarPeriodTotal[totIdx] += ta_candlerange(Far_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(Far_rangeType, inOpen[FarTrailingIdx-totIdx], inHigh[FarTrailingIdx-totIdx], inLow[FarTrailingIdx-totIdx], inClose[FarTrailingIdx-totIdx]);
         NearPeriodTotal[totIdx] += ta_candlerange(Near_rangeType, inOpen[i-totIdx], inHigh[i-totIdx], inLow[i-totIdx], inClose[i-totIdx])
         - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-totIdx], inHigh[NearTrailingIdx-totIdx], inLow[NearTrailingIdx-totIdx], inClose[NearTrailingIdx-totIdx]);
      }
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-2], inHigh[BodyLongTrailingIdx-2], inLow[BodyLongTrailingIdx-2], inClose[BodyLongTrailingIdx-2]);
      i++;
      ShadowShortTrailingIdx++;
      ShadowLongTrailingIdx++;
      NearTrailingIdx++;
      FarTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
