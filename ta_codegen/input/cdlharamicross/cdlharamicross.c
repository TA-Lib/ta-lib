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
 *  102404 AC   Creation
 *  040309 AC   Increased flexibility to allow real bodies matching
 *              on one end (Greg Morris - "Candlestick charting explained")
 *
 */

int cdlharamicross_lookback(void)
{
   return max( BodyDoji_avgPeriod, BodyLong_avgPeriod ) + 1;
}

TA_RetCode cdlharamicross(int startIdx, int endIdx,
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

   lookbackTotal = cdlharamicross_lookback();

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
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white (black) real body
    * - second candle: doji totally engulfed by the first
    * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
    * the user should consider that a harami cross is significant when it appears in a downtrend if bullish or
    * in an uptrend when bearish, while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) ) {  // 1st: long
         if( ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) ) {  // 2nd: doji
            if ( max( inClose[i], inOpen[i] ) < max( inClose[i-1], inOpen[i-1] ) &&              // 2nd is engulfed by 1st
               min( inClose[i], inOpen[i] ) > min( inClose[i-1], inOpen[i-1] )
            ) {
               outInteger[outIdx++] = -ta_candlecolor(inClose[i-1], inOpen[i-1]) * 100;
            }
            else
               if ( max( inClose[i], inOpen[i] ) <= max( inClose[i-1], inOpen[i-1] ) &&         // 2nd is engulfed by 1st
               min( inClose[i], inOpen[i] ) >= min( inClose[i-1], inOpen[i-1] )            // (one end of real body can match;
            ) {  // engulfing guaranteed by "long" and "doji")
               outInteger[outIdx++] = -ta_candlecolor(inClose[i-1], inOpen[i-1]) * 80;
            }
            else {
               outInteger[outIdx++] = 0;
            }
         }
         else {
            outInteger[outIdx++] = 0;
         }
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
