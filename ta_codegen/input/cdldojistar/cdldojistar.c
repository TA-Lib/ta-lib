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
 *  100204 AC   Creation
 *
 */

int cdldojistar_lookback(void)
{
   return max( BodyDoji_avgPeriod, BodyLong_avgPeriod ) + 1;
}

TA_RetCode cdldojistar(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyDojiPeriodTotal, BodyLongPeriodTotal;
   int i, outIdx, BodyDojiTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdldojistar_lookback();

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
   BodyLongTrailingIdx = startIdx -1 - BodyLong_avgPeriod;
   BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx-1 ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = BodyDojiTrailingIdx;
   while( i < startIdx ) {
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long real body
    * - second candle: star (open gapping up in an uptrend or down in a downtrend) with a doji
    * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * it's defined bullish when the long candle is white and the star gaps up, bearish when the long candle
    * is black and the star gaps down; the user should consider that a doji star is bullish when it appears
    * in an uptrend and it's bearish when it appears in a downtrend, so to determine the bullishness or
    * bearishness of the pattern the trend must be analyzed
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&     // 1st: long real body
         ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&        // 2nd: doji
         ( ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 && ta_realbodygapup(inOpen[i], inClose[i], inOpen[i-1], inClose[i-1]) )                        //        that gaps up if 1st is white
         ||
         ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 && ta_realbodygapdown(inOpen[i], inClose[i], inOpen[i-1], inClose[i-1]) )                        //      or down if 1st is black
      ) ) {
         outInteger[outIdx++] = -ta_candlecolor(inClose[i-1], inOpen[i-1]) * 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx], inHigh[BodyLongTrailingIdx], inLow[BodyLongTrailingIdx], inClose[BodyLongTrailingIdx]);
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyDojiTrailingIdx], inHigh[BodyDojiTrailingIdx], inLow[BodyDojiTrailingIdx], inClose[BodyDojiTrailingIdx]);
      i++;
      BodyLongTrailingIdx++;
      BodyDojiTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
