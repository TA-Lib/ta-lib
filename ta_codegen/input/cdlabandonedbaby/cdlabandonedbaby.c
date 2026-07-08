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
 *  102304 AC   Creation
 *
 */

int cdlabandonedbaby_lookback(double        optInPenetration)
{
   (void)optInPenetration;

   return max( max( BodyDoji_avgPeriod, BodyLong_avgPeriod ),
      BodyShort_avgPeriod
   ) + 2;
}

TA_RetCode cdlabandonedbaby(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInPenetration,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyDojiPeriodTotal, BodyLongPeriodTotal, BodyShortPeriodTotal;
   int i, outIdx, BodyDojiTrailingIdx, BodyLongTrailingIdx, BodyShortTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlabandonedbaby_lookback(optInPenetration);

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
   BodyLongPeriodTotal = 0;
   BodyDojiPeriodTotal = 0;
   BodyShortPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx -2 - BodyLong_avgPeriod;
   BodyDojiTrailingIdx = startIdx -1 - BodyDoji_avgPeriod;
   BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx-2 ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyDojiTrailingIdx;
   while( i < startIdx-1 ) {
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
    * - first candle: long white (black) real body
    * - second candle: doji
    * - third candle: black (white) real body that moves well within the first candle's real body
    * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
    * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
    * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
    * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
    * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
    * it to be relatively long
    * outInteger is positive (1 to 100) when it's an abandoned baby bottom or negative (-1 to -100) when it's
    * an abandoned baby top; the user should consider that an abandoned baby is significant when it appears in
    * an uptrend or downtrend, while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i-2], inOpen[i-2]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) &&         // 1st: long
         ta_realbody(inClose[i-1], inOpen[i-1]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&        // 2nd: doji
         ta_realbody(inClose[i], inOpen[i]) > ta_candleaverage(BodyShort_rangeType, BodyShort_avgPeriod, BodyShort_factor, BodyShortPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&           // 3rd: longer than short
         ( ( ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&                                                         // 1st white
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                                          // 3rd black
         inClose[i] < inClose[i-2] - ta_realbody(inClose[i-2], inOpen[i-2]) * optInPenetration &&                  // 3rd closes well within 1st rb
         ta_candlegapup(inLow[i-1], inHigh[i-2]) &&                                                          // upside gap between 1st and 2nd
         ta_candlegapdown(inHigh[i], inLow[i-1])                                                             // downside gap between 2nd and 3rd
      )
         ||
         (
         ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                                        // 1st black
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                           // 3rd white
         inClose[i] > inClose[i-2] + ta_realbody(inClose[i-2], inOpen[i-2]) * optInPenetration &&                  // 3rd closes well within 1st rb
         ta_candlegapdown(inHigh[i-1], inLow[i-2]) &&                                                        // downside gap between 1st and 2nd
         ta_candlegapup(inLow[i], inHigh[i-1])                                                               // upside gap between 2nd and 3rd
      )
      )
      )
      {
         outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
      }
      else
      {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyDojiTrailingIdx], inHigh[BodyDojiTrailingIdx], inLow[BodyDojiTrailingIdx], inClose[BodyDojiTrailingIdx]);
      BodyShortPeriodTotal += ta_candlerange(BodyShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyShort_rangeType, inOpen[BodyShortTrailingIdx], inHigh[BodyShortTrailingIdx], inLow[BodyShortTrailingIdx], inClose[BodyShortTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      BodyDojiTrailingIdx++;
      BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
