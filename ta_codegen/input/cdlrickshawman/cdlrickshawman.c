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
 *  011505 AC   Creation
 *
 */

int cdlrickshawman_lookback(void)
{
   return max( max( BodyDoji_avgPeriod, ShadowLong_avgPeriod ),
      Near_avgPeriod
   );
}

TA_RetCode cdlrickshawman(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyDojiPeriodTotal, ShadowLongPeriodTotal, NearPeriodTotal;
   int i, outIdx, BodyDojiTrailingIdx, ShadowLongTrailingIdx, NearTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlrickshawman_lookback();

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
   BodyDojiPeriodTotal = 0;
   BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
   ShadowLongPeriodTotal = 0;
   ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
   NearPeriodTotal = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;

   i = BodyDojiTrailingIdx;
   while( i < startIdx ) {
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = ShadowLongTrailingIdx;
   while( i < startIdx ) {
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }
   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    *
    * Must have:
    * - doji body
    * - two long shadows
    * - body near the midpoint of the high-low range
    * The meaning of "doji" and "near" is specified with TA_SetCandleSettings
    * outInteger is always positive (1 to 100) but this does not mean it is bullish: rickshaw man shows uncertainty
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&            // doji
         ta_lowershadow(inLow[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowLong_rangeType, ShadowLong_avgPeriod, ShadowLong_factor, ShadowLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&      // long shadow
         ta_uppershadow(inHigh[i], inClose[i], inOpen[i]) > ta_candleaverage(ShadowLong_rangeType, ShadowLong_avgPeriod, ShadowLong_factor, ShadowLongPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) &&      // long shadow
         (                                                                                       // body near midpoint
         min( inOpen[i], inClose[i] )
         <= inLow[i] + ta_highlowrange(inHigh[i], inLow[i]) / 2 + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
         &&
         max( inOpen[i], inClose[i] )
         >= inLow[i] + ta_highlowrange(inHigh[i], inLow[i]) / 2 - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i])
      )
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }

      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyDojiTrailingIdx], inHigh[BodyDojiTrailingIdx], inLow[BodyDojiTrailingIdx], inClose[BodyDojiTrailingIdx]);
      ShadowLongPeriodTotal += ta_candlerange(ShadowLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(ShadowLong_rangeType, inOpen[ShadowLongTrailingIdx], inHigh[ShadowLongTrailingIdx], inLow[ShadowLongTrailingIdx], inClose[ShadowLongTrailingIdx]);
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx], inHigh[NearTrailingIdx], inLow[NearTrailingIdx], inClose[NearTrailingIdx]);

      i++;
      BodyDojiTrailingIdx++;
      ShadowLongTrailingIdx++;
      NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
